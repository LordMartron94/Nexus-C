#include "../nexus.h"

#include <math.h>

static void n_real64_assert_finite(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value == value, "cannot round NaN");
  NEXUS_ASSERT_MESSAGE_DEBUG(value < REAL64_MAX_VAL && value > -REAL64_MAX_VAL, "cannot round infinity");
}

static real64 n_real64_round_even(real64 value) {
  real64 integral;
  real64 fraction;

  fraction = modf(value, &integral);

  if (fraction > 0.5) {
    return integral + 1.0;
  }
  if (fraction < -0.5) {
    return integral - 1.0;
  }
  if (fraction > -0.5 && fraction < 0.5) {
    return integral;
  }

  if (fmod(integral, 2.0) == 0.0) {
    return integral;
  }

  if (value > 0.0) {
    return integral + 1.0;
  }

  return integral - 1.0;
}

static real64 n_real64_round_mode(real64 value, NexusRealRoundMode mode) {
  n_real64_assert_finite(value);

  switch (mode) {
  case NRRM_FLOOR:
    return floor(value);
  case NRRM_CEIL:
    return ceil(value);
  case NRRM_TRUNC:
    if (value >= 0.0) {
      return floor(value);
    }
    return ceil(value);
  case NRRM_ROUND:
    if (value >= 0.0) {
      return floor(value + 0.5);
    }
    return ceil(value - 0.5);
  case NRRM_ROUND_EVEN:
    return n_real64_round_even(value);
  default:
    NEXUS_ASSERT_MESSAGE_DEBUG(FALSE, "invalid NexusRealRoundMode");
    return value;
  }
}

static void n_real64_assert_in_int32_range(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value >= (real64)INT32_MIN_VAL && value <= (real64)INT32_MAX_VAL, "rounded value is outside int32 range");
}

static void n_real64_assert_in_int64_range(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value >= (real64)INT64_MIN_VAL && value <= (real64)INT64_MAX_VAL, "rounded value is outside int64 range");
}

static void n_real64_assert_in_uint32_range(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value >= 0.0 && value <= (real64)UINT32_MAX_VAL, "rounded value is outside uint32 range");
}

static void n_real64_assert_in_uint64_range(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value >= 0.0 && value <= (real64)UINT64_MAX_VAL, "rounded value is outside uint64 range");
}

static void n_real64_assert_in_int_large_range(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value >= (real64)INT_LARGE_MIN_VAL && value <= (real64)INT_LARGE_MAX_VAL, "rounded value is outside int_large range");
}

static void n_real64_assert_in_uint_large_range(real64 value) {
  NEXUS_ASSERT_MESSAGE_DEBUG(value >= 0.0 && value <= (real64)UINT_LARGE_MAX_VAL, "rounded value is outside uint_large range");
}

real32 nexus_real32_round(real32 value, NexusRealRoundMode mode) {
  return (real32)n_real64_round_mode((real64)value, mode);
}

real64 nexus_real64_round(real64 value, NexusRealRoundMode mode) {
  return n_real64_round_mode(value, mode);
}

f_real nexus_real_round(f_real value, NexusRealRoundMode mode) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (f_real)nexus_real64_round((real64)value, mode);
#else
  return (f_real)nexus_real32_round((real32)value, mode);
#endif
}

int32 nexus_real32_round_to_int32(real32 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = (real64)nexus_real32_round(value, mode);
  n_real64_assert_in_int32_range(rounded);
  return (int32)rounded;
}

int64 nexus_real32_round_to_int64(real32 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = (real64)nexus_real32_round(value, mode);
  n_real64_assert_in_int64_range(rounded);
  return (int64)rounded;
}

uint32 nexus_real32_round_to_uint32(real32 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = (real64)nexus_real32_round(value, mode);
  n_real64_assert_in_uint32_range(rounded);
  return (uint32)rounded;
}

uint64 nexus_real32_round_to_uint64(real32 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = (real64)nexus_real32_round(value, mode);
  n_real64_assert_in_uint64_range(rounded);
  return (uint64)rounded;
}

int32 nexus_real64_round_to_int32(real64 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = nexus_real64_round(value, mode);
  n_real64_assert_in_int32_range(rounded);
  return (int32)rounded;
}

int64 nexus_real64_round_to_int64(real64 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = nexus_real64_round(value, mode);
  n_real64_assert_in_int64_range(rounded);
  return (int64)rounded;
}

uint32 nexus_real64_round_to_uint32(real64 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = nexus_real64_round(value, mode);
  n_real64_assert_in_uint32_range(rounded);
  return (uint32)rounded;
}

uint64 nexus_real64_round_to_uint64(real64 value, NexusRealRoundMode mode) {
  real64 rounded;

  rounded = nexus_real64_round(value, mode);
  n_real64_assert_in_uint64_range(rounded);
  return (uint64)rounded;
}

int32 nexus_real_round_to_int32(f_real value, NexusRealRoundMode mode) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_real64_round_to_int32((real64)value, mode);
#else
  return nexus_real32_round_to_int32((real32)value, mode);
#endif
}

int64 nexus_real_round_to_int64(f_real value, NexusRealRoundMode mode) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_real64_round_to_int64((real64)value, mode);
#else
  return nexus_real32_round_to_int64((real32)value, mode);
#endif
}

uint32 nexus_real_round_to_uint32(f_real value, NexusRealRoundMode mode) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_real64_round_to_uint32((real64)value, mode);
#else
  return nexus_real32_round_to_uint32((real32)value, mode);
#endif
}

uint64 nexus_real_round_to_uint64(f_real value, NexusRealRoundMode mode) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_real64_round_to_uint64((real64)value, mode);
#else
  return nexus_real32_round_to_uint64((real32)value, mode);
#endif
}

int_large nexus_real_round_to_int_large(f_real value, NexusRealRoundMode mode) {
  real64 rounded;

#if NEXUS_FLOAT_DOUBLE_PRECISION
  rounded = nexus_real64_round((real64)value, mode);
#else
  rounded = (real64)nexus_real32_round((real32)value, mode);
#endif

  n_real64_assert_in_int_large_range(rounded);
  return (int_large)rounded;
}

uint_large nexus_real_round_to_uint_large(f_real value, NexusRealRoundMode mode) {
  real64 rounded;

#if NEXUS_FLOAT_DOUBLE_PRECISION
  rounded = nexus_real64_round((real64)value, mode);
#else
  rounded = (real64)nexus_real32_round((real32)value, mode);
#endif

  n_real64_assert_in_uint_large_range(rounded);
  return (uint_large)rounded;
}

f_real nexus_real_log_epsilon(void) {
  static f_real  cached = 0.0;
  static boolean ready  = FALSE;

  if (ready != TRUE) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
    cached = (f_real)log((double)DBL_EPSILON);
#else
    cached = (f_real)logf(FLT_EPSILON);
#endif
    ready = TRUE;
  }
  return cached;
}

f_real nexus_real_softmax_logit_prune_threshold(f_real temperature) {
  f_real floor_ln;

  floor_ln = nexus_real_log_epsilon();
  if (temperature <= 0.0 || temperature == 1.0) {
    return floor_ln;
  }
  return floor_ln * temperature;
}
