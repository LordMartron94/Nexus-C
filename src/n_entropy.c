#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <bcrypt.h>

#elif defined(NEXUS_PLATFORM_LINUX) || defined(NEXUS_PLATFORM_ANDROID)

#  include <errno.h>
#  include <sys/random.h>

#elif defined(NEXUS_PLATFORM_MACOS) || defined(NEXUS_PLATFORM_IOS) || defined(NEXUS_PLATFORM_BSD)

#  include <stdlib.h>

#endif

NError nexus_entropy_system_fill(void *user_data, byte *out_bytes, uint64 byte_count) {
  uint64  offset;
  ssize_t result;
  (void)user_data;

  NEXUS_ASSERT_DEBUG(out_bytes != NULL || byte_count == 0);

  if (out_bytes == NULL && byte_count != 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (byte_count == 0) {
    return NEXUS_ERROR_NONE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  ULONG    chunk_size;
  NTSTATUS status;

  offset = 0;

  while (offset < byte_count) {
    if (byte_count - offset > 0xFFFFFFFFULL) {
      chunk_size = 0xFFFFFFFFUL;
    } else {
      chunk_size = (ULONG)(byte_count - offset);
    }

    status = BCryptGenRandom(NULL, (PUCHAR)&out_bytes[offset], chunk_size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (status < 0) {
      return NEXUS_ERROR_IO;
    }

    offset += chunk_size;
  }

  return NEXUS_ERROR_NONE;

#elif defined(NEXUS_PLATFORM_LINUX) || defined(NEXUS_PLATFORM_ANDROID)
  offset = 0;

  while (offset < byte_count) {
    result = getrandom(&out_bytes[offset], (size_t)(byte_count - offset), 0);

    if (result < 0) {
      if (errno == EINTR) {
        continue;
      }

      return NEXUS_ERROR_IO;
    }

    if (result == 0) {
      return NEXUS_ERROR_IO;
    }

    offset += (uint64)result;
  }

  return NEXUS_ERROR_NONE;

#elif defined(NEXUS_PLATFORM_MACOS) || defined(NEXUS_PLATFORM_IOS) || defined(NEXUS_PLATFORM_BSD)

  arc4random_buf(out_bytes, (size_t)byte_count);

  return NEXUS_ERROR_NONE;

#else

  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;

#endif
}