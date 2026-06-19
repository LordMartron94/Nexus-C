#include "../nexus.h"
#include <string.h>

void nexus_memory_bytes_copy(void *dest, const void *src, uint_large byte_count) {
  if (byte_count == 0) {
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(src != NULL);

  memcpy(dest, src, (size_t)byte_count);
}
