#include "../nexus.h"

/* FNV-1a 64-bit: offset basis and prime (public domain parameters). */
#define N_HASH_FNV1A64_OFFSET_BASIS 14695981039346656037ULL
#define N_HASH_FNV1A64_PRIME        1099511628211ULL

uint64 nexus_hash_fnv1a64_begin(void) {
  return N_HASH_FNV1A64_OFFSET_BASIS;
}

uint64 nexus_hash_fnv1a64_byte(uint64 hash, uint8 value) {
  hash ^= (uint64)value;
  hash *= N_HASH_FNV1A64_PRIME;
  return hash;
}

uint64 nexus_hash_fnv1a64_bytes(uint64 hash, const void *data, uint64 byte_count) {
  const uint8 *bytes;
  uint64       index;

  NEXUS_ASSERT_DEBUG(byte_count == 0u || data != NULL);

  bytes = (const uint8 *)data;
  for (index = 0u; index < byte_count; index++) {
    hash = nexus_hash_fnv1a64_byte(hash, bytes[index]);
  }
  return hash;
}

uint64 nexus_hash_fnv1a64(const void *data, uint64 byte_count) {
  return nexus_hash_fnv1a64_bytes(nexus_hash_fnv1a64_begin(), data, byte_count);
}
