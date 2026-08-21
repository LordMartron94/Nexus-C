#include "../nexus.h"
#include "./n_internal.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <process.h>
#  include <stdlib.h>
#  include <windows.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  include <errno.h>
#  include <signal.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <fcntl.h>

extern char **environ;
#else
#  error "Unsupported platform"
#endif

#if defined(NEXUS_PLATFORM_WINDOWS)

static boolean nexus_process_windows_argument_needs_quotes(const char *argument) {
  uint_large index;

  if (argument == NULL) {
    return FALSE;
  }

  if (argument[0] == '\0') {
    return TRUE;
  }

  for (index = 0; argument[index] != '\0'; index++) {
    if (argument[index] == ' ' || argument[index] == '\t' || argument[index] == '"') {
      return TRUE;
    }
  }

  return FALSE;
}

static NError nexus_process_windows_command_line_append_character(char *buffer, uint_large buffer_max_length, uint_large *write_index,
                                                                  char character) {
  if (*write_index + 1 >= buffer_max_length) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  buffer[*write_index] = character;
  (*write_index)++;
  buffer[*write_index] = '\0';
  return NEXUS_ERROR_NONE;
}

static NError nexus_process_windows_command_line_append_argument(char *buffer, uint_large buffer_max_length, uint_large *write_index,
                                                                 const char *argument) {
  uint_large argument_index;
  uint_large backslash_count;
  NError     status;

  if (argument == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (nexus_process_windows_argument_needs_quotes(argument) == FALSE) {
    while (argument[0] != '\0') {
      status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, argument[0]);
      if (status != NEXUS_ERROR_NONE) {
        return status;
      }
      argument++;
    }
    return NEXUS_ERROR_NONE;
  }

  status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, '"');
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  argument_index = 0;
  while (argument[argument_index] != '\0') {
    backslash_count = 0;
    while (argument[argument_index] == '\\') {
      backslash_count++;
      argument_index++;
    }

    if (argument[argument_index] == '"') {
      uint_large backslash_index;

      for (backslash_index = 0; backslash_index < (backslash_count * 2) + 1; backslash_index++) {
        status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, '\\');
        if (status != NEXUS_ERROR_NONE) {
          return status;
        }
      }
      status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, '"');
      if (status != NEXUS_ERROR_NONE) {
        return status;
      }
      argument_index++;
      continue;
    }

    if (argument[argument_index] == '\0') {
      uint_large backslash_index;

      for (backslash_index = 0; backslash_index < backslash_count * 2; backslash_index++) {
        status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, '\\');
        if (status != NEXUS_ERROR_NONE) {
          return status;
        }
      }
      break;
    }

    {
      uint_large backslash_index;

      for (backslash_index = 0; backslash_index < backslash_count; backslash_index++) {
        status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, '\\');
        if (status != NEXUS_ERROR_NONE) {
          return status;
        }
      }
    }

    status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, argument[argument_index]);
    if (status != NEXUS_ERROR_NONE) {
      return status;
    }
    argument_index++;
  }

  status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, write_index, '"');
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  return NEXUS_ERROR_NONE;
}

static NError nexus_process_windows_command_line_build(char *const *argv, int32 start_index, char *buffer, uint_large buffer_max_length) {
  int32      argument_index;
  uint_large write_index;
  NError     status;

  if (argv == NULL || buffer == NULL || buffer_max_length == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  write_index    = 0;
  buffer[0]      = '\0';
  argument_index = start_index;
  while (argv[argument_index] != NULL) {
    if (argument_index > start_index) {
      status = nexus_process_windows_command_line_append_character(buffer, buffer_max_length, &write_index, ' ');
      if (status != NEXUS_ERROR_NONE) {
        return status;
      }
    }

    status = nexus_process_windows_command_line_append_argument(buffer, buffer_max_length, &write_index, argv[argument_index]);
    if (status != NEXUS_ERROR_NONE) {
      return status;
    }

    argument_index++;
  }

  return NEXUS_ERROR_NONE;
}

#endif

struct NexusProcess {
#if defined(NEXUS_PLATFORM_WINDOWS)
  HANDLE process_handle;
  HANDLE thread_handle;
  HANDLE stdin_write;
  HANDLE stdout_read;
#else
  pid_t process_id;
  int   stdin_socket;
  int   stdout_socket;
#endif

  boolean running;
  boolean waited;
  int32   exit_code;
};

struct NexusProcessChildChannel {
#if defined(NEXUS_PLATFORM_WINDOWS)
  HANDLE read_handle;
  HANDLE write_handle;
#else
  int socket_handle;
#endif
};

#if defined(NEXUS_PLATFORM_WINDOWS)
extern char **_environ;
#endif

static char **n_process_environment_with_variable_create(char *const *environment, const char *name, const char *value) {
  char *const *source_environment;
  char       **result_environment;
  char        *entry;
  uint_large   name_length;
  uint_large   value_length;
  uint_large   entry_length;
  uint_large   source_index;
  uint_large   result_index;
  uint_large   retained_count;

  NEXUS_ASSERT_DEBUG(name != NULL);
  NEXUS_ASSERT_DEBUG(value != NULL);

  if (name == NULL || value == NULL || name[0] == '\0') {
    return NULL;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  source_environment = environment != NULL ? environment : _environ;
#else
  source_environment = environment != NULL ? environment : environ;
#endif

  retained_count = 0;
  for (source_index = 0; source_environment != NULL && source_environment[source_index] != NULL; source_index++) {
    if (nexus_strings_string_starts_with(source_environment[source_index], name) == TRUE &&
        source_environment[source_index][nexus_strings_string_length(name)] == '=') {
      continue;
    }
    retained_count++;
  }

  if (retained_count > (UINT_LARGE_MAX_VAL / NEXUS_SIZEOF(*result_environment)) - 2U) {
    return NULL;
  }

  result_environment = (char **)malloc(NEXUS_SIZEOF(*result_environment) * (retained_count + 2U));
  if (result_environment == NULL) {
    return NULL;
  }

  name_length  = nexus_strings_string_length(name);
  value_length = nexus_strings_string_length(value);
  if (name_length > UINT_LARGE_MAX_VAL - value_length - 2U) {
    free((void *)result_environment);
    return NULL;
  }

  entry_length = name_length + value_length + 2U;
  entry        = (char *)malloc(entry_length);
  if (entry == NULL) {
    free((void *)result_environment);
    return NULL;
  }

  nexus_memory_bytes_copy(entry, name, name_length);
  entry[name_length] = '=';
  nexus_memory_bytes_copy(entry + name_length + 1U, value, value_length);
  entry[name_length + value_length + 1U] = '\0';

  result_index = 0;
  for (source_index = 0; source_environment != NULL && source_environment[source_index] != NULL; source_index++) {
    if (nexus_strings_string_starts_with(source_environment[source_index], name) == TRUE && source_environment[source_index][name_length] == '=') {
      continue;
    }
    result_environment[result_index] = source_environment[source_index];
    result_index++;
  }

  result_environment[result_index] = entry;
  result_index++;
  result_environment[result_index] = NULL;

  return result_environment;
}

static void n_process_environment_with_variable_destroy(char **environment) {
  uint_large entry_index;

  if (environment == NULL) {
    return;
  }

  entry_index = 0;
  while (environment[entry_index] != NULL) {
    entry_index++;
  }

  NEXUS_ASSERT_MESSAGE_DEBUG(entry_index > 0, "Process environment with injected variable requires an injected entry.");
  free(environment[entry_index - 1U]);
  free((void *)environment);
}

#if defined(NEXUS_PLATFORM_WINDOWS)

static NError nexus_process_windows_environment_block_create(char *const *environment, char **out_environment_block) {
  char      *block;
  uint_large required_size;
  uint_large write_offset;
  uint_large string_length;
  uint32     i;

  NEXUS_ASSERT_DEBUG(out_environment_block != NULL);

  if (out_environment_block == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_environment_block = NULL;

  if (environment == NULL) {
    return NEXUS_ERROR_NONE;
  }

  required_size = 1;
  i             = 0;

  while (environment[i] != NULL) {
    string_length = nexus_strings_string_length(environment[i]);

    if (required_size > UINT_LARGE_MAX_VAL - string_length - 1U) {
      return NEXUS_ERROR_CAPACITY;
    }

    required_size += string_length + 1U;
    i++;
  }

  block = (char *)malloc(required_size);

  if (block == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

  write_offset = 0;
  i            = 0;

  while (environment[i] != NULL) {
    string_length = nexus_strings_string_length(environment[i]);

    if (string_length > 0) {
      nexus_memory_bytes_copy(block + write_offset, environment[i], string_length);
    }

    write_offset += string_length;
    block[write_offset] = '\0';
    write_offset++;
    i++;
  }

  block[write_offset] = '\0';

  *out_environment_block = block;

  return NEXUS_ERROR_NONE;
}

static void nexus_process_windows_handle_close(HANDLE *handle) {
  if (handle == NULL || *handle == NULL || *handle == INVALID_HANDLE_VALUE) {
    return;
  }

  (void)CloseHandle(*handle);
  *handle = NULL;
}

#else

static void nexus_process_posix_socket_close(int *socket_handle) {
  if (socket_handle == NULL || *socket_handle < 0) {
    return;
  }

  (void)close(*socket_handle);
  *socket_handle = -1;
}

#endif

static void n_process_child_channel_destroy(NexusProcessChildChannel *channel) {
  if (channel == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  nexus_process_windows_handle_close(&channel->read_handle);
  nexus_process_windows_handle_close(&channel->write_handle);
#else
  nexus_process_posix_socket_close(&channel->socket_handle);
#endif

  free(channel);
}

void nexus_process_child_channel_destroy(NexusProcessChildChannel *channel) {
  n_process_child_channel_destroy(channel);
}

NError nexus_process_child_channel_write(NexusProcessChildChannel *channel, const byte *bytes, uint_large byte_count) {
  uint_large written_total;

  NEXUS_ASSERT_DEBUG(channel != NULL);
  NEXUS_ASSERT_DEBUG(bytes != NULL || byte_count == 0);

  if (channel == NULL || (bytes == NULL && byte_count > 0)) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  written_total = 0;
  while (written_total < byte_count) {
#if defined(NEXUS_PLATFORM_WINDOWS)
    DWORD request_size;
    DWORD bytes_written;

    if (channel->write_handle == NULL) {
      return NEXUS_ERROR_IO;
    }

    request_size  = (DWORD)((byte_count - written_total) > (uint_large)0x7FFFFFFFUL ? (uint_large)0x7FFFFFFFUL : (byte_count - written_total));
    bytes_written = 0;
    if (WriteFile(channel->write_handle, bytes + written_total, request_size, &bytes_written, NULL) == 0 || bytes_written == 0) {
      return NEXUS_ERROR_IO;
    }
    written_total += (uint_large)bytes_written;
#else
    ssize_t bytes_written;
    int     send_flags;

    if (channel->socket_handle < 0) {
      return NEXUS_ERROR_IO;
    }

    send_flags = 0;
#  if defined(MSG_NOSIGNAL)
    send_flags = MSG_NOSIGNAL;
#  endif
    bytes_written = send(channel->socket_handle, bytes + written_total, (size_t)(byte_count - written_total), send_flags);
    if (bytes_written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return NEXUS_ERROR_IO;
    }
    if (bytes_written == 0) {
      return NEXUS_ERROR_IO;
    }
    written_total += (uint_large)bytes_written;
#endif
  }

  return NEXUS_ERROR_NONE;
}

NError nexus_process_child_channel_read(NexusProcessChildChannel *channel, byte *buffer, uint_large byte_count, uint_large *out_bytes_read,
                                        boolean *out_reached_eof) {
  NEXUS_ASSERT_DEBUG(channel != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL || byte_count == 0);
  NEXUS_ASSERT_DEBUG(out_bytes_read != NULL);
  NEXUS_ASSERT_DEBUG(out_reached_eof != NULL);

  if (channel == NULL || (buffer == NULL && byte_count > 0) || out_bytes_read == NULL || out_reached_eof == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_bytes_read  = 0;
  *out_reached_eof = FALSE;

  if (byte_count == 0) {
    return NEXUS_ERROR_NONE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    DWORD request_size;
    DWORD bytes_read;

    if (channel->read_handle == NULL) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    request_size = (DWORD)(byte_count > (uint_large)0x7FFFFFFFUL ? (uint_large)0x7FFFFFFFUL : byte_count);
    bytes_read   = 0;
    if (ReadFile(channel->read_handle, buffer, request_size, &bytes_read, NULL) == 0) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        *out_reached_eof = TRUE;
        return NEXUS_ERROR_NONE;
      }
      return NEXUS_ERROR_IO;
    }

    if (bytes_read == 0) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    *out_bytes_read = (uint_large)bytes_read;
  }
#else
  {
    ssize_t bytes_read;

    if (channel->socket_handle < 0) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    do {
      bytes_read = recv(channel->socket_handle, buffer, (size_t)byte_count, 0);
    } while (bytes_read < 0 && errno == EINTR);

    if (bytes_read < 0) {
      return NEXUS_ERROR_IO;
    }
    if (bytes_read == 0) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    *out_bytes_read = (uint_large)bytes_read;
  }
#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_process_spawn_with_child_channel(NexusPath executable_path, char *const *argv, char *const *environment,
                                              const char *child_channel_environment_name, NexusProcess **out_process,
                                              NexusProcessChildChannel **out_child_channel) {
  NexusProcess             *process;
  NexusProcessChildChannel *channel;
  char                     *channel_environment_value;
  char                    **launch_environment;

  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);
  NEXUS_ASSERT_DEBUG(child_channel_environment_name != NULL);
  NEXUS_ASSERT_DEBUG(out_process != NULL);
  NEXUS_ASSERT_DEBUG(out_child_channel != NULL);

  if (executable_path.buffer[0] == '\0' || argv == NULL || child_channel_environment_name == NULL || child_channel_environment_name[0] == '\0' ||
      out_process == NULL || out_child_channel == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_process       = NULL;
  *out_child_channel = NULL;
  process            = (NexusProcess *)malloc(NEXUS_SIZEOF(*process));
  channel            = (NexusProcessChildChannel *)malloc(NEXUS_SIZEOF(*channel));

  if (process == NULL || channel == NULL) {
    free(process);
    free(channel);
    return NEXUS_ERROR_CAPACITY;
  }

  nexus_memory_bytes_clear(process, NEXUS_SIZEOF(*process));
  nexus_memory_bytes_clear(channel, NEXUS_SIZEOF(*channel));
#if defined(NEXUS_PLATFORM_POSIX)
  process->stdin_socket  = -1;
  process->stdout_socket = -1;
  channel->socket_handle = -1;
#endif
  channel_environment_value = NULL;
  launch_environment        = NULL;

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    SECURITY_ATTRIBUTES     security_attributes;
    STARTUPINFOA            startup_info;
    PROCESS_INFORMATION     process_info;
    HANDLE                  child_read_handle;
    HANDLE                  child_write_handle;
    char                   *environment_block;
    char                    command_line[4096];
    NexusStringFormatResult format_result;
    NError                  error;

    child_read_handle  = NULL;
    child_write_handle = NULL;
    environment_block  = NULL;

    nexus_memory_bytes_clear(&security_attributes, NEXUS_SIZEOF(security_attributes));
    security_attributes.nLength        = NEXUS_SIZEOF(security_attributes);
    security_attributes.bInheritHandle = TRUE;

    if (CreatePipe(&child_read_handle, &channel->write_handle, &security_attributes, 0) == 0 ||
        SetHandleInformation(channel->write_handle, HANDLE_FLAG_INHERIT, 0) == 0 ||
        CreatePipe(&channel->read_handle, &child_write_handle, &security_attributes, 0) == 0 ||
        SetHandleInformation(channel->read_handle, HANDLE_FLAG_INHERIT, 0) == 0) {
      nexus_process_windows_handle_close(&child_read_handle);
      nexus_process_windows_handle_close(&child_write_handle);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_IO;
    }

    channel_environment_value = (char *)malloc(2U * 2U * NEXUS_SIZEOF(uint_large) + 4U);
    if (channel_environment_value == NULL) {
      nexus_process_windows_handle_close(&child_read_handle);
      nexus_process_windows_handle_close(&child_write_handle);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_CAPACITY;
    }

    format_result = nexus_strings_string_format(channel_environment_value, 2U * 2U * NEXUS_SIZEOF(uint_large) + 4U, "%llu:%llu",
                                                (unsigned long long)(uintptr_t)child_read_handle, (unsigned long long)(uintptr_t)child_write_handle);
    if (format_result.success == FALSE || format_result.truncated != FALSE) {
      free(channel_environment_value);
      nexus_process_windows_handle_close(&child_read_handle);
      nexus_process_windows_handle_close(&child_write_handle);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_CAPACITY;
    }

    launch_environment = n_process_environment_with_variable_create(environment, child_channel_environment_name, channel_environment_value);
    free(channel_environment_value);
    if (launch_environment == NULL) {
      nexus_process_windows_handle_close(&child_read_handle);
      nexus_process_windows_handle_close(&child_write_handle);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_CAPACITY;
    }

    error = nexus_process_windows_command_line_build(argv, 0, command_line, (uint_large)NEXUS_SIZEOF(command_line));
    if (error == NEXUS_ERROR_NONE) {
      error = nexus_process_windows_environment_block_create(launch_environment, &environment_block);
    }
    if (error != NEXUS_ERROR_NONE) {
      n_process_environment_with_variable_destroy(launch_environment);
      nexus_process_windows_handle_close(&child_read_handle);
      nexus_process_windows_handle_close(&child_write_handle);
      n_process_child_channel_destroy(channel);
      free(process);
      return error;
    }

    nexus_memory_bytes_clear(&startup_info, NEXUS_SIZEOF(startup_info));
    nexus_memory_bytes_clear(&process_info, NEXUS_SIZEOF(process_info));
    startup_info.cb = NEXUS_SIZEOF(startup_info);

    if (CreateProcessA(executable_path.buffer, command_line, NULL, NULL, TRUE, 0, environment_block, NULL, &startup_info, &process_info) == 0) {
      NEXUS_FREE_IF_NOT_NULL(environment_block);
      n_process_environment_with_variable_destroy(launch_environment);
      nexus_process_windows_handle_close(&child_read_handle);
      nexus_process_windows_handle_close(&child_write_handle);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_IO;
    }

    NEXUS_FREE_IF_NOT_NULL(environment_block);
    n_process_environment_with_variable_destroy(launch_environment);
    nexus_process_windows_handle_close(&child_read_handle);
    nexus_process_windows_handle_close(&child_write_handle);

    process->process_handle = process_info.hProcess;
    process->thread_handle  = process_info.hThread;
  }
#else
  {
    int                     channel_pair[2];
    pid_t                   child_process_id;
    NexusStringFormatResult format_result;

    channel_pair[0] = -1;
    channel_pair[1] = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, channel_pair) != 0) {
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_IO;
    }

    channel_environment_value = (char *)malloc(32U);
    if (channel_environment_value == NULL) {
      nexus_process_posix_socket_close(&channel_pair[0]);
      nexus_process_posix_socket_close(&channel_pair[1]);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_CAPACITY;
    }

    format_result = nexus_strings_string_format(channel_environment_value, 32U, "%d", channel_pair[1]);
    if (format_result.success == FALSE || format_result.truncated != FALSE) {
      free(channel_environment_value);
      nexus_process_posix_socket_close(&channel_pair[0]);
      nexus_process_posix_socket_close(&channel_pair[1]);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_CAPACITY;
    }

    launch_environment = n_process_environment_with_variable_create(environment, child_channel_environment_name, channel_environment_value);
    free(channel_environment_value);
    if (launch_environment == NULL) {
      nexus_process_posix_socket_close(&channel_pair[0]);
      nexus_process_posix_socket_close(&channel_pair[1]);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_CAPACITY;
    }

    child_process_id = fork();
    if (child_process_id < 0) {
      n_process_environment_with_variable_destroy(launch_environment);
      nexus_process_posix_socket_close(&channel_pair[0]);
      nexus_process_posix_socket_close(&channel_pair[1]);
      n_process_child_channel_destroy(channel);
      free(process);
      return NEXUS_ERROR_IO;
    }

    if (child_process_id == 0) {
      (void)close(channel_pair[0]);
      if (execve(executable_path.buffer, argv, launch_environment) == -1) {
        _exit(127);
      }
      _exit(127);
    }

    n_process_environment_with_variable_destroy(launch_environment);
    nexus_process_posix_socket_close(&channel_pair[1]);
    channel->socket_handle = channel_pair[0];
    process->process_id    = child_process_id;
  }
#endif

  process->running   = TRUE;
  process->waited    = FALSE;
  process->exit_code = 0;
  *out_process       = process;
  *out_child_channel = channel;

  return NEXUS_ERROR_NONE;
}

NError nexus_process_child_channel_open_from_environment(const char *environment_name, NexusProcessChildChannel **out_channel) {
  char                      channel_value[96];
  NexusProcessChildChannel *channel;
  NError                    error;

  NEXUS_ASSERT_DEBUG(environment_name != NULL);
  NEXUS_ASSERT_DEBUG(out_channel != NULL);

  if (environment_name == NULL || environment_name[0] == '\0' || out_channel == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_channel = NULL;
  error        = nexus_environment_variable_get(environment_name, channel_value, NEXUS_SIZEOF(channel_value));
  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  channel = (NexusProcessChildChannel *)malloc(NEXUS_SIZEOF(*channel));
  if (channel == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }
  nexus_memory_bytes_clear(channel, NEXUS_SIZEOF(*channel));

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    char  *separator;
    uint64 read_value;
    uint64 write_value;

    separator = channel_value;
    while (*separator != '\0' && *separator != ':') {
      separator++;
    }
    if (*separator != ':') {
      free(channel);
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }
    *separator = '\0';
    error      = nexus_strings_string_parse_uint64(channel_value, &read_value);
    if (error == NEXUS_ERROR_NONE) {
      error = nexus_strings_string_parse_uint64(separator + 1, &write_value);
    }
    if (error != NEXUS_ERROR_NONE) {
      free(channel);
      return error;
    }

    channel->read_handle  = (HANDLE)(uintptr_t)read_value;
    channel->write_handle = (HANDLE)(uintptr_t)write_value;
    if (SetHandleInformation(channel->read_handle, HANDLE_FLAG_INHERIT, 0) == 0 ||
        SetHandleInformation(channel->write_handle, HANDLE_FLAG_INHERIT, 0) == 0) {
      free(channel);
      return NEXUS_ERROR_IO;
    }
  }
#else
  {
    uint32 descriptor_value;
    int    descriptor_flags;

    error = nexus_strings_string_parse_uint32(channel_value, &descriptor_value);
    if (error != NEXUS_ERROR_NONE || descriptor_value > (uint32)INT_MAX) {
      free(channel);
      return error != NEXUS_ERROR_NONE ? error : NEXUS_ERROR_INVALID_ARGUMENT;
    }

    channel->socket_handle = (int)descriptor_value;
    descriptor_flags       = fcntl(channel->socket_handle, F_GETFD);
    if (descriptor_flags < 0 || fcntl(channel->socket_handle, F_SETFD, descriptor_flags | FD_CLOEXEC) < 0) {
      free(channel);
      return NEXUS_ERROR_IO;
    }
  }
#endif

  *out_channel = channel;
  return NEXUS_ERROR_NONE;
}

NError nexus_process_spawn_piped(NexusPath executable_path, char *const *argv, char *const *environment, NexusProcess **out_process) {
  NexusProcess *process;

  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);
  NEXUS_ASSERT_DEBUG(out_process != NULL);

  if (executable_path.buffer[0] == '\0' || argv == NULL || out_process == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_process = NULL;

  process = (NexusProcess *)malloc(NEXUS_SIZEOF(*process));

  if (process == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

  nexus_memory_bytes_clear(process, NEXUS_SIZEOF(*process));

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    SECURITY_ATTRIBUTES security_attributes;
    STARTUPINFOA        startup_info;
    PROCESS_INFORMATION process_info;
    HANDLE              child_stdin_read;
    HANDLE              child_stdout_write;
    HANDLE              parent_stdin_write;
    HANDLE              parent_stdout_read;
    char               *environment_block;
    char                command_line[4096];
    NError              error;

    child_stdin_read   = NULL;
    child_stdout_write = NULL;
    parent_stdin_write = NULL;
    parent_stdout_read = NULL;
    environment_block  = NULL;

    nexus_memory_bytes_clear(&security_attributes, NEXUS_SIZEOF(security_attributes));
    security_attributes.nLength        = NEXUS_SIZEOF(security_attributes);
    security_attributes.bInheritHandle = TRUE;

    if (CreatePipe(&child_stdin_read, &parent_stdin_write, &security_attributes, 0) == 0) {
      free(process);
      return NEXUS_ERROR_IO;
    }

    if (SetHandleInformation(parent_stdin_write, HANDLE_FLAG_INHERIT, 0) == 0) {
      nexus_process_windows_handle_close(&child_stdin_read);
      nexus_process_windows_handle_close(&parent_stdin_write);
      free(process);
      return NEXUS_ERROR_IO;
    }

    if (CreatePipe(&parent_stdout_read, &child_stdout_write, &security_attributes, 0) == 0) {
      nexus_process_windows_handle_close(&child_stdin_read);
      nexus_process_windows_handle_close(&parent_stdin_write);
      free(process);
      return NEXUS_ERROR_IO;
    }

    if (SetHandleInformation(parent_stdout_read, HANDLE_FLAG_INHERIT, 0) == 0) {
      nexus_process_windows_handle_close(&child_stdin_read);
      nexus_process_windows_handle_close(&parent_stdin_write);
      nexus_process_windows_handle_close(&parent_stdout_read);
      nexus_process_windows_handle_close(&child_stdout_write);
      free(process);
      return NEXUS_ERROR_IO;
    }

    error = nexus_process_windows_command_line_build(argv, 0, command_line, (uint_large)NEXUS_SIZEOF(command_line));

    if (error != NEXUS_ERROR_NONE) {
      nexus_process_windows_handle_close(&child_stdin_read);
      nexus_process_windows_handle_close(&parent_stdin_write);
      nexus_process_windows_handle_close(&parent_stdout_read);
      nexus_process_windows_handle_close(&child_stdout_write);
      free(process);
      return error;
    }

    error = nexus_process_windows_environment_block_create(environment, &environment_block);

    if (error != NEXUS_ERROR_NONE) {
      nexus_process_windows_handle_close(&child_stdin_read);
      nexus_process_windows_handle_close(&parent_stdin_write);
      nexus_process_windows_handle_close(&parent_stdout_read);
      nexus_process_windows_handle_close(&child_stdout_write);
      free(process);
      return error;
    }

    nexus_memory_bytes_clear(&startup_info, NEXUS_SIZEOF(startup_info));
    nexus_memory_bytes_clear(&process_info, NEXUS_SIZEOF(process_info));

    startup_info.cb         = NEXUS_SIZEOF(startup_info);
    startup_info.dwFlags    = STARTF_USESTDHANDLES;
    startup_info.hStdInput  = child_stdin_read;
    startup_info.hStdOutput = child_stdout_write;
    startup_info.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    if (CreateProcessA(executable_path.buffer, command_line, NULL, NULL, TRUE, 0, environment_block != NULL ? (LPVOID)environment_block : NULL, NULL,
                       &startup_info, &process_info) == 0) {
      NEXUS_FREE_IF_NOT_NULL(environment_block);
      nexus_process_windows_handle_close(&child_stdin_read);
      nexus_process_windows_handle_close(&parent_stdin_write);
      nexus_process_windows_handle_close(&parent_stdout_read);
      nexus_process_windows_handle_close(&child_stdout_write);
      free(process);
      return NEXUS_ERROR_IO;
    }

    NEXUS_FREE_IF_NOT_NULL(environment_block);

    nexus_process_windows_handle_close(&child_stdin_read);
    nexus_process_windows_handle_close(&child_stdout_write);

    process->process_handle = process_info.hProcess;
    process->thread_handle  = process_info.hThread;
    process->stdin_write    = parent_stdin_write;
    process->stdout_read    = parent_stdout_read;
  }
#else
  {
    int   stdin_pair[2];
    int   stdout_pair[2];
    pid_t child_process_id;

    stdin_pair[0]  = -1;
    stdin_pair[1]  = -1;
    stdout_pair[0] = -1;
    stdout_pair[1] = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pair) != 0) {
      free(process);
      return NEXUS_ERROR_IO;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pair) != 0) {
      nexus_process_posix_socket_close(&stdin_pair[0]);
      nexus_process_posix_socket_close(&stdin_pair[1]);
      free(process);
      return NEXUS_ERROR_IO;
    }

#  if defined(SO_NOSIGPIPE)
    {
      int enabled;

      enabled = 1;
      (void)setsockopt(stdin_pair[0], SOL_SOCKET, SO_NOSIGPIPE, &enabled, (socklen_t)NEXUS_SIZEOF(enabled));
    }
#  endif

    child_process_id = fork();

    if (child_process_id < 0) {
      nexus_process_posix_socket_close(&stdin_pair[0]);
      nexus_process_posix_socket_close(&stdin_pair[1]);
      nexus_process_posix_socket_close(&stdout_pair[0]);
      nexus_process_posix_socket_close(&stdout_pair[1]);
      free(process);
      return NEXUS_ERROR_IO;
    }

    if (child_process_id == 0) {
      (void)close(stdin_pair[0]);
      (void)close(stdout_pair[0]);

      if (dup2(stdin_pair[1], STDIN_FILENO) < 0 || dup2(stdout_pair[1], STDOUT_FILENO) < 0) {
        _exit(127);
      }

      if (stdin_pair[1] != STDIN_FILENO) {
        (void)close(stdin_pair[1]);
      }

      if (stdout_pair[1] != STDOUT_FILENO) {
        (void)close(stdout_pair[1]);
      }

      if (execve(executable_path.buffer, argv, environment != NULL ? environment : environ) == -1) {
        _exit(127);
      }

      _exit(127);
    }

    nexus_process_posix_socket_close(&stdin_pair[1]);
    nexus_process_posix_socket_close(&stdout_pair[1]);

    process->process_id    = child_process_id;
    process->stdin_socket  = stdin_pair[0];
    process->stdout_socket = stdout_pair[0];
  }
#endif

  process->running   = TRUE;
  process->waited    = FALSE;
  process->exit_code = 0;

  *out_process = process;

  return NEXUS_ERROR_NONE;
}

NError nexus_process_stdin_write(NexusProcess *process, const byte *bytes, uint_large byte_count) {
  uint_large written_total;

  NEXUS_ASSERT_DEBUG(process != NULL);
  NEXUS_ASSERT_DEBUG(bytes != NULL || byte_count == 0);

  if (process == NULL || (bytes == NULL && byte_count > 0)) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (byte_count == 0) {
    return NEXUS_ERROR_NONE;
  }

  written_total = 0;

#if defined(NEXUS_PLATFORM_WINDOWS)
  while (written_total < byte_count) {
    DWORD request_size;
    DWORD bytes_written;

    if (process->stdin_write == NULL) {
      return NEXUS_ERROR_IO;
    }

    request_size  = (DWORD)((byte_count - written_total) > (uint_large)0x7FFFFFFFUL ? (uint_large)0x7FFFFFFFUL : (byte_count - written_total));
    bytes_written = 0;

    if (WriteFile(process->stdin_write, bytes + written_total, request_size, &bytes_written, NULL) == 0 || bytes_written == 0) {
      return NEXUS_ERROR_IO;
    }

    written_total += (uint_large)bytes_written;
  }
#else
  while (written_total < byte_count) {
    ssize_t bytes_written;
    int     send_flags;

    if (process->stdin_socket < 0) {
      return NEXUS_ERROR_IO;
    }

    send_flags = 0;
#  if defined(MSG_NOSIGNAL)
    send_flags = MSG_NOSIGNAL;
#  endif

    bytes_written = send(process->stdin_socket, bytes + written_total, (size_t)(byte_count - written_total), send_flags);

    if (bytes_written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return NEXUS_ERROR_IO;
    }

    if (bytes_written == 0) {
      return NEXUS_ERROR_IO;
    }

    written_total += (uint_large)bytes_written;
  }
#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_process_stdin_close(NexusProcess *process) {
  NEXUS_ASSERT_DEBUG(process != NULL);

  if (process == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  nexus_process_windows_handle_close(&process->stdin_write);
#else
  nexus_process_posix_socket_close(&process->stdin_socket);
#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_process_stdout_read(NexusProcess *process, byte *buffer, uint_large byte_count, uint_large *out_bytes_read, boolean *out_reached_eof) {
  NEXUS_ASSERT_DEBUG(process != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL || byte_count == 0);
  NEXUS_ASSERT_DEBUG(out_bytes_read != NULL);
  NEXUS_ASSERT_DEBUG(out_reached_eof != NULL);

  if (process == NULL || (buffer == NULL && byte_count > 0) || out_bytes_read == NULL || out_reached_eof == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_bytes_read  = 0;
  *out_reached_eof = FALSE;

  if (byte_count == 0) {
    return NEXUS_ERROR_NONE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    DWORD request_size;
    DWORD bytes_read;
    BOOL  success;

    if (process->stdout_read == NULL) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    request_size = (DWORD)(byte_count > (uint_large)0x7FFFFFFFUL ? (uint_large)0x7FFFFFFFUL : byte_count);
    bytes_read   = 0;
    success      = ReadFile(process->stdout_read, buffer, request_size, &bytes_read, NULL);

    if (success == 0) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        *out_reached_eof = TRUE;
        return NEXUS_ERROR_NONE;
      }
      return NEXUS_ERROR_IO;
    }

    if (bytes_read == 0) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    *out_bytes_read = (uint_large)bytes_read;
  }
#else
  {
    ssize_t bytes_read;

    if (process->stdout_socket < 0) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    for (;;) {
      bytes_read = recv(process->stdout_socket, buffer, (size_t)byte_count, 0);

      if (bytes_read < 0 && errno == EINTR) {
        continue;
      }

      break;
    }

    if (bytes_read < 0) {
      return NEXUS_ERROR_IO;
    }

    if (bytes_read == 0) {
      *out_reached_eof = TRUE;
      return NEXUS_ERROR_NONE;
    }

    *out_bytes_read = (uint_large)bytes_read;
  }
#endif

  return NEXUS_ERROR_NONE;
}

boolean nexus_process_running_get(NexusProcess *process) {
  NEXUS_ASSERT_DEBUG(process != NULL);

  if (process == NULL || process->running == FALSE) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    DWORD exit_code;

    exit_code = 0;

    if (GetExitCodeProcess(process->process_handle, &exit_code) == 0) {
      return FALSE;
    }

    if (exit_code != STILL_ACTIVE) {
      process->running   = FALSE;
      process->exit_code = (int32)exit_code;
    }
  }
#else
  {
    int   wait_status;
    pid_t wait_result;

    wait_result = waitpid(process->process_id, &wait_status, WNOHANG);

    if (wait_result == process->process_id) {
      process->running = FALSE;
      process->waited  = TRUE;

      if (WIFEXITED(wait_status) != 0) {
        process->exit_code = (int32)WEXITSTATUS(wait_status);
      } else if (WIFSIGNALED(wait_status) != 0) {
        process->exit_code = 128 + (int32)WTERMSIG(wait_status);
      } else {
        process->exit_code = 1;
      }
    }
  }
#endif

  return process->running;
}

NError nexus_process_wait(NexusProcess *process, NexusProcessSpawnResult *result) {
  NEXUS_ASSERT_DEBUG(process != NULL);
  NEXUS_ASSERT_DEBUG(result != NULL);

  if (process == NULL || result == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (process->waited != FALSE) {
    result->exit_code = process->exit_code;
    return NEXUS_ERROR_NONE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    DWORD wait_result;
    DWORD exit_code;

    wait_result = WaitForSingleObject(process->process_handle, INFINITE);

    if (wait_result != WAIT_OBJECT_0) {
      return NEXUS_ERROR_IO;
    }

    exit_code = 0;

    if (GetExitCodeProcess(process->process_handle, &exit_code) == 0) {
      return NEXUS_ERROR_IO;
    }

    process->exit_code = (int32)exit_code;
  }
#else
  {
    int   wait_status;
    pid_t wait_result;

    do {
      wait_result = waitpid(process->process_id, &wait_status, 0);
    } while (wait_result < 0 && errno == EINTR);

    if (wait_result < 0) {
      return NEXUS_ERROR_IO;
    }

    if (WIFEXITED(wait_status) != 0) {
      process->exit_code = (int32)WEXITSTATUS(wait_status);
    } else if (WIFSIGNALED(wait_status) != 0) {
      process->exit_code = 128 + (int32)WTERMSIG(wait_status);
    } else {
      process->exit_code = 1;
    }
  }
#endif

  process->running  = FALSE;
  process->waited   = TRUE;
  result->exit_code = process->exit_code;

  return NEXUS_ERROR_NONE;
}

NError nexus_process_terminate(NexusProcess *process) {
  NEXUS_ASSERT_DEBUG(process != NULL);

  if (process == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (nexus_process_running_get(process) == FALSE) {
    return NEXUS_ERROR_NONE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (TerminateProcess(process->process_handle, 1) == 0) {
    return NEXUS_ERROR_IO;
  }
#else
  if (kill(process->process_id, SIGKILL) != 0 && errno != ESRCH) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

void nexus_process_destroy(NexusProcess *process) {
  NexusProcessSpawnResult result;

  if (process == NULL) {
    return;
  }

  if (nexus_process_running_get(process) != FALSE) {
    (void)nexus_process_terminate(process);
  }

  if (process->waited == FALSE) {
    (void)nexus_process_wait(process, &result);
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  nexus_process_windows_handle_close(&process->stdin_write);
  nexus_process_windows_handle_close(&process->stdout_read);
  nexus_process_windows_handle_close(&process->thread_handle);
  nexus_process_windows_handle_close(&process->process_handle);
#else
  nexus_process_posix_socket_close(&process->stdin_socket);
  nexus_process_posix_socket_close(&process->stdout_socket);
#endif

  free(process);
}

NError nexus_process_replace(NexusPath executable_path, char *const *argv, char *const *environment) {
  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (environment == NULL) {
    if (_execv(executable_path.buffer, argv) == -1) {
      return NEXUS_ERROR_IO;
    }
  } else {
    if (_execve(executable_path.buffer, argv, environment) == -1) {
      return NEXUS_ERROR_IO;
    }
  }
#else
  if (execve(executable_path.buffer, argv, environment != NULL ? environment : environ) == -1) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_IO;
}

NError nexus_process_spawn_wait(NexusPath executable_path, char *const *argv, char *const *environment, NexusProcessSpawnResult *result) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  STARTUPINFOA        startup_info;
  PROCESS_INFORMATION process_info;
  char                command_line[4096];
  DWORD               wait_result;
  DWORD               exit_code;
  NError              status;

  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);
  NEXUS_ASSERT_DEBUG(result != NULL);

  status = nexus_process_windows_command_line_build(argv, 0, command_line, (uint_large)NEXUS_SIZEOF(command_line));
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  nexus_memory_bytes_clear(&startup_info, NEXUS_SIZEOF(startup_info));
  startup_info.cb = NEXUS_SIZEOF(startup_info);

  if (CreateProcessA(executable_path.buffer, command_line, NULL, NULL, TRUE, 0, environment != NULL ? (LPVOID)environment : NULL, NULL, &startup_info,
                     &process_info) == 0) {
    return NEXUS_ERROR_IO;
  }

  wait_result = WaitForSingleObject(process_info.hProcess, INFINITE);
  if (wait_result != WAIT_OBJECT_0) {
    (void)CloseHandle(process_info.hProcess);
    (void)CloseHandle(process_info.hThread);
    return NEXUS_ERROR_IO;
  }

  if (GetExitCodeProcess(process_info.hProcess, &exit_code) == 0) {
    (void)CloseHandle(process_info.hProcess);
    (void)CloseHandle(process_info.hThread);
    return NEXUS_ERROR_IO;
  }

  (void)CloseHandle(process_info.hProcess);
  (void)CloseHandle(process_info.hThread);

  result->exit_code = (int32)exit_code;
  return NEXUS_ERROR_NONE;
#else
  pid_t child_process_id;
  int   wait_status;

  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);
  NEXUS_ASSERT_DEBUG(result != NULL);

  child_process_id = fork();
  if (child_process_id < 0) {
    return NEXUS_ERROR_IO;
  }

  if (child_process_id == 0) {
    if (execve(executable_path.buffer, argv, environment != NULL ? environment : environ) == -1) {
      _exit(127);
    }
  }

  if (waitpid(child_process_id, &wait_status, 0) < 0) {
    return NEXUS_ERROR_IO;
  }

  if (WIFEXITED(wait_status) != 0) {
    result->exit_code = (int32)WEXITSTATUS(wait_status);
    return NEXUS_ERROR_NONE;
  }

  if (WIFSIGNALED(wait_status) != 0) {
    (void)nexus_stdio_stderr_write_formatted("Child process terminated by signal %d.\n", WTERMSIG(wait_status));
  } else {
    (void)nexus_stdio_stderr_write_cstring("Child process terminated abnormally.\n");
  }

  result->exit_code = 1;
  return NEXUS_ERROR_NONE;
#endif
}

uint32 nexus_process_id_get(void) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  return (uint32)GetCurrentProcessId();
#else
  return (uint32)getpid();
#endif
}

static boolean n_process_executable_has_path_component(NexusPath executable) {
  const char *cursor;

  cursor = executable.buffer;

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (cursor[0] != '\0' && cursor[1] == ':') {
    return TRUE;
  }

  while (*cursor != '\0') {
    if (*cursor == '/' || *cursor == '\\') {
      return TRUE;
    }

    cursor++;
  }
#else
  while (*cursor != '\0') {
    if (*cursor == '/') {
      return TRUE;
    }

    cursor++;
  }
#endif

  return FALSE;
}

#if defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)

static NError n_process_posix_executable_validate(NexusPath executable) {
  struct stat file_status;

  for (;;) {
    if (stat(executable.buffer, &file_status) == 0) {
      break;
    }

    if (errno == EINTR) {
      continue;
    }

    if (errno == ENOENT || errno == ENOTDIR) {
      return NEXUS_ERROR_FILE_NOT_FOUND;
    }

    if (errno == EACCES) {
      return NEXUS_ERROR_PERMISSION_DENIED;
    }

    return nexus_errors_from_errno();
  }

  if (S_ISREG(file_status.st_mode) == 0) {
    return NEXUS_ERROR_FILE_NOT_FOUND;
  }

  for (;;) {
    if (access(executable.buffer, X_OK) == 0) {
      return NEXUS_ERROR_NONE;
    }

    if (errno == EINTR) {
      continue;
    }

    if (errno == ENOENT || errno == ENOTDIR) {
      return NEXUS_ERROR_FILE_NOT_FOUND;
    }

    if (errno == EACCES) {
      return NEXUS_ERROR_PERMISSION_DENIED;
    }

    return nexus_errors_from_errno();
  }
}

static NError n_process_posix_executable_search(NexusPath executable, NexusPath *out_resolved_executable) {
  const char *path_environment;
  const char *component_begin;
  const char *component_end;

  NexusPath candidate;

  char candidate_buffer[NEXUS_MAX_PATH_LENGTH];

  uint_large executable_length;
  uint_large component_length;
  uint_large candidate_length;
  uint_large offset;

  boolean permission_denied;

  NError error;

  path_environment = getenv("PATH");

  if (path_environment == NULL) {
    return NEXUS_ERROR_FILE_NOT_FOUND;
  }

  executable_length = nexus_strings_string_length(executable.buffer);
  permission_denied = FALSE;

  component_begin = path_environment;

  for (;;) {
    component_end = component_begin;

    while (*component_end != '\0' && *component_end != ':') {
      component_end++;
    }

    component_length = (uint_large)(component_end - component_begin);

    /*
    An empty PATH component represents the current working directory.
    */
    if (component_length == 0) {
      candidate_length = 1U + 1U + executable_length;

      if (candidate_length + 1U <= NEXUS_SIZEOF(candidate_buffer)) {
        candidate_buffer[0] = '.';
        candidate_buffer[1] = '/';

        nexus_memory_bytes_copy(candidate_buffer + 2, executable.buffer, executable_length);

        candidate_buffer[candidate_length] = '\0';

        candidate = nexus_paths_path_create(candidate_buffer);

        error = n_process_posix_executable_validate(candidate);

        if (error == NEXUS_ERROR_NONE) {
          *out_resolved_executable = nexus_paths_path_relative_to_absolute(candidate);
          return NEXUS_ERROR_NONE;
        }

        if (error == NEXUS_ERROR_PERMISSION_DENIED) {
          permission_denied = TRUE;
        } else if (error != NEXUS_ERROR_FILE_NOT_FOUND) {
          return error;
        }
      }
    } else {
      candidate_length = component_length;

      if (component_begin[component_length - 1U] != '/') {
        candidate_length++;
      }

      if (UINT_LARGE_MAX_VAL - candidate_length < executable_length) {
        return NEXUS_ERROR_CAPACITY;
      }

      candidate_length += executable_length;

      if (candidate_length + 1U <= NEXUS_SIZEOF(candidate_buffer)) {
        offset = 0;

        nexus_memory_bytes_copy(candidate_buffer, component_begin, component_length);
        offset += component_length;

        if (candidate_buffer[offset - 1U] != '/') {
          candidate_buffer[offset] = '/';
          offset++;
        }

        nexus_memory_bytes_copy(candidate_buffer + offset, executable.buffer, executable_length);
        offset += executable_length;

        candidate_buffer[offset] = '\0';

        candidate = nexus_paths_path_create(candidate_buffer);

        error = n_process_posix_executable_validate(candidate);

        if (error == NEXUS_ERROR_NONE) {
          *out_resolved_executable = nexus_paths_path_relative_to_absolute(candidate);
          return NEXUS_ERROR_NONE;
        }

        if (error == NEXUS_ERROR_PERMISSION_DENIED) {
          permission_denied = TRUE;
        } else if (error != NEXUS_ERROR_FILE_NOT_FOUND) {
          return error;
        }
      }
    }

    if (*component_end == '\0') {
      break;
    }

    component_begin = component_end + 1;
  }

  if (permission_denied != FALSE) {
    return NEXUS_ERROR_PERMISSION_DENIED;
  }

  return NEXUS_ERROR_FILE_NOT_FOUND;
}

#endif

NError nexus_process_executable_resolve(NexusPath executable, NexusPath *out_resolved_executable) {
  NexusPath resolved;

  NError error;

  NEXUS_ASSERT_DEBUG(executable.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(out_resolved_executable != NULL);

  if (executable.buffer[0] == '\0' || out_resolved_executable == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  nexus_memory_bytes_clear(out_resolved_executable, NEXUS_SIZEOF(*out_resolved_executable));

  if (n_process_executable_has_path_component(executable) != FALSE) {
    resolved = nexus_paths_path_relative_to_absolute(executable);

#if defined(NEXUS_PLATFORM_WINDOWS)
    {
      DWORD attributes;

      attributes = GetFileAttributesA(resolved.buffer);

      if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD windows_error;

        windows_error = GetLastError();

        if (windows_error == ERROR_FILE_NOT_FOUND || windows_error == ERROR_PATH_NOT_FOUND) {
          return NEXUS_ERROR_FILE_NOT_FOUND;
        }

        if (windows_error == ERROR_ACCESS_DENIED) {
          return NEXUS_ERROR_PERMISSION_DENIED;
        }

        return NEXUS_ERROR_IO;
      }

      if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return NEXUS_ERROR_FILE_NOT_FOUND;
      }
    }
#else
    error = n_process_posix_executable_validate(resolved);

    if (error != NEXUS_ERROR_NONE) {
      return error;
    }
#endif

    *out_resolved_executable = resolved;

    return NEXUS_ERROR_NONE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    char  resolved_buffer[NEXUS_MAX_PATH_LENGTH];
    DWORD resolved_length;

    resolved_length = SearchPathA(NULL, executable.buffer, ".exe", (DWORD)NEXUS_SIZEOF(resolved_buffer), resolved_buffer, NULL);

    if (resolved_length == 0) {
      DWORD windows_error;

      windows_error = GetLastError();

      if (windows_error == ERROR_FILE_NOT_FOUND || windows_error == ERROR_PATH_NOT_FOUND) {
        return NEXUS_ERROR_FILE_NOT_FOUND;
      }

      if (windows_error == ERROR_ACCESS_DENIED) {
        return NEXUS_ERROR_PERMISSION_DENIED;
      }

      return NEXUS_ERROR_IO;
    }

    if (resolved_length >= (DWORD)NEXUS_SIZEOF(resolved_buffer)) {
      return NEXUS_ERROR_CAPACITY;
    }

    resolved = nexus_paths_path_create(resolved_buffer);

    /*
    SearchPath normally returns a fully qualified path, but normalize through
    Nexus so the API guarantees an absolute result.
    */
    resolved = nexus_paths_path_relative_to_absolute(resolved);

    *out_resolved_executable = resolved;

    return NEXUS_ERROR_NONE;
  }

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)

  return n_process_posix_executable_search(executable, out_resolved_executable);

#else

  (void)error;

  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;

#endif
}
