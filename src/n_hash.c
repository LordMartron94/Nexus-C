#include <stddef.h>

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

/* ---------------------------------------------------------------------------- */
/* SHA-1                                                                        */
/* ---------------------------------------------------------------------------- */

static uint32 nexus_hash_sha1_rotate_left(uint32 value, uint32 amount) {
  return (value << amount) | (value >> (32u - amount));
}

static uint32 nexus_hash_sha1_uint32_be_read(const byte *bytes) {
  return ((uint32)bytes[0] << 24u) | ((uint32)bytes[1] << 16u) | ((uint32)bytes[2] << 8u) | ((uint32)bytes[3]);
}

static void nexus_hash_sha1_block_process(NexusHashSHA1Context *context, const byte block[NEXUS_HASH_SHA1_BLOCK_SIZE]) {
  uint32 words[80];

  uint32 a; /* NOLINT(readability-identifier-length) */
  uint32 b; /* NOLINT(readability-identifier-length) */
  uint32 c; /* NOLINT(readability-identifier-length) */
  uint32 d; /* NOLINT(readability-identifier-length) */
  uint32 e; /* NOLINT(readability-identifier-length) */

  uint32 f; /* NOLINT(readability-identifier-length) */
  uint32 k;
  uint32 temporary;

  uint32 i;

  for (i = 0; i < 16; i++) {
    words[i] = nexus_hash_sha1_uint32_be_read(&block[(size_t)i * 4]);
  }

  for (i = 16; i < 80; i++) {
    words[i] = nexus_hash_sha1_rotate_left(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
  }

  a = context->state[0];
  b = context->state[1];
  c = context->state[2];
  d = context->state[3];
  e = context->state[4];

  for (i = 0; i < 80; i++) {
    if (i < 20) {
      f = (b & c) | ((~b) & d);
      k = 0x5A827999u;
    } else if (i < 40) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1u;
    } else if (i < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDCu;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6u;
    }

    temporary = nexus_hash_sha1_rotate_left(a, 5) + f + e + k + words[i];

    e = d;
    d = c;
    c = nexus_hash_sha1_rotate_left(b, 30);
    b = a;
    a = temporary;
  }

  context->state[0] += a;
  context->state[1] += b;
  context->state[2] += c;
  context->state[3] += d;
  context->state[4] += e;
}

void nexus_hash_sha1_begin(NexusHashSHA1Context *context) {
  NEXUS_ASSERT_DEBUG(context != NULL);

  if (context == NULL) {
    return;
  }

  context->state[0] = 0x67452301u;
  context->state[1] = 0xEFCDAB89u;
  context->state[2] = 0x98BADCFEu;
  context->state[3] = 0x10325476u;
  context->state[4] = 0xC3D2E1F0u;

  context->total_byte_count = 0;
  context->block_byte_count = 0;
}

void nexus_hash_sha1_bytes(NexusHashSHA1Context *context, const void *data, uint64 byte_count) {
  const byte *bytes;

  uint64 remaining;
  uint64 copy_count;

  NEXUS_ASSERT_DEBUG(context != NULL);
  NEXUS_ASSERT_DEBUG(data != NULL || byte_count == 0);

  if (context == NULL || (data == NULL && byte_count != 0)) {
    return;
  }

  if (byte_count == 0) {
    return;
  }

  bytes     = (const byte *)data;
  remaining = byte_count;

  context->total_byte_count += byte_count;

  while (remaining > 0) {
    copy_count = (uint64)NEXUS_HASH_SHA1_BLOCK_SIZE - context->block_byte_count;

    if (copy_count > remaining) {
      copy_count = remaining;
    }

    nexus_memory_bytes_copy(&context->block[context->block_byte_count], bytes, copy_count);

    context->block_byte_count += (uint32)copy_count;

    bytes += copy_count;
    remaining -= copy_count;

    if (context->block_byte_count == NEXUS_HASH_SHA1_BLOCK_SIZE) {
      nexus_hash_sha1_block_process(context, context->block);
      context->block_byte_count = 0;
    }
  }
}

void nexus_hash_sha1_end(NexusHashSHA1Context *context, byte out_digest[NEXUS_HASH_SHA1_DIGEST_SIZE]) {
  uint64 bit_length;

  uint32 i;

  NEXUS_ASSERT_DEBUG(context != NULL);
  NEXUS_ASSERT_DEBUG(out_digest != NULL);

  if (context == NULL || out_digest == NULL) {
    return;
  }

  bit_length = context->total_byte_count * 8ULL;

  context->block[context->block_byte_count++] = 0x80u;

  if (context->block_byte_count > 56) {
    while (context->block_byte_count < NEXUS_HASH_SHA1_BLOCK_SIZE) {
      context->block[context->block_byte_count++] = 0;
    }

    nexus_hash_sha1_block_process(context, context->block);

    context->block_byte_count = 0;
  }

  while (context->block_byte_count < 56) {
    context->block[context->block_byte_count++] = 0;
  }

  for (i = 0; i < 8; i++) {
    context->block[56 + i] = (byte)(bit_length >> ((7u - i) * 8u));
  }

  nexus_hash_sha1_block_process(context, context->block);

  for (i = 0; i < 5; i++) {
    out_digest[(i * 4) + 0] = (byte)(context->state[i] >> 24u);
    out_digest[(i * 4) + 1] = (byte)(context->state[i] >> 16u);
    out_digest[(i * 4) + 2] = (byte)(context->state[i] >> 8u);
    out_digest[(i * 4) + 3] = (byte)context->state[i];
  }
}

void nexus_hash_sha1(const void *data, uint64 byte_count, byte out_digest[NEXUS_HASH_SHA1_DIGEST_SIZE]) {
  NexusHashSHA1Context context;

  NEXUS_ASSERT_DEBUG(data != NULL || byte_count == 0);
  NEXUS_ASSERT_DEBUG(out_digest != NULL);

  if ((data == NULL && byte_count != 0) || out_digest == NULL) {
    return;
  }

  nexus_hash_sha1_begin(&context);
  nexus_hash_sha1_bytes(&context, data, byte_count);
  nexus_hash_sha1_end(&context, out_digest);
}