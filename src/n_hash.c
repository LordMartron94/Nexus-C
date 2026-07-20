#include "../nexus.h"

/* FNV-1a 64-bit: offset basis and prime (public domain parameters). */
#define N_HASH_FNV1A64_OFFSET_BASIS 14695981039346656037ULL
#define N_HASH_FNV1A64_PRIME        1099511628211ULL

void nexus_hash_zobrist_table_fill(NexusHashZobristTable *table, NexusHashEntropyFillCallback *fill, void *user_data) {
  uint64 i;
  byte   key_bytes[8];

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(fill != NULL);
  NEXUS_ASSERT_DEBUG(table->key_count == 0 || table->keys != NULL);

  for (i = 0; i < table->key_count; i++) {
    fill(user_data, key_bytes, 8);

    table->keys[i] = ((uint64)key_bytes[0]) | ((uint64)key_bytes[1] << 8) | ((uint64)key_bytes[2] << 16) | ((uint64)key_bytes[3] << 24) |
                     ((uint64)key_bytes[4] << 32) | ((uint64)key_bytes[5] << 40) | ((uint64)key_bytes[6] << 48) | ((uint64)key_bytes[7] << 56);
  }
}

uint64 nexus_hash_zobrist_key_get(const NexusHashZobristTable *table, uint64 index) {
  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->keys != NULL);
  NEXUS_ASSERT_MESSAGE_DEBUG(index < table->key_count, "Zobrist key index out of range.");

  return table->keys[index];
}

uint64 nexus_hash_zobrist_hash_xor(uint64 hash, uint64 key) {
  return hash ^ key;
}

uint64 nexus_hash_zobrist_hash_from_keys(const uint64 *keys, uint64 key_count) {
  uint64 hash;
  uint64 i;

  NEXUS_ASSERT_DEBUG(key_count == 0 || keys != NULL);

  hash = 0;
  for (i = 0; i < key_count; i++) {
    hash = nexus_hash_zobrist_hash_xor(hash, keys[i]);
  }

  return hash;
}

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
