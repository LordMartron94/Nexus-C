#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <process.h>
#  include <stdlib.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <unistd.h>

extern char **environ;
#else
#  error "Unsupported platform"
#endif

NError nexus_process_replace(NexusPath executable_path, char *const *argv, char *const *environment) {
  NEXUS_ASSERT_DEBUG(executable_path.buffer[0] != '\0');
  NEXUS_ASSERT_DEBUG(argv != NULL);

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (_execve(executable_path.buffer, (char *const *)argv, (char *const *)environment) == -1) {
    return NEXUS_ERROR_IO;
  }
#else
  if (execve(executable_path.buffer, (char *const *)argv, environment != NULL ? (char *const *)environment : environ) == -1) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_IO;
}
