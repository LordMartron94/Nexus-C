#if defined(__linux__) || defined(__APPLE__)
#  define _POSIX_C_SOURCE 200112L
#endif

#include "../nexus.h"
#include <stdlib.h>
#include <time.h>

#if NEXUS_PLATFORM_WINDOWS
#  include <windows.h>
#endif

#if NEXUS_PLATFORM_LINUX || NEXUS_PLATFORM_MACOS

static NexusTimePrecision n_time_precision_from_nanoseconds(uint64 precision_nanoseconds) {
  if (precision_nanoseconds <= 1) {
    return NTP_NANOSECOND;
  }

  if (precision_nanoseconds <= NEXUS_NANOSECONDS_PER_MICROSECOND) {
    return NTP_MICROSECOND;
  }

  if (precision_nanoseconds <= NEXUS_NANOSECONDS_PER_MILLISECOND) {
    return NTP_MILLISECOND;
  }

  return NTP_SECOND;
}

static NexusTime nexus_time_from_clock(int clock_id) {
  struct timespec current_clock;
  struct timespec clock_resolution;
  uint64          precision_nanoseconds;

  uint64    current_time_ns;
  NexusTime current_time;

  if (clock_getres(clock_id, &clock_resolution) != 0) {
    exit(NEXUS_ERROR_CODE_CLOCK_FAILURE);
  }

  precision_nanoseconds = ((uint64)clock_resolution.tv_sec * NEXUS_NANOSECONDS_PER_SECOND) + (uint64)clock_resolution.tv_nsec;

  if (clock_gettime(clock_id, &current_clock) != 0) {
    exit(NEXUS_ERROR_CODE_CLOCK_FAILURE);
  }

  current_time_ns = ((uint64)current_clock.tv_sec * NEXUS_NANOSECONDS_PER_SECOND) + (uint64)current_clock.tv_nsec;

  current_time.time      = current_time_ns;
  current_time.precision = n_time_precision_from_nanoseconds(precision_nanoseconds);

  return current_time;
}

NexusTime nexus_time_get_real(void) {
  return nexus_time_from_clock(CLOCK_REALTIME);
}

NexusTime nexus_time_get_monotonic(void) {
  return nexus_time_from_clock(CLOCK_MONOTONIC);
}

#elif NEXUS_PLATFORM_WINDOWS

static NexusTimePrecision n_cached_precision         = NTP_COUNT;
static uint64             n_cached_counts_per_second = 0;

static NexusTimePrecision n_time_precision_from_counts_per_second(uint64 counts_per_second) {
  if (counts_per_second == 0) {
    return NTP_SECOND;
  }
  if (counts_per_second >= NEXUS_NANOSECONDS_PER_SECOND) {
    return NTP_NANOSECOND;
  }
  if (counts_per_second >= NEXUS_NANOSECONDS_PER_MILLISECOND) {
    return NTP_MICROSECOND;
  }
  if (counts_per_second >= NEXUS_NANOSECONDS_PER_MICROSECOND) {
    return NTP_MILLISECOND;
  }
  return NTP_SECOND;
}

static uint64 n_filetime_to_unix_ns(const FILETIME *ft) {
  uint64 ft_100ns = ((uint64)ft->dwHighDateTime << 32) | ft->dwLowDateTime;

  /* 116444736000000000 = ticks from 1601-01-01 to 1970-01-01 */
  const uint64 epoch_offset = 116444736000000000ULL;
  return (ft_100ns - epoch_offset) * 100ULL;
}

typedef void(WINAPI *pfnGetSystemTimePreciseAsFileTime)(LPFILETIME);

NexusTime nexus_time_get_real(void) {
  NexusTime                                current_time;
  FILETIME                                 ft;
  static pfnGetSystemTimePreciseAsFileTime pfnPrecise;
  static boolean                           checked;

  if (!checked) {
    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    if (hKernel) {
      pfnPrecise = (pfnGetSystemTimePreciseAsFileTime)GetProcAddress(hKernel, "GetSystemTimePreciseAsFileTime");
    }
    checked = TRUE;
  }

  if (pfnPrecise) {
    pfnPrecise(&ft);
    current_time.precision = NTP_MICROSECOND; /* <1us guaranteed */
  } else {
    GetSystemTimeAsFileTime(&ft);
    current_time.precision = NTP_MILLISECOND; /* conservative */
  }

  current_time.time = n_filetime_to_unix_ns(&ft);
  return current_time;
}

NexusTime nexus_time_get_monotonic(void) {
  NexusTime current_time = {0};
  uint64    counts_per_second;
  uint64    current_counts;

  if (n_cached_precision == NTP_COUNT) {
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&counts_per_second)) {
      exit(NEXUS_ERROR_CODE_CLOCK_FAILURE);
    }
    n_cached_counts_per_second = counts_per_second;
    n_cached_precision         = n_time_precision_from_counts_per_second(counts_per_second);
  }

  current_time.precision = n_cached_precision;

  if (!QueryPerformanceCounter((LARGE_INTEGER *)&current_counts)) {
    exit(NEXUS_ERROR_CODE_CLOCK_FAILURE);
  }

  /* Precise scaling: (counts * NS_PER_SEC) / freq */
  current_time.time = (current_counts * NEXUS_NANOSECONDS_PER_SECOND) / n_cached_counts_per_second;

  return current_time;
}

#endif

/* TODO - consolidate into an actual crash system (and integrating with `errno` for proper error reporting) */

NexusDuration nexus_time_duration_from_nanoseconds(int64 nanoseconds) {
  NexusDuration duration;
  duration.nanoseconds = nanoseconds;
  duration.precision   = NTP_NANOSECOND;
  return duration;
}

NexusDuration nexus_time_duration_from_microseconds(int64 microseconds) {
  NexusDuration duration;
  duration.nanoseconds = microseconds * (int64)NEXUS_NANOSECONDS_PER_MICROSECOND;
  duration.precision   = NTP_MICROSECOND;
  return duration;
}

NexusDuration nexus_time_duration_from_milliseconds(int64 milliseconds) {
  NexusDuration duration;
  duration.nanoseconds = milliseconds * (int64)NEXUS_NANOSECONDS_PER_MILLISECOND;
  duration.precision   = NTP_MILLISECOND;
  return duration;
}

NexusDuration nexus_time_duration_from_seconds(int64 seconds) {
  NexusDuration duration;
  duration.nanoseconds = seconds * (int64)NEXUS_NANOSECONDS_PER_SECOND;
  duration.precision   = NTP_SECOND;
  return duration;
}

NexusTime nexus_time_add_duration(NexusTime time, NexusDuration duration) {
  NexusTime result;
  uint64    base_time;
  uint64    offset;
  uint64    new_time;

  result.precision = (time.precision < duration.precision) ? time.precision : duration.precision;

  base_time = time.time;
  offset    = (uint64)duration.nanoseconds;
  new_time  = base_time + offset;

  if (duration.nanoseconds > 0 && new_time < base_time) {
    result.time = ~((uint64)0); /* max uint64 */
  } else if (duration.nanoseconds < 0 && new_time > base_time) {
    result.time = 0;
  } else {
    result.time = new_time;
  }

  return result;
}

NexusTime nexus_time_sub_duration(NexusTime time, NexusDuration duration) {
  NexusDuration negated;
  negated.nanoseconds = -duration.nanoseconds;
  negated.precision   = duration.precision;
  return nexus_time_add_duration(time, negated);
}

NexusDuration nexus_time_duration_add(NexusDuration duration1, NexusDuration duration2) {
  NexusDuration result;
  result.nanoseconds = duration1.nanoseconds + duration2.nanoseconds;
  result.precision   = duration1.precision < duration2.precision ? duration1.precision : duration2.precision;
  return result;
}

NexusDuration nexus_time_duration_sub(NexusDuration duration1, NexusDuration duration2) {
  NexusDuration result;
  result.nanoseconds = duration1.nanoseconds - duration2.nanoseconds;
  result.precision   = duration1.precision < duration2.precision ? duration1.precision : duration2.precision;
  return result;
}

NexusDuration nexus_time_duration_mul(NexusDuration duration, int64 scalar) {
  NexusDuration result;
  result.nanoseconds = duration.nanoseconds * scalar;
  result.precision   = duration.precision;
  return result;
}

NexusDuration nexus_time_duration_mul_f(NexusDuration duration, f_real scalar) {
  NexusDuration result;

  /* Note: This will truncate precision for durations exceeding ~104 days. */
  result.nanoseconds = (int64)((f_real)duration.nanoseconds * scalar);
  result.precision   = duration.precision;

  return result;
}

NexusDuration nexus_time_duration_div(NexusDuration duration, int64 scalar) {
  NexusDuration result;
  if (scalar == 0) {
    result.nanoseconds = 0;
  } else {
    result.nanoseconds = duration.nanoseconds / scalar;
  }
  result.precision = duration.precision;
  return result;
}

NexusDateTime nexus_time_to_local_datetime(NexusTime utc_time) {
  NexusDateTime date_time;
  uint64        seconds;
  uint32        nanoseconds;
  time_t        timer_seconds;
  struct tm     tm_info;
#if NEXUS_PLATFORM_WINDOWS
  __time64_t t64;
#endif
  int conversion_ok;

  /* Initialize baseline */
  date_time.year       = 0;
  date_time.month      = 0;
  date_time.day        = 0;
  date_time.hour       = 0;
  date_time.minute     = 0;
  date_time.second     = 0;
  date_time.nanosecond = 0;
  date_time.precision  = utc_time.precision;
  date_time.is_dst     = -1;

  seconds              = utc_time.time / NEXUS_NANOSECONDS_PER_SECOND;
  nanoseconds          = (uint32)(utc_time.time % NEXUS_NANOSECONDS_PER_SECOND);
  date_time.nanosecond = nanoseconds;

  if (seconds == 0) {
  } else if (seconds > 0xFFFFFFFFFFFFULL) {
    return date_time;
  }

#if NEXUS_PLATFORM_WINDOWS
  t64           = (__time64_t)seconds;
  conversion_ok = (_localtime64_s(&tm_info, &t64) == 0);
#else
  timer_seconds = (time_t)seconds;
  conversion_ok = (localtime_r(&timer_seconds, &tm_info) != NULL);
#endif

  if (!conversion_ok) {
    return date_time;
  }

  date_time.year   = (int32)(tm_info.tm_year + 1900);
  date_time.month  = (uint8)(tm_info.tm_mon + 1);
  date_time.day    = (uint8)tm_info.tm_mday;
  date_time.hour   = (uint8)tm_info.tm_hour;
  date_time.minute = (uint8)tm_info.tm_min;
  date_time.second = (uint8)tm_info.tm_sec;
  date_time.is_dst = (int8)tm_info.tm_isdst;

  return date_time;
}