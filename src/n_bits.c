#include "../nexus.h"

static uint16 n_internal_uint16_from_bytes_lsb(const byte *bytes) {
  return (uint16)((uint16)bytes[0] | ((uint16)bytes[1] << 8));
}

static uint32 n_internal_uint32_from_bytes_lsb(const byte *bytes) {
  return (uint32)bytes[0] | ((uint32)bytes[1] << 8) | ((uint32)bytes[2] << 16) | ((uint32)bytes[3] << 24);
}

static uint64 n_internal_uint64_from_bytes_lsb(const byte *bytes) {
  return (uint64)bytes[0] | ((uint64)bytes[1] << 8) | ((uint64)bytes[2] << 16) | ((uint64)bytes[3] << 24) | ((uint64)bytes[4] << 32) |
         ((uint64)bytes[5] << 40) | ((uint64)bytes[6] << 48) | ((uint64)bytes[7] << 56);
}

static uint16 n_internal_uint16_from_bytes_msb(const byte *bytes) {
  return (uint16)(((uint16)bytes[0] << 8) | (uint16)bytes[1]);
}

static uint32 n_internal_uint32_from_bytes_msb(const byte *bytes) {
  return ((uint32)bytes[0] << 24) | ((uint32)bytes[1] << 16) | ((uint32)bytes[2] << 8) | (uint32)bytes[3];
}

static uint64 n_internal_uint64_from_bytes_msb(const byte *bytes) {
  return ((uint64)bytes[0] << 56) | ((uint64)bytes[1] << 48) | ((uint64)bytes[2] << 40) | ((uint64)bytes[3] << 32) | ((uint64)bytes[4] << 24) |
         ((uint64)bytes[5] << 16) | ((uint64)bytes[6] << 8) | (uint64)bytes[7];
}

static real32 n_internal_real32_from_bits(uint32 bits) {
  union {
    uint32 bit_pattern;
    real32 value;
  } converter;

  converter.bit_pattern = bits;
  return converter.value;
}

static real64 n_internal_real64_from_bits(uint64 bits) {
  union {
    uint64 bit_pattern;
    real64 value;
  } converter;

  converter.bit_pattern = bits;
  return converter.value;
}

uint16 nexus_bits_uint16_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_uint16_from_bytes_lsb(bytes);
}

uint32 nexus_bits_uint32_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_uint32_from_bytes_lsb(bytes);
}

uint64 nexus_bits_uint64_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_uint64_from_bytes_lsb(bytes);
}

int16 nexus_bits_int16_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (int16)n_internal_uint16_from_bytes_lsb(bytes);
}

int32 nexus_bits_int32_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (int32)n_internal_uint32_from_bytes_lsb(bytes);
}

int64 nexus_bits_int64_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (int64)n_internal_uint64_from_bytes_lsb(bytes);
}

real32 nexus_bits_real32_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_real32_from_bits(n_internal_uint32_from_bytes_lsb(bytes));
}

real64 nexus_bits_real64_from_bytes_lsb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_real64_from_bits(n_internal_uint64_from_bytes_lsb(bytes));
}

uint16 nexus_bits_uint16_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_uint16_from_bytes_msb(bytes);
}

uint32 nexus_bits_uint32_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_uint32_from_bytes_msb(bytes);
}

uint64 nexus_bits_uint64_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_uint64_from_bytes_msb(bytes);
}

int16 nexus_bits_int16_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (int16)n_internal_uint16_from_bytes_msb(bytes);
}

int32 nexus_bits_int32_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (int32)n_internal_uint32_from_bytes_msb(bytes);
}

int64 nexus_bits_int64_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (int64)n_internal_uint64_from_bytes_msb(bytes);
}

real32 nexus_bits_real32_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_real32_from_bits(n_internal_uint32_from_bytes_msb(bytes));
}

real64 nexus_bits_real64_from_bytes_msb(const byte *bytes) {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return n_internal_real64_from_bits(n_internal_uint64_from_bytes_msb(bytes));
}

uint32 nexus_bits_uint32_from_real32(real32 value) {
  union {
    real32     value;
    uint32 bit_pattern;
  } converter;

  converter.value = value;
  return converter.bit_pattern;
}

real32 nexus_bits_real32_from_uint32(uint32 bits) {
  return n_internal_real32_from_bits(bits);
}

uint64 nexus_bits_uint64_from_real64(real64 value) {
  union {
    real64     value;
    uint64 bit_pattern;
  } converter;

  converter.value = value;
  return converter.bit_pattern;
}

real64 nexus_bits_real64_from_uint64(uint64 bits) {
  return n_internal_real64_from_bits(bits);
}

uint32 nexus_bits_uint32_from_f_real(f_real value) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (uint32)nexus_bits_uint64_from_real64((real64)value);
#else
  return nexus_bits_uint32_from_real32((real32)value);
#endif
}

uint64 nexus_bits_uint64_from_f_real(f_real value) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_bits_uint64_from_real64((real64)value);
#else
  return (uint64)nexus_bits_uint32_from_real32((real32)value);
#endif
}

f_real nexus_bits_f_real_from_uint32(uint32 bits) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (f_real)nexus_bits_real64_from_uint64((uint64)bits);
#else
  return (f_real)nexus_bits_real32_from_uint32(bits);
#endif
}

f_real nexus_bits_f_real_from_uint64(uint64 bits) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (f_real)nexus_bits_real64_from_uint64(bits);
#else
  return (f_real)nexus_bits_real32_from_uint32((uint32)bits);
#endif
}
