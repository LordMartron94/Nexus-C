#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <windows.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <stdlib.h>
#else
#  error "Unsupported platform"
#endif

NError nexus_environment_variable_set(const char *name, const char *value) {
  NEXUS_ASSERT_DEBUG(name != NULL);
  NEXUS_ASSERT_DEBUG(value != NULL);

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (SetEnvironmentVariableA(name, value) == 0) {
    return NEXUS_ERROR_IO;
  }
  return NEXUS_ERROR_NONE;
#elif defined(NEXUS_PLATFORM_POSIX)
  if (setenv(name, value, 1) != 0) {
    return NEXUS_ERROR_IO;
  }
  return NEXUS_ERROR_NONE;
#else
#  error "Unsupported platform"
#endif
}

NError nexus_environment_variable_get(const char *name, char *buffer, uint_large buffer_max_length) {
  NexusStringFormatResult copy_result;
  const char             *value;

  NEXUS_ASSERT_DEBUG(name != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_MESSAGE_DEBUG(buffer_max_length > 0, "environment buffer must be non-zero");

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    DWORD copied_length;

    copied_length = GetEnvironmentVariableA(name, buffer, (DWORD)buffer_max_length);
    if (copied_length == 0) {
      return NEXUS_ERROR_FILE_NOT_FOUND;
    }
    if (copied_length >= (DWORD)buffer_max_length) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }
    return NEXUS_ERROR_NONE;
  }
#elif defined(NEXUS_PLATFORM_POSIX)
  value = getenv(name);
  if (value == NULL) {
    return NEXUS_ERROR_FILE_NOT_FOUND;
  }

  copy_result = nexus_strings_string_copy_with_truncation(buffer, buffer_max_length, value);
  if (copy_result.truncated == TRUE) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  return NEXUS_ERROR_NONE;
#else
#  error "Unsupported platform"
#endif
}
