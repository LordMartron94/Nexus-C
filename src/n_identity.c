#include <stddef.h>
#include <string.h>

#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD) || defined(NEXUS_PLATFORM_IOS)
#  include <pthread.h>
#else

#  error "nexus_identity requires a supported threading platform implementation"

#endif

/* ---------------------------------------------------------------------------- */
/* CONSTANTS                                                                    */
/* ---------------------------------------------------------------------------- */

#define N_IDENTITY_UUID_BYTE_COUNT 16

#define N_IDENTITY_UUID_VERSION_MASK 0x0Fu
#define N_IDENTITY_UUID_VARIANT_MASK 0x3Fu

#define N_IDENTITY_UUID_VERSION_4   0x40u
#define N_IDENTITY_UUID_VERSION_5   0x50u
#define N_IDENTITY_UUID_VERSION_7   0x70u
#define N_IDENTITY_UUID_VARIANT_RFC 0x80u

#define N_IDENTITY_UUID_V7_TIMESTAMP_MAX 0xFFFFFFFFFFFFULL
#define N_IDENTITY_UUID_V7_FRACTION_MAX  0x0FFFu
#define N_IDENTITY_UUID_V7_COUNTER_MAX   0x0FFFu

/* ---------------------------------------------------------------------------- */
/* UUID INTERNAL                                                                */
/* ---------------------------------------------------------------------------- */

static boolean n_identity_ascii_whitespace_get(char character) {
  return character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '\f' || character == '\v';
}

static boolean n_identity_hex_nibble_get(char character, byte *out_value) {
  NEXUS_ASSERT_DEBUG(out_value != NULL);

  if (character >= '0' && character <= '9') {
    *out_value = (byte)(character - '0');
    return TRUE;
  }

  if (character >= 'a' && character <= 'f') {
    *out_value = (byte)(character - 'a' + 10);
    return TRUE;
  }

  if (character >= 'A' && character <= 'F') {
    *out_value = (byte)(character - 'A' + 10);
    return TRUE;
  }

  return FALSE;
}

static void n_identity_uuid_timestamp_write(byte uuid[N_IDENTITY_UUID_BYTE_COUNT], uint64 unix_milliseconds) {
  uuid[0] = (byte)(unix_milliseconds >> 40u);
  uuid[1] = (byte)(unix_milliseconds >> 32u);
  uuid[2] = (byte)(unix_milliseconds >> 24u);
  uuid[3] = (byte)(unix_milliseconds >> 16u);
  uuid[4] = (byte)(unix_milliseconds >> 8u);
  uuid[5] = (byte)(unix_milliseconds);
}

static NError n_identity_uuid_real_time_get(uint64 *out_unix_milliseconds, uint16 *out_fraction) {
  NexusTime now;

  uint64 milliseconds;
  uint64 sub_millisecond_nanoseconds;

  uint16 fraction;

  NEXUS_ASSERT_DEBUG(out_unix_milliseconds != NULL);

  now = nexus_time_get_real();

  NEXUS_ASSERT_DEBUG(now.clock_origin == NCO_REAL);

  milliseconds = now.time / NEXUS_NANOSECONDS_PER_MILLISECOND;

  if (milliseconds > N_IDENTITY_UUID_V7_TIMESTAMP_MAX) {
    return NEXUS_ERROR_CAPACITY;
  }

  fraction = 0;

  if (out_fraction != NULL && now.precision < NTP_MILLISECOND) {
    sub_millisecond_nanoseconds = now.time % NEXUS_NANOSECONDS_PER_MILLISECOND;

    fraction = (uint16)((sub_millisecond_nanoseconds * 4096ULL) / NEXUS_NANOSECONDS_PER_MILLISECOND);
  }

  *out_unix_milliseconds = milliseconds;

  if (out_fraction != NULL) {
    *out_fraction = fraction;
  }

  return NEXUS_ERROR_NONE;
}

/* ---------------------------------------------------------------------------- */
/* UUID STRING                                                                  */
/* ---------------------------------------------------------------------------- */

void nexus_identity_uuid_string_write(NexusUUID uuid, char out_string[NEXUS_UUID_STRING_BUFFER_SIZE]) {
  static const char hexadecimal[] = "0123456789abcdef";

  byte bytes[N_IDENTITY_UUID_BYTE_COUNT];

  uint32 byte_index;
  uint32 string_index;

  NEXUS_ASSERT_DEBUG(out_string != NULL);

  nexus_uint128_bytes_big_endian_write(uuid, bytes);

  string_index = 0;

  for (byte_index = 0; byte_index < N_IDENTITY_UUID_BYTE_COUNT; byte_index++) {
    if (byte_index == 4 || byte_index == 6 || byte_index == 8 || byte_index == 10) {
      out_string[string_index] = '-';
      string_index++;
    }

    out_string[string_index] = hexadecimal[(bytes[byte_index] >> 4u) & 0x0Fu];
    string_index++;

    out_string[string_index] = hexadecimal[bytes[byte_index] & 0x0Fu];
    string_index++;
  }

  out_string[string_index] = '\0';

  NEXUS_ASSERT_DEBUG(string_index == NEXUS_UUID_STRING_LENGTH);
}

NError nexus_identity_uuid_string_parse(const char *string, NexusUUID *out_uuid) {
  const char *begin;
  const char *end;

  byte bytes[N_IDENTITY_UUID_BYTE_COUNT];

  byte high;
  byte low;

  uint_large string_length;
  uint_large normalized_length;

  uint32 string_index;
  uint32 byte_index;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(out_uuid != NULL);

  if (string == NULL || out_uuid == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  string_length = nexus_strings_string_length(string);

  begin = string;
  end   = string + string_length;

  while (begin < end && n_identity_ascii_whitespace_get(*begin) != FALSE) {
    begin++;
  }

  while (end > begin && n_identity_ascii_whitespace_get(*(end - 1)) != FALSE) {
    end--;
  }

  normalized_length = (uint_large)(end - begin);

  if (normalized_length != NEXUS_UUID_STRING_LENGTH) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (begin[8] != '-' || begin[13] != '-' || begin[18] != '-' || begin[23] != '-') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  string_index = 0;
  byte_index   = 0;

  while (string_index < NEXUS_UUID_STRING_LENGTH) {
    if (string_index == 8 || string_index == 13 || string_index == 18 || string_index == 23) {
      string_index++;
      continue;
    }

    if (n_identity_hex_nibble_get(begin[string_index], &high) == FALSE) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }

    if (n_identity_hex_nibble_get(begin[string_index + 1], &low) == FALSE) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }

    bytes[byte_index] = (byte)((high << 4u) | low);

    byte_index++;
    string_index += 2;
  }

  if (byte_index != N_IDENTITY_UUID_BYTE_COUNT) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_uuid = nexus_uint128_bytes_big_endian_read(bytes);

  return NEXUS_ERROR_NONE;
}

/* ---------------------------------------------------------------------------- */
/* UUID VERSION 4                                                               */
/* ---------------------------------------------------------------------------- */

NError nexus_identity_uuid_v4_generate_with_entropy(NexusEntropyFillFunc *entropy_fill, void *entropy_user_data, NexusUUID *out_uuid) {
  byte uuid[16];

  NError error;

  NEXUS_ASSERT_DEBUG(entropy_fill != NULL);
  NEXUS_ASSERT_DEBUG(out_uuid != NULL);

  if (entropy_fill == NULL || out_uuid == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  error = entropy_fill(entropy_user_data, uuid, NEXUS_SIZEOF(uuid));

  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  uuid[6] = (byte)((uuid[6] & N_IDENTITY_UUID_VERSION_MASK) | N_IDENTITY_UUID_VERSION_4);
  uuid[8] = (byte)((uuid[8] & N_IDENTITY_UUID_VARIANT_MASK) | N_IDENTITY_UUID_VARIANT_RFC);

  *out_uuid = nexus_uint128_bytes_big_endian_read(uuid);

  return NEXUS_ERROR_NONE;
}

NError nexus_identity_uuid_v4_generate(NexusUUID *out_uuid) {
  return nexus_identity_uuid_v4_generate_with_entropy(nexus_entropy_system_fill, NULL, out_uuid);
}

/* ---------------------------------------------------------------------------- */
/* UUID VERSION 5                                                               */
/* ---------------------------------------------------------------------------- */

NError nexus_identity_uuid_v5_generate_bytes(NexusUUID namespace_uuid, const void *name, uint64 name_size, NexusUUID *out_uuid) {
  NexusHashSHA1Context hash;

  byte namespace_bytes[16];
  byte digest[NEXUS_HASH_SHA1_DIGEST_SIZE];
  byte uuid[16];

  NEXUS_ASSERT_DEBUG(name != NULL || name_size == 0);
  NEXUS_ASSERT_DEBUG(out_uuid != NULL);

  if ((name == NULL && name_size != 0) || out_uuid == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  nexus_uint128_bytes_big_endian_write(namespace_uuid, namespace_bytes);

  nexus_hash_sha1_begin(&hash);

  nexus_hash_sha1_bytes(&hash, namespace_bytes, NEXUS_SIZEOF(namespace_bytes));

  nexus_hash_sha1_bytes(&hash, name, name_size);

  nexus_hash_sha1_end(&hash, digest);

  nexus_memory_bytes_copy(uuid, digest, NEXUS_SIZEOF(uuid));

  /*
  UUIDv5 uses the most-significant 128 bits of the SHA-1 digest.
  */
  uuid[6] = (byte)((uuid[6] & N_IDENTITY_UUID_VERSION_MASK) | N_IDENTITY_UUID_VERSION_5);
  uuid[8] = (byte)((uuid[8] & N_IDENTITY_UUID_VARIANT_MASK) | N_IDENTITY_UUID_VARIANT_RFC);

  *out_uuid = nexus_uint128_bytes_big_endian_read(uuid);

  return NEXUS_ERROR_NONE;
}

NError nexus_identity_uuid_v5_generate(NexusUUID namespace_uuid, const char *name, NexusUUID *out_uuid) {
  uint_large name_length;

  NEXUS_ASSERT_DEBUG(name != NULL);
  NEXUS_ASSERT_DEBUG(out_uuid != NULL);

  if (name == NULL || out_uuid == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  name_length = nexus_strings_string_length(name);

  return nexus_identity_uuid_v5_generate_bytes(namespace_uuid, name, name_length, out_uuid);
}

/* ---------------------------------------------------------------------------- */
/* UUID VERSION 7 - RANDOM                                                      */
/* ---------------------------------------------------------------------------- */

NError nexus_identity_uuid_v7_generate_random_with_entropy(NexusEntropyFillFunc *entropy_fill, void *entropy_user_data, NexusUUID *out_uuid) {
  byte uuid[16];

  uint64 unix_milliseconds;

  NError error;

  NEXUS_ASSERT_DEBUG(entropy_fill != NULL);
  NEXUS_ASSERT_DEBUG(out_uuid != NULL);

  if (entropy_fill == NULL || out_uuid == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  error = n_identity_uuid_real_time_get(&unix_milliseconds, NULL);

  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  /*
  Bytes 0-5 are timestamp bytes, so only bytes 6-15 need entropy.
  */
  error = entropy_fill(entropy_user_data, &uuid[6], 10);

  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  n_identity_uuid_timestamp_write(uuid, unix_milliseconds);

  uuid[6] = (byte)((uuid[6] & N_IDENTITY_UUID_VERSION_MASK) | N_IDENTITY_UUID_VERSION_7);
  uuid[8] = (byte)((uuid[8] & N_IDENTITY_UUID_VARIANT_MASK) | N_IDENTITY_UUID_VARIANT_RFC);

  *out_uuid = nexus_uint128_bytes_big_endian_read(uuid);

  return NEXUS_ERROR_NONE;
}

NError nexus_identity_uuid_v7_generate_random(NexusUUID *out_uuid) {
  return nexus_identity_uuid_v7_generate_random_with_entropy(nexus_entropy_system_fill, NULL, out_uuid);
}

/* ---------------------------------------------------------------------------- */
/* UUID VERSION 7 - MONOTONIC STATE                                             */
/* ---------------------------------------------------------------------------- */

typedef struct NIdentityUUIDv7MonotonicState {
  uint64 unix_milliseconds;

  uint16 fraction;
  uint16 counter;

  boolean initialized;
} NIdentityUUIDv7MonotonicState;

static NIdentityUUIDv7MonotonicState n_identity_uuid_v7_monotonic_state = {0, 0, 0, FALSE};

#if defined(NEXUS_PLATFORM_WINDOWS)

static SRWLOCK n_identity_uuid_v7_monotonic_mutex = SRWLOCK_INIT;

static void n_identity_uuid_v7_monotonic_lock(void) {
  AcquireSRWLockExclusive(&n_identity_uuid_v7_monotonic_mutex);
}

static void n_identity_uuid_v7_monotonic_unlock(void) {
  ReleaseSRWLockExclusive(&n_identity_uuid_v7_monotonic_mutex);
}

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)

static pthread_mutex_t n_identity_uuid_v7_monotonic_mutex = PTHREAD_MUTEX_INITIALIZER;

static void n_identity_uuid_v7_monotonic_lock(void) {
  int result;

  result = pthread_mutex_lock(&n_identity_uuid_v7_monotonic_mutex);

  NEXUS_ASSERT(result == 0);
}

static void n_identity_uuid_v7_monotonic_unlock(void) {
  int result;

  result = pthread_mutex_unlock(&n_identity_uuid_v7_monotonic_mutex);

  NEXUS_ASSERT(result == 0);
}

#endif

static boolean n_identity_uuid_v7_monotonic_increment(void) {
  if (n_identity_uuid_v7_monotonic_state.counter < N_IDENTITY_UUID_V7_COUNTER_MAX) {
    n_identity_uuid_v7_monotonic_state.counter++;

    return TRUE;
  }

  n_identity_uuid_v7_monotonic_state.counter = 0;

  if (n_identity_uuid_v7_monotonic_state.fraction < N_IDENTITY_UUID_V7_FRACTION_MAX) {
    n_identity_uuid_v7_monotonic_state.fraction++;

    return TRUE;
  }

  n_identity_uuid_v7_monotonic_state.fraction = 0;

  if (n_identity_uuid_v7_monotonic_state.unix_milliseconds >= N_IDENTITY_UUID_V7_TIMESTAMP_MAX) {
    return FALSE;
  }

  n_identity_uuid_v7_monotonic_state.unix_milliseconds++;

  return TRUE;
}

/* ---------------------------------------------------------------------------- */
/* UUID VERSION 7 - MONOTONIC                                                   */
/* ---------------------------------------------------------------------------- */

NError nexus_identity_uuid_v7_generate_monotonic_with_entropy(NexusEntropyFillFunc *entropy_fill, void *entropy_user_data, NexusUUID *out_uuid) {
  byte uuid[N_IDENTITY_UUID_BYTE_COUNT];
  byte random_bytes[7];

  uint64 actual_milliseconds;
  uint64 output_milliseconds;

  uint16 actual_fraction;
  uint16 output_fraction;
  uint16 output_counter;

  NError error;

  boolean increment_succeeded;

  NEXUS_ASSERT_DEBUG(entropy_fill != NULL);
  NEXUS_ASSERT_DEBUG(out_uuid != NULL);

  if (entropy_fill == NULL || out_uuid == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  /*
  Generate entropy before entering the monotonic-state lock. The random suffix
  does not determine monotonic ordering because timestamp/fraction/counter
  precede it.

  When this function is called concurrently with the same custom entropy
  provider, that provider is responsible for synchronizing its own state.
  nexus_entropy_system_fill is safe for concurrent use.
  */
  error = entropy_fill(entropy_user_data, random_bytes, NEXUS_SIZEOF(random_bytes));

  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  error = n_identity_uuid_real_time_get(&actual_milliseconds, &actual_fraction);

  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  n_identity_uuid_v7_monotonic_lock();

  increment_succeeded = TRUE;

  if (n_identity_uuid_v7_monotonic_state.initialized == FALSE) {
    n_identity_uuid_v7_monotonic_state.unix_milliseconds = actual_milliseconds;

    n_identity_uuid_v7_monotonic_state.fraction    = actual_fraction;
    n_identity_uuid_v7_monotonic_state.counter     = 0;
    n_identity_uuid_v7_monotonic_state.initialized = TRUE;
  } else if (actual_milliseconds > n_identity_uuid_v7_monotonic_state.unix_milliseconds) {
    /*
    Actual wall time has advanced into a new millisecond.
    */
    n_identity_uuid_v7_monotonic_state.unix_milliseconds = actual_milliseconds;

    n_identity_uuid_v7_monotonic_state.fraction = actual_fraction;
    n_identity_uuid_v7_monotonic_state.counter  = 0;
  } else if (actual_milliseconds == n_identity_uuid_v7_monotonic_state.unix_milliseconds &&
             actual_fraction > n_identity_uuid_v7_monotonic_state.fraction) {
    /*
    We remain in the same millisecond but the sub-millisecond clock fraction
    advanced.
    */
    n_identity_uuid_v7_monotonic_state.fraction = actual_fraction;
    n_identity_uuid_v7_monotonic_state.counter  = 0;
  } else {
    /*
    The physical clock has either:
      - remained in the same fraction bucket;
      - moved backwards within the millisecond; or
      - moved backwards to an earlier millisecond.

    Preserve the logical clock and advance its counter instead.
    */
    increment_succeeded = n_identity_uuid_v7_monotonic_increment();
  }

  output_milliseconds = n_identity_uuid_v7_monotonic_state.unix_milliseconds;
  output_fraction     = n_identity_uuid_v7_monotonic_state.fraction;
  output_counter      = n_identity_uuid_v7_monotonic_state.counter;

  n_identity_uuid_v7_monotonic_unlock();

  if (increment_succeeded == FALSE) {
    return NEXUS_ERROR_CAPACITY;
  }

  /*
  0-47: Unix timestamp.
  */
  n_identity_uuid_timestamp_write(uuid, output_milliseconds);

  /*
  48-51: version 7
  52-63: complete 12-bit sub-millisecond fraction
  */
  uuid[6] = (byte)(N_IDENTITY_UUID_VERSION_7 | ((output_fraction >> 8u) & 0x0Fu));
  uuid[7] = (byte)(output_fraction & 0xFFu);

  /*
  64-65: RFC variant
  66-77: complete 12-bit monotonic counter

      octet 8: 10cccccc
      octet 9: ccccccRR

  The two low bits of octet 9 begin the random suffix.
  */
  uuid[8] = (byte)(N_IDENTITY_UUID_VARIANT_RFC | ((output_counter >> 6u) & 0x3Fu));

  uuid[9] = (byte)(((output_counter & 0x3Fu) << 2u) | (random_bytes[0] & 0x03u));

  /*
  Remaining 48 random bits.

  Together with the two random bits in octet 9 this produces a 50-bit random
  suffix.
  */
  uuid[10] = random_bytes[1];
  uuid[11] = random_bytes[2];
  uuid[12] = random_bytes[3];
  uuid[13] = random_bytes[4];
  uuid[14] = random_bytes[5];
  uuid[15] = random_bytes[6];

  *out_uuid = nexus_uint128_bytes_big_endian_read(uuid);

  return NEXUS_ERROR_NONE;
}

NError nexus_identity_uuid_v7_generate_monotonic(NexusUUID *out_uuid) {
  return nexus_identity_uuid_v7_generate_monotonic_with_entropy(nexus_entropy_system_fill, NULL, out_uuid);
}