#include <errno.h>
#include <stdio.h>
#include "../nexus.h"
#include "./n_internal.h"

static const char *n_internal_nexus_error_message_for_code(uint16 code) {
  switch (code) {
  case 1:
    return "file not found";
  case 2:
    return "permission denied";
  case 3:
    return "already exists";
  case 4:
    return "directory not empty";
  case 5:
    return "disk full";
  case 6:
    return "invalid argument";
  case 7:
    return "I/O error";
  case 8:
    return "unsupported architecture for operation";
  default:
    return "unknown Nexus error";
  }
}

static boolean n_internal_error_is_nexus(NError error) {
  return NEXUS_ERROR_FACILITY_BYTE_1(error) == 'N' && NEXUS_ERROR_FACILITY_BYTE_2(error) == 'X' ? TRUE : FALSE;
}

NError nexus_errors_from_errno(void) {
  switch (errno) {
  case ENOENT:
    return NEXUS_ERROR_FILE_NOT_FOUND;
  case EACCES:
  case EPERM:
    return NEXUS_ERROR_PERMISSION_DENIED;
  case EEXIST:
    return NEXUS_ERROR_ALREADY_EXISTS;
  case ENOSPC:
    return NEXUS_ERROR_DISK_FULL;
  case ENOTEMPTY:
    return NEXUS_ERROR_DIR_NOT_EMPTY;
  case EINVAL:
    return NEXUS_ERROR_INVALID_ARGUMENT;
  default:
    return NEXUS_ERROR_IO;
  }
}

#if NEXUS_PLATFORM_WINDOWS
NError nexus_errors_from_windows_error(unsigned long win32_error) {
  switch (win32_error) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
    return NEXUS_ERROR_FILE_NOT_FOUND;
  case ERROR_ACCESS_DENIED:
    return NEXUS_ERROR_PERMISSION_DENIED;
  case ERROR_ALREADY_EXISTS:
    return NEXUS_ERROR_ALREADY_EXISTS;
  case ERROR_DISK_FULL:
    return NEXUS_ERROR_DISK_FULL;
  case ERROR_DIR_NOT_EMPTY:
    return NEXUS_ERROR_DIR_NOT_EMPTY;
  default:
    return NEXUS_ERROR_IO;
  }
}
#endif

uint_large nexus_errors_message_write(NError error, char *buffer, uint_large buffer_max_length, const char *prefix) {
  const char *message;
  boolean     use_prefix;

  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(buffer_max_length > 0);

  if (error == NEXUS_ERROR_NONE) {
    buffer[0] = '\0';
    return 0;
  }

  use_prefix = (prefix != NULL && prefix[0] != '\0') ? TRUE : FALSE;

  if (n_internal_error_is_nexus(error)) {
    message = n_internal_nexus_error_message_for_code(NEXUS_ERROR_CODE(error));
    if (use_prefix) {
      nexus_strings_string_format_with_truncation(buffer, buffer_max_length, "%s: %s", prefix, message);
    } else {
      nexus_strings_string_copy(buffer, buffer_max_length, message);
    }
    return nexus_strings_string_length(buffer);
  }

  if (use_prefix) {
    nexus_strings_string_format_with_truncation(buffer, buffer_max_length, "%s: Error %c%c-%u", prefix, NEXUS_ERROR_FACILITY_BYTE_1(error),
                                                NEXUS_ERROR_FACILITY_BYTE_2(error), (unsigned int)NEXUS_ERROR_CODE(error));
  } else {
    nexus_strings_string_format_with_truncation(buffer, buffer_max_length, "Error %c%c-%u", NEXUS_ERROR_FACILITY_BYTE_1(error),
                                                NEXUS_ERROR_FACILITY_BYTE_2(error), (unsigned int)NEXUS_ERROR_CODE(error));
  }
  return nexus_strings_string_length(buffer);
}
