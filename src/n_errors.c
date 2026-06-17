#include <errno.h>
#include <string.h>
#include "../nexus.h"
#include "./n_internal.h"

static NexusErrorCode n_last_error_code = NEXUS_ERROR_NONE;

void nexus_errors_record_code(NexusErrorCode code) {
  n_last_error_code = code;
}

void nexus_errors_record_errno(void) {
  nexus_errors_record_code((NexusErrorCode)errno);
}

#if NEXUS_PLATFORM_WINDOWS
void nexus_errors_record_windows_error(unsigned long win32_error) {
  int32 code;

  switch (win32_error) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    code = ENOENT;
    break;
  case ERROR_ACCESS_DENIED:
    code = EACCES;
    break;
  case ERROR_ALREADY_EXISTS:
    code = EEXIST;
    break;
  case ERROR_DISK_FULL:
    code = ENOSPC;
    break;
  case ERROR_DIR_NOT_EMPTY:
    code = ENOTEMPTY;
    break;
  default:
    code = EIO;
    break;
  }

  nexus_errors_record_code(code);
}
#endif

void nexus_errors_clear(void) {
  n_last_error_code = NEXUS_ERROR_NONE;
}

boolean nexus_errors_occurred(void) {
  return n_last_error_code != NEXUS_ERROR_NONE ? TRUE : FALSE;
}

const char *nexus_errors_message_get(void) {
  if (n_last_error_code == NEXUS_ERROR_NONE)
    return "";

  return strerror(n_last_error_code);
}

uint_large nexus_errors_message_write(char *buffer, uint_large buffer_max_length) {
  const char *message;

  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(buffer_max_length > 0);

  message = nexus_errors_message_get();
  nexus_strings_string_copy(buffer, buffer_max_length, message);
  return nexus_strings_string_length(buffer);
}
