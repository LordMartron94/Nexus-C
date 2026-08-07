#include "../nexus.h"

/* Instantiate all Key-Value function definitions */
#define X_KV_DEF(key_type, val_type, suffix) NEXUS_KEY_VALUE_PAIR_DEFINE(key_type, val_type, suffix)
NEXUS_KV_TYPE_TABLE(X_KV_DEF)
#undef X_KV_DEF

/* Instantiate all Heap function definitions */
#define X_HEAP_DEF(key_type, suffix) NEXUS_DATA_HEAP_MIN_INDEX_DEFINE(key_type, suffix)
NEXUS_HEAP_TYPE_TABLE(X_HEAP_DEF)
#undef X_HEAP_DEF