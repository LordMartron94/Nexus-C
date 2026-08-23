#include <xxhash.h>

#include "../nexus.h"

/* Instantiate all Key-Value function definitions */
#define X_KV_DEF(key_type, val_type, suffix) NEXUS_KEY_VALUE_PAIR_DEFINE(key_type, val_type, suffix)
NEXUS_KV_TYPE_TABLE(X_KV_DEF)
#undef X_KV_DEF

/* Instantiate all Heap function definitions */
#define X_HEAP_DEF(key_type, suffix) NEXUS_DATA_HEAP_MIN_INDEX_DEFINE(key_type, suffix)
NEXUS_HEAP_TYPE_TABLE(X_HEAP_DEF)
#undef X_HEAP_DEF

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL TYPES                                                    */
/* -------------------------------------------------------------------------- */

#define NEXUS_HASHMAP_GROUP_SLOT_COUNT 8
#define NEXUS_HASHMAP_GROUP_LOAD_LIMIT 7

#define NEXUS_SWISS_EMPTY   0x80U
#define NEXUS_SWISS_DELETED 0xFEU

typedef struct HashMap {
  byte   *storage;
  byte   *data;
  uint64 *metadata;

  uint_large capacity_groups;

  uint_large element_count;
  uint_large deleted_count;

  uint_large key_size_bytes;
  uint_large value_size_bytes;

  uint64               hash_seed;
  NexusHashMapHashMode hash_mode;
} HashMap;

typedef struct HashMapHash {
  uint_large initial_group_idx;
  uint8      fingerprint;
} HashMapHash;

typedef struct HashMapProbe {
  boolean found;
  boolean insertable;

  uint_large group_idx;
  uint8      slot_idx;

  uint8 fingerprint;
} HashMapProbe;

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL HASHING                                                  */
/* -------------------------------------------------------------------------- */

static HashMapHash nexus_data_hashmap_hash_get(const HashMap *hashmap, const void *key) {
  HashMapHash result;
  uint64      hash;

  if (hashmap->hash_mode == NHMHM_PREHASHED) {
    hash = *(const uint64 *)key;
  } else {
    hash = (uint64)XXH3_64bits_withSeed(key, hashmap->key_size_bytes, hashmap->hash_seed);
  }

  result.initial_group_idx = (hash >> 7) % hashmap->capacity_groups;
  result.fingerprint       = (uint8)(hash & 0x7FU);

  return result;
}

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL PROBING                                                  */
/* -------------------------------------------------------------------------- */

static HashMapProbe nexus_data_hashmap_probe(HashMap *hashmap, const void *key) {
  HashMapHash  hash;
  HashMapProbe result;

  uint_large group_idx;
  uint8     *group_control;
  uint_large stride;
  uint_large slot_offset;
  byte      *key_pointer;

  uint8 slot_idx;
  uint8 first_empty_slot;

  boolean found_empty;

  hash = nexus_data_hashmap_hash_get(hashmap, key);

  result.found       = FALSE;
  result.insertable  = FALSE;
  result.group_idx   = 0;
  result.slot_idx    = 0;
  result.fingerprint = hash.fingerprint;

  group_idx = hash.initial_group_idx;
  stride    = hashmap->key_size_bytes + hashmap->value_size_bytes;

  for (;;) {
    group_control = (uint8 *)&hashmap->metadata[group_idx];

    /*
     * First search this group for an existing key.
     *
     * Fingerprint matching filters almost all candidates before the full
     * key comparison is required.
     */
    for (slot_idx = 0; slot_idx < NEXUS_HASHMAP_GROUP_SLOT_COUNT; slot_idx++) {
      if (group_control[slot_idx] != hash.fingerprint) {
        continue;
      }

      slot_offset = ((group_idx * NEXUS_HASHMAP_GROUP_SLOT_COUNT) + slot_idx) * stride;
      key_pointer = hashmap->data + slot_offset;

      if (nexus_memory_bytes_compare(key_pointer, key, hashmap->key_size_bytes) == 0) {
        result.found     = TRUE;
        result.group_idx = group_idx;
        result.slot_idx  = slot_idx;

        return result;
      }
    }

    /*
     * Remember the first tombstone encountered.
     *
     * We cannot terminate at a tombstone because the requested key may occur
     * farther along the probe sequence.
     */
    if (result.insertable == FALSE) {
      for (slot_idx = 0; slot_idx < NEXUS_HASHMAP_GROUP_SLOT_COUNT; slot_idx++) {
        if (group_control[slot_idx] == NEXUS_SWISS_DELETED) {
          result.insertable = TRUE;
          result.group_idx  = group_idx;
          result.slot_idx   = slot_idx;

          break;
        }
      }
    }

    /*
     * An EMPTY slot terminates the probe sequence.
     *
     * If no earlier tombstone was found, use the first empty slot as the
     * insertion target.
     */
    found_empty      = FALSE;
    first_empty_slot = 0;

    for (slot_idx = 0; slot_idx < NEXUS_HASHMAP_GROUP_SLOT_COUNT; slot_idx++) {
      if (group_control[slot_idx] == NEXUS_SWISS_EMPTY) {
        first_empty_slot = slot_idx;
        found_empty      = TRUE;

        break;
      }
    }

    if (found_empty == TRUE) {
      if (result.insertable == FALSE) {
        result.insertable = TRUE;
        result.group_idx  = group_idx;
        result.slot_idx   = first_empty_slot;
      }

      return result;
    }

    group_idx = (group_idx + 1) % hashmap->capacity_groups;

    if (group_idx == hash.initial_group_idx) {
      return result;
    }
  }
}

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL WRITING                                                  */
/* -------------------------------------------------------------------------- */

static void *nexus_data_hashmap_insert_without_resize(HashMap *hashmap, const void *key) {
  HashMapProbe probe;
  uint8       *group_control;
  uint8        previous_control;
  uint_large   stride;
  uint_large   slot_offset;
  byte        *key_pointer;
  void        *value_pointer;

  probe = nexus_data_hashmap_probe(hashmap, key);

  stride        = hashmap->key_size_bytes + hashmap->value_size_bytes;
  slot_offset   = ((probe.group_idx * NEXUS_HASHMAP_GROUP_SLOT_COUNT) + probe.slot_idx) * stride;
  key_pointer   = hashmap->data + slot_offset;
  value_pointer = key_pointer + hashmap->key_size_bytes;

  if (probe.found != FALSE) {
    return value_pointer;
  }

  NEXUS_ASSERT_MESSAGE_DEBUG(probe.insertable != FALSE, "Hashmap has no available insertion slot.");
  if (probe.insertable == FALSE) {
    return NULL;
  }

  group_control = (uint8 *)&hashmap->metadata[probe.group_idx];

  previous_control = group_control[probe.slot_idx];

  NEXUS_ASSERT_DEBUG(previous_control == NEXUS_SWISS_EMPTY || previous_control == NEXUS_SWISS_DELETED);

  if (previous_control == NEXUS_SWISS_DELETED) {
    NEXUS_ASSERT_DEBUG(hashmap->deleted_count > 0);
    hashmap->deleted_count--;
  }

  nexus_memory_bytes_copy(key_pointer, key, hashmap->key_size_bytes);

  group_control[probe.slot_idx] = probe.fingerprint;

  hashmap->element_count++;
  return value_pointer;
}

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL ITERATION                                                */
/* -------------------------------------------------------------------------- */

static boolean nexus_data_hashmap_active_entry_next(HashMap *hashmap, uint_large *group_idx, uint8 *slot_idx, void **out_key, void **out_value) {
  uint8     *group_control;
  uint8      current_slot;
  uint_large stride;
  uint_large slot_offset;
  byte      *key_pointer;

  NEXUS_ASSERT_DEBUG(hashmap != NULL);
  NEXUS_ASSERT_DEBUG(group_idx != NULL);
  NEXUS_ASSERT_DEBUG(slot_idx != NULL);

  stride = hashmap->key_size_bytes + hashmap->value_size_bytes;

  while (*group_idx < hashmap->capacity_groups) {
    group_control = (uint8 *)&hashmap->metadata[*group_idx];

    while (*slot_idx < NEXUS_HASHMAP_GROUP_SLOT_COUNT) {
      current_slot = *slot_idx;

      (*slot_idx)++;

      if (group_control[current_slot] == NEXUS_SWISS_EMPTY || group_control[current_slot] == NEXUS_SWISS_DELETED) {
        continue;
      }

      slot_offset = (((*group_idx * NEXUS_HASHMAP_GROUP_SLOT_COUNT) + current_slot) * stride);
      key_pointer = hashmap->data + slot_offset;

      if (out_key != NULL) {
        *out_key = key_pointer;
      }

      if (out_value != NULL) {
        *out_value = key_pointer + hashmap->key_size_bytes;
      }

      return TRUE;
    }

    (*group_idx)++;
    *slot_idx = 0;
  }

  return FALSE;
}

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL STORAGE                                                  */
/* -------------------------------------------------------------------------- */

static boolean nexus_data_hashmap_storage_create(HashMap *hashmap, uint_large capacity_groups) {
  uint_large entry_size;
  uint_large slot_count;
  uint_large data_size;
  uint_large metadata_size;

  NEXUS_ASSERT_DEBUG(hashmap != NULL);
  NEXUS_ASSERT_DEBUG(capacity_groups > 0);

  entry_size = hashmap->key_size_bytes + hashmap->value_size_bytes;
  slot_count = capacity_groups * NEXUS_HASHMAP_GROUP_SLOT_COUNT;

  data_size = slot_count * entry_size;

  metadata_size    = capacity_groups * NEXUS_SIZEOF(uint64);
  hashmap->storage = (byte *)malloc(metadata_size + data_size);
  if (hashmap->storage == NULL) {
    return FALSE;
  }

  hashmap->metadata = (uint64 *)hashmap->storage;
  hashmap->data     = hashmap->storage + metadata_size;

  nexus_memory_bytes_set(hashmap->metadata, NEXUS_SWISS_EMPTY, metadata_size);

  hashmap->capacity_groups = capacity_groups;
  hashmap->element_count   = 0;
  hashmap->deleted_count   = 0;

  return TRUE;
}

static boolean nexus_data_hashmap_resize(HashMap *hashmap, uint_large new_capacity_groups) {
  HashMap new_hashmap;

  uint_large group_idx;
  uint8      slot_idx;

  void *key;
  void *value;

  boolean allocation_succeeded;

  NEXUS_ASSERT_DEBUG(hashmap != NULL);
  NEXUS_ASSERT_DEBUG(new_capacity_groups > hashmap->capacity_groups);

  new_hashmap.data             = NULL;
  new_hashmap.metadata         = NULL;
  new_hashmap.storage          = NULL;
  new_hashmap.capacity_groups  = 0;
  new_hashmap.element_count    = 0;
  new_hashmap.deleted_count    = 0;
  new_hashmap.key_size_bytes   = hashmap->key_size_bytes;
  new_hashmap.value_size_bytes = hashmap->value_size_bytes;
  new_hashmap.hash_seed        = hashmap->hash_seed;

  allocation_succeeded = nexus_data_hashmap_storage_create(&new_hashmap, new_capacity_groups);

  if (allocation_succeeded == FALSE) {
    return FALSE;
  }

  group_idx = 0;
  slot_idx  = 0;

  while (nexus_data_hashmap_active_entry_next(hashmap, &group_idx, &slot_idx, &key, &value) == TRUE) {
    nexus_memory_bytes_copy(nexus_data_hashmap_insert_without_resize(&new_hashmap, key), value, new_hashmap.value_size_bytes);
  }

  NEXUS_ASSERT_DEBUG(new_hashmap.element_count == hashmap->element_count);
  NEXUS_ASSERT_DEBUG(new_hashmap.deleted_count == 0);

  free(hashmap->storage);

  hashmap->storage         = new_hashmap.storage;
  hashmap->data            = new_hashmap.data;
  hashmap->metadata        = new_hashmap.metadata;
  hashmap->capacity_groups = new_hashmap.capacity_groups;
  hashmap->element_count   = new_hashmap.element_count;
  hashmap->deleted_count   = 0;

  return TRUE;
}

/* -------------------------------------------------------------------------- */
/* HASH MAP INTERNAL COLLECTION                                               */
/* -------------------------------------------------------------------------- */

static void nexus_data_hashmap_collect(HashMap *hashmap, boolean collect_keys, boolean collect_values, void **out_keys_buffer,
                                       void **out_values_buffer, uint_large *out_count) {
  byte *keys_buffer;
  byte *values_buffer;

  uint_large key_write_offset;
  uint_large value_write_offset;

  uint_large group_idx;
  uint8      slot_idx;

  void *key;
  void *value;

  NEXUS_ASSERT_DEBUG(hashmap != NULL);
  NEXUS_ASSERT_DEBUG(out_count != NULL);

  if (collect_keys == TRUE) {
    NEXUS_ASSERT_DEBUG(out_keys_buffer != NULL);
    *out_keys_buffer = NULL;
  }

  if (collect_values == TRUE) {
    NEXUS_ASSERT_DEBUG(out_values_buffer != NULL);
    *out_values_buffer = NULL;
  }

  *out_count = hashmap->element_count;

  if (hashmap->element_count == 0) {
    return;
  }

  keys_buffer   = NULL;
  values_buffer = NULL;

  if (collect_keys == TRUE) {
    keys_buffer = (byte *)malloc(hashmap->element_count * hashmap->key_size_bytes);

    if (keys_buffer == NULL) {
      *out_count = 0;
      return;
    }
  }

  if (collect_values == TRUE) {
    values_buffer = (byte *)malloc(hashmap->element_count * hashmap->value_size_bytes);

    if (values_buffer == NULL) {
      if (keys_buffer != NULL) {
        free(keys_buffer);
      }

      *out_count = 0;
      return;
    }
  }

  key_write_offset   = 0;
  value_write_offset = 0;

  group_idx = 0;
  slot_idx  = 0;

  while (nexus_data_hashmap_active_entry_next(hashmap, &group_idx, &slot_idx, &key, &value) == TRUE) {
    if (collect_keys == TRUE) {
      nexus_memory_bytes_copy(NEXUS_MEMORY_OFFSET(keys_buffer, key_write_offset), key, hashmap->key_size_bytes);
      key_write_offset += hashmap->key_size_bytes;
    }

    if (collect_values == TRUE) {
      nexus_memory_bytes_copy(NEXUS_MEMORY_OFFSET(values_buffer, value_write_offset), value, hashmap->value_size_bytes);
      value_write_offset += hashmap->value_size_bytes;
    }
  }

  if (collect_keys == TRUE) {
    *out_keys_buffer = keys_buffer;
  }

  if (collect_values == TRUE) {
    *out_values_buffer = values_buffer;
  }
}

/* -------------------------------------------------------------------------- */
/* HASH MAP PUBLIC API                                                        */
/* -------------------------------------------------------------------------- */

NexusHashMap *nexus_data_hashmap_create(uint_large key_size_bytes, uint_large value_size_bytes, uint_large initial_capacity_groups, uint64 hash_seed,
                                        NexusHashMapHashMode mode) {
  HashMap *hashmap;
  boolean  storage_created;

  NEXUS_ASSERT_DEBUG(key_size_bytes > 0);
  NEXUS_ASSERT_DEBUG(value_size_bytes > 0);
  NEXUS_ASSERT_DEBUG(initial_capacity_groups > 0);

  hashmap = (HashMap *)malloc(NEXUS_SIZEOF(*hashmap));

  if (hashmap == NULL) {
    return NULL;
  }

  hashmap->storage          = NULL;
  hashmap->data             = NULL;
  hashmap->metadata         = NULL;
  hashmap->capacity_groups  = 0;
  hashmap->element_count    = 0;
  hashmap->deleted_count    = 0;
  hashmap->key_size_bytes   = key_size_bytes;
  hashmap->value_size_bytes = value_size_bytes;
  hashmap->hash_seed        = hash_seed;
  hashmap->hash_mode        = mode;

  storage_created = nexus_data_hashmap_storage_create(hashmap, initial_capacity_groups);

  if (storage_created == FALSE) {
    free(hashmap);

    return NULL;
  }

  return (NexusHashMap *)hashmap;
}

void nexus_data_hashmap_destroy(NexusHashMap *hashmap_handle) {
  HashMap *hashmap;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);

  hashmap = (HashMap *)hashmap_handle;

  NEXUS_FREE_IF_NOT_NULL(hashmap->storage);

  free(hashmap);
}

void nexus_data_hashmap_put(NexusHashMap *hashmap_handle, const void *key, const void *value) {
  void *stored_value;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(key != NULL);
  NEXUS_ASSERT_DEBUG(value != NULL);

  stored_value = nexus_data_hashmap_insert(hashmap_handle, key);
  NEXUS_ASSERT_DEBUG(stored_value != NULL);
  if (stored_value == NULL) {
    return;
  }

  nexus_memory_bytes_copy(stored_value, value, ((HashMap *)hashmap_handle)->value_size_bytes);
}

void *nexus_data_hashmap_insert(NexusHashMap *hashmap_handle, const void *key) {
  HashMap *hashmap;
  boolean  resize_succeeded;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(key != NULL);
  hashmap = (HashMap *)hashmap_handle;

  if (hashmap->element_count + hashmap->deleted_count >= hashmap->capacity_groups * NEXUS_HASHMAP_GROUP_LOAD_LIMIT) {
    resize_succeeded = nexus_data_hashmap_resize(hashmap, hashmap->capacity_groups * 2);

    NEXUS_ASSERT_MESSAGE_DEBUG(resize_succeeded == TRUE, "Failed to resize hashmap.");

    if (resize_succeeded == FALSE) {
      return NULL;
    }
  }

  return nexus_data_hashmap_insert_without_resize(hashmap, key);
}

void *nexus_data_hashmap_get(NexusHashMap *hashmap_handle, const void *key) {
  HashMap     *hashmap;
  HashMapProbe probe;
  uint_large   stride;
  uint_large   slot_offset;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(key != NULL);
  hashmap = (HashMap *)hashmap_handle;

  probe = nexus_data_hashmap_probe(hashmap, key);

  if (probe.found == FALSE) {
    return NULL;
  }

  stride      = hashmap->key_size_bytes + hashmap->value_size_bytes;
  slot_offset = ((probe.group_idx * NEXUS_HASHMAP_GROUP_SLOT_COUNT) + probe.slot_idx) * stride;
  return hashmap->data + slot_offset + hashmap->key_size_bytes;
}

boolean nexus_data_hashmap_get_copy(NexusHashMap *hashmap_handle, const void *key, void *out_value) {
  HashMap *hashmap;
  void    *stored_value;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(key != NULL);
  NEXUS_ASSERT_DEBUG(out_value != NULL);

  hashmap      = (HashMap *)hashmap_handle;
  stored_value = nexus_data_hashmap_get(hashmap_handle, key);

  if (stored_value == NULL) {
    return FALSE;
  }

  nexus_memory_bytes_copy(out_value, stored_value, hashmap->value_size_bytes);
  return TRUE;
}

boolean nexus_data_hashmap_delete(NexusHashMap *hashmap_handle, const void *key) {
  HashMap     *hashmap;
  HashMapProbe probe;
  uint8       *group_control;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(key != NULL);

  hashmap = (HashMap *)hashmap_handle;

  probe = nexus_data_hashmap_probe(hashmap, key);

  if (probe.found == FALSE) {
    return FALSE;
  }

  group_control = (uint8 *)&hashmap->metadata[probe.group_idx];

  group_control[probe.slot_idx] = NEXUS_SWISS_DELETED;

  NEXUS_ASSERT_DEBUG(hashmap->element_count > 0);

  hashmap->element_count--;
  hashmap->deleted_count++;

  return TRUE;
}

/* -------------------------------------------------------------------------- */
/* HASH MAP ENUMERATION                                                       */
/* -------------------------------------------------------------------------- */

void nexus_data_hashmap_enumerator_init(NexusHashMapEnumerator *enumerator) {
  NEXUS_ASSERT_DEBUG(enumerator != NULL);

  enumerator->group_idx = 0;
  enumerator->slot_idx  = 0;
}

boolean nexus_data_hashmap_enumerator_next(NexusHashMapEnumerator *enumerator, void **out_key, void **out_value) {
  HashMap *hashmap;

  NEXUS_ASSERT_DEBUG(enumerator != NULL);
  NEXUS_ASSERT_DEBUG(enumerator->hashmap != NULL);
  NEXUS_ASSERT_DEBUG(out_key != NULL);
  NEXUS_ASSERT_DEBUG(out_value != NULL);

  hashmap = (HashMap *)enumerator->hashmap;

  return nexus_data_hashmap_active_entry_next(hashmap, &enumerator->group_idx, &enumerator->slot_idx, out_key, out_value);
}

/* -------------------------------------------------------------------------- */
/* HASH MAP COLLECTION                                                        */
/* -------------------------------------------------------------------------- */

boolean nexus_data_hashmap_get_keys_allocated(NexusHashMap *hashmap_handle, void **out_keys_buffer, uint_large *out_count) {
  HashMap *hashmap;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(out_keys_buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_count != NULL);

  hashmap = (HashMap *)hashmap_handle;

  nexus_data_hashmap_collect(hashmap, TRUE, FALSE, out_keys_buffer, NULL, out_count);
  return (boolean)(*out_count >= 1);
}

boolean nexus_data_hashmap_get_values_allocated(NexusHashMap *hashmap_handle, void **out_values_buffer, uint_large *out_count) {
  HashMap *hashmap;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(out_values_buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_count != NULL);

  hashmap = (HashMap *)hashmap_handle;

  nexus_data_hashmap_collect(hashmap, FALSE, TRUE, NULL, out_values_buffer, out_count);
  return (boolean)(*out_count >= 1);
}

boolean nexus_data_hashmap_get_entries_allocated(NexusHashMap *hashmap_handle, void **out_keys_buffer, void **out_values_buffer,
                                                 uint_large *out_count) {
  HashMap *hashmap;

  NEXUS_ASSERT_DEBUG(hashmap_handle != NULL);
  NEXUS_ASSERT_DEBUG(out_keys_buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_values_buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_count != NULL);

  hashmap = (HashMap *)hashmap_handle;

  nexus_data_hashmap_collect(hashmap, TRUE, TRUE, out_keys_buffer, out_values_buffer, out_count);
  return (boolean)(*out_count >= 1);
}

boolean nexus_data_array_reserve(void **array, uint32 *capacity, uint32 required_count, uint_large element_size) {
  void  *resized;
  uint32 new_capacity;

  NEXUS_ASSERT_DEBUG(array != NULL);
  NEXUS_ASSERT_DEBUG(capacity != NULL);
  NEXUS_ASSERT_DEBUG(element_size > 0U);

  if (array == NULL || capacity == NULL || element_size == 0U) {
    return FALSE;
  }

  if (required_count <= *capacity) {
    return TRUE;
  }

  new_capacity = *capacity == 0U ? 4U : *capacity;

  while (new_capacity < required_count) {
    if (new_capacity > UINT32_MAX_VAL / 2U) {
      new_capacity = required_count;
      break;
    }

    new_capacity *= 2U;
  }

  if ((uint_large)new_capacity > UINT_LARGE_MAX_VAL / element_size) {
    return FALSE;
  }

  if (*array == NULL) {
    resized = malloc((uint_large)new_capacity * element_size);
  } else {
    resized = realloc(*array, (uint_large)new_capacity * element_size);
  }

  if (resized == NULL) {
    return FALSE;
  }

  *array    = resized;
  *capacity = new_capacity;

  return TRUE;
}
