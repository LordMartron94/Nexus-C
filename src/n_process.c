#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <process.h>
#  include <stdlib.h>
#  include <string.h>
#  include <windows.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>

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

NError nexus_process_replace(NexusPath executable_path, char *const *argv, char *const *environment) {
  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (_execve(executable_path.buffer, argv, environment) == -1) {
    return NEXUS_ERROR_IO;
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

  status = nexus_process_windows_command_line_build(argv, 1, command_line, (uint_large)NEXUS_SIZEOF(command_line));
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  memset(&startup_info, 0, NEXUS_SIZEOF(startup_info));
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
