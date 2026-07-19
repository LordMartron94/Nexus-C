#include "../nexus.h"

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
