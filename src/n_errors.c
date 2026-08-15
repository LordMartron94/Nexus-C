#include <errno.h>
#include <stdio.h>
#include "../nexus.h"
#include "./n_internal.h"

typedef struct NexusErrorFacilityFormatterEntry {
  char                        facility_byte_1;
  char                        facility_byte_2;
  NexusErrorMessageFormatter *formatter;
} NexusErrorFacilityFormatterEntry;

static NexusErrorFacilityFormatterEntry n_error_facility_formatters[NEXUS_ERROR_MESSAGE_FORMATTER_CAPACITY];
static uint_large                       n_error_facility_formatter_count = 0;

static const char *n_internal_nexus_error_message_for_code(uint16 code) {
  switch (code) {
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_FILE_NOT_FOUND):
    return "file not found";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_PERMISSION_DENIED):
    return "permission denied";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_ALREADY_EXISTS):
    return "already exists";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_DIR_NOT_EMPTY):
    return "directory not empty";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_DISK_FULL):
    return "disk full";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_INVALID_ARGUMENT):
    return "invalid argument";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_IO):
    return "I/O error";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE):
    return "unsupported architecture for operation";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_CAPACITY):
    return "capacity exhausted";
  case NEXUS_ERROR_LOCAL_ID(NEXUS_ERROR_INTERRUPTED):
    return "interrupted process";
  default:
    return "unknown Nexus error";
  }
}

static boolean n_internal_error_is_nexus(NError error) {
  return NEXUS_ERROR_FACILITY_BYTE_1(error) == 'N' && NEXUS_ERROR_FACILITY_BYTE_2(error) == 'X' ? TRUE : FALSE;
}

static boolean n_internal_error_facility_is_nexus(char facility_byte_1, char facility_byte_2) {
  return facility_byte_1 == 'N' && facility_byte_2 == 'X' ? TRUE : FALSE;
}

static NexusErrorFacilityFormatterEntry *n_internal_error_facility_formatter_find(char facility_byte_1, char facility_byte_2) {
  uint_large index;

  for (index = 0; index < n_error_facility_formatter_count; index++) {
    if (n_error_facility_formatters[index].facility_byte_1 == facility_byte_1 &&
        n_error_facility_formatters[index].facility_byte_2 == facility_byte_2) {
      return &n_error_facility_formatters[index];
    }
  }

  return NULL;
}

static NexusErrorMessageFormatter *n_internal_error_facility_formatter_lookup(char facility_byte_1, char facility_byte_2) {
  NexusErrorFacilityFormatterEntry *entry;

  entry = n_internal_error_facility_formatter_find(facility_byte_1, facility_byte_2);
  if (entry == NULL) {
    return NULL;
  }

  return entry->formatter;
}

static uint_large n_internal_error_message_write_resolved(const char *message, char *buffer, uint_large buffer_max_length, const char *prefix,
                                                          boolean use_prefix) {
  if (use_prefix) {
    nexus_strings_string_format_with_truncation(buffer, buffer_max_length, "%s: %s", prefix, message);
  } else {
    nexus_strings_string_copy(buffer, buffer_max_length, message);
  }
  return nexus_strings_string_length(buffer);
}

static uint_large n_internal_error_message_write_generic(NError error, char *buffer, uint_large buffer_max_length, const char *prefix,
                                                         boolean use_prefix) {
  if (use_prefix) {
    nexus_strings_string_format_with_truncation(buffer, buffer_max_length, "%s: Error %c%c-%u", prefix, NEXUS_ERROR_FACILITY_BYTE_1(error),
                                                NEXUS_ERROR_FACILITY_BYTE_2(error), (unsigned int)NEXUS_ERROR_CODE(error));
  } else {
    nexus_strings_string_format_with_truncation(buffer, buffer_max_length, "Error %c%c-%u", NEXUS_ERROR_FACILITY_BYTE_1(error),
                                                NEXUS_ERROR_FACILITY_BYTE_2(error), (unsigned int)NEXUS_ERROR_CODE(error));
  }
  return nexus_strings_string_length(buffer);
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

NError nexus_errors_message_formatter_register(char facility_byte_1, char facility_byte_2, NexusErrorMessageFormatter *formatter) {
  NexusErrorFacilityFormatterEntry *entry;

  if (formatter == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (n_internal_error_facility_is_nexus(facility_byte_1, facility_byte_2)) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  entry = n_internal_error_facility_formatter_find(facility_byte_1, facility_byte_2);
  if (entry != NULL) {
    entry->formatter = formatter;
    return NEXUS_ERROR_NONE;
  }

  if (n_error_facility_formatter_count >= NEXUS_ERROR_MESSAGE_FORMATTER_CAPACITY) {
    return NEXUS_ERROR_CAPACITY;
  }

  n_error_facility_formatters[n_error_facility_formatter_count].facility_byte_1 = facility_byte_1;
  n_error_facility_formatters[n_error_facility_formatter_count].facility_byte_2 = facility_byte_2;
  n_error_facility_formatters[n_error_facility_formatter_count].formatter       = formatter;
  n_error_facility_formatter_count++;

  return NEXUS_ERROR_NONE;
}

void nexus_errors_message_formatter_unregister(char facility_byte_1, char facility_byte_2) {
  uint_large index;
  uint_large last_index;

  for (index = 0; index < n_error_facility_formatter_count; index++) {
    if (n_error_facility_formatters[index].facility_byte_1 != facility_byte_1 ||
        n_error_facility_formatters[index].facility_byte_2 != facility_byte_2) {
      continue;
    }

    last_index = n_error_facility_formatter_count - 1;
    if (index != last_index) {
      n_error_facility_formatters[index] = n_error_facility_formatters[last_index];
    }
    n_error_facility_formatters[last_index].facility_byte_1 = '\0';
    n_error_facility_formatters[last_index].facility_byte_2 = '\0';
    n_error_facility_formatters[last_index].formatter       = NULL;
    n_error_facility_formatter_count--;
    return;
  }
}

uint_large nexus_errors_message_write(NError error, char *buffer, uint_large buffer_max_length, const char *prefix) {
  const char                 *message;
  boolean                     use_prefix;
  NexusErrorMessageFormatter *formatter;

  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(buffer_max_length > 0);

  if (error == NEXUS_ERROR_NONE) {
    buffer[0] = '\0';
    return 0;
  }

  use_prefix = (prefix != NULL && prefix[0] != '\0') ? TRUE : FALSE;

  if (n_internal_error_is_nexus(error)) {
    message = n_internal_nexus_error_message_for_code(NEXUS_ERROR_CODE(error));
    return n_internal_error_message_write_resolved(message, buffer, buffer_max_length, prefix, use_prefix);
  }

  formatter = n_internal_error_facility_formatter_lookup(NEXUS_ERROR_FACILITY_BYTE_1(error), NEXUS_ERROR_FACILITY_BYTE_2(error));
  if (formatter != NULL) {
    message = formatter(NEXUS_ERROR_CODE(error));
    if (message != NULL) {
      return n_internal_error_message_write_resolved(message, buffer, buffer_max_length, prefix, use_prefix);
    }
  }

  return n_internal_error_message_write_generic(error, buffer, buffer_max_length, prefix, use_prefix);
}
