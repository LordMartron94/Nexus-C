#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <process.h>
#  include <windows.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  include <errno.h>
#  include <signal.h>
#  include <sys/socket.h>

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
