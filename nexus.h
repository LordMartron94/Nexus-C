#pragma once

/* ---------------------------------------------------------------------------- */
/* GLOBAL DEFINES                                                               */
/* ---------------------------------------------------------------------------- */

/*
These typedefs have (in part) been sourced from Eskil Steenberg's Forge.
*/

/* PLATFORM DETECTION */

#ifndef NEXUS_PLATFORMS
#  define NEXUS_PLATFORMS

#  if defined(_WIN32)
#    define NEXUS_PLATFORM_WINDOWS 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    define NEXUS_PLATFORM_MACOS 1
#  elif defined(__linux__)
#    define NEXUS_PLATFORM_LINUX 1
#  else
#    error Unsupported platform
#  endif
#endif

#ifndef NULL
#  ifdef __cplusplus
#    define NULL 0 /* Defines NULL in C++*/
#  else
#    define NULL ((void *)0) /* Defines NULL in C*/
#  endif
#endif

#if !defined(TRUE)
#  define TRUE 1
#endif

#if !defined(FALSE)
#  define FALSE 0
#endif

#if defined _WIN32
typedef unsigned int uint;
#else
#  include <sys/types.h>
#endif

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

typedef signed char  int8;
typedef signed short int16;
typedef signed int   int32;

typedef float  real32;
typedef double real64;

typedef uint8 byte;

typedef unsigned char boolean;

/* 64 bit exceptions */

#if defined(_MSC_VER)
typedef signed __int64   int64;
typedef unsigned __int64 uint64;

#elif defined(__GNUC__) || defined(__clang__)
__extension__ typedef signed long long   int64;
__extension__ typedef unsigned long long uint64;

#elif defined(__LP64__) || defined(_LP64)
typedef signed long   int64;
typedef unsigned long uint64;

#else
#  error "Your compiler/platform does not support a native 64-bit integer type in C89 mode."
#endif

/*
timestamp is a 64-bit data type to store time values.

Semantics are determined by usage.
*/
typedef uint64 timestamp;

/* PRECISIONS */

#define NEXUS_DOUBLE_PRECISION /* if NEXUS_DOUBLE_PRECISION is defined the type "f_real" is defined as double otherwhise it will be defined as       \
                               float. This is very useful if you want to write an application that can be compiled to use either 32 or 64 bit        \
                               floating point math.                                                                                                  \
                               */

#ifdef NEXUS_DOUBLE_PRECISION
typedef double f_real;
#else
typedef float f_real;
#endif

/* ERROR CODES */

#ifndef NEXUS_ERROR_CODES
#  define NEXUS_ERROR_CODES
#  define NEXUS_ERROR_CODE_FLUSH_FAILURE               0x2
#  define NEXUS_ERROR_CODE_USE_OF_UNINITIALIZED_OBJECT 0x3
#  define NEXUS_ERROR_CODE_PRINT_FORMAT_FAILURE        0x4
#  define NEXUS_ERROR_CODE_CLOCK_FAILURE               0x5
#  define NEXUS_ERROR_CODE_ALLOCATION_FAILURE          0x6
#endif

/* ---------------------------------------------------------------------------- */
/* TIME                                                                         */
/* ---------------------------------------------------------------------------- */

#ifndef NEXUS_TIME_DEFINES
#  define NEXUS_TIME_DEFINES
#  define NEXUS_NANOSECONDS_PER_MICROSECOND 1000ULL
#  define NEXUS_NANOSECONDS_PER_MILLISECOND 1000000ULL
#  define NEXUS_NANOSECONDS_PER_SECOND      1000000000ULL
#endif

/*
NexusTimePrecision is an enum specifying different precisions for time.
*/
typedef enum NexusTimePrecision {
  /*
  NTP_NANOSECOND indicates a nanosecond precision.
  */
  NTP_NANOSECOND,

  /*
  NTP_MICROSECOND indicates a microsecond precision.
  */
  NTP_MICROSECOND,

  /*
  NTP_MILLISECOND indicates a millisecond precision.
  */
  NTP_MILLISECOND,

  /*
  NTP_SECOND indicates a second precision.
  */
  NTP_SECOND,

  /*
  NTP_COUNT is the number of NexusTimePrecision enum values.
  */
  NTP_COUNT
} NexusTimePrecision;

/*
NexusTime represents an absolute, timezone-agnostic point in time (Machine Time).

CRITICAL ARCHITECTURE NOTE:
This struct must strictly hold either a pure UTC Epoch or a Monotonic hardware count.
Never mathematically shift this value to represent a local timezone offset. Timezones
are exclusively a presentation-layer concern handled by NexusDateTime.
*/
typedef struct NexusTime {
  /*
  time stores the raw timestamp. The semantic meaning (UTC Epoch vs. Monotonic)
  is defined by the function used to generate it.
  */
  timestamp time;

  /*
  precision stores the reported precision for the used clock (`clock_getres` semantics).
  This indicates the mathematical trustworthiness of the sub-second field.
  */
  NexusTimePrecision precision;
} NexusTime;

/*
nexus_time_get_real returns the current wall-clock time as a pure UTC Epoch.
*/
extern NexusTime nexus_time_get_real(void);

/*
nexus_time_get_monotonic returns an absolute monotonic duration since an arbitrary
hardware boot point. Useful only for measuring durations; never for calendar dates.
*/
extern NexusTime nexus_time_get_monotonic(void);

/*
NexusDateTime represents a broken-down calendar date and time (Human Time).
This is the presentation layer. It provides human-readable components and
should be the only structure that ever reflects a local timezone or DST.
*/
typedef struct NexusDateTime {
  /* year: Full year (e.g., 2026). Supports negative years for dates before 1 AD. */
  int32 year;
  /* month: Month of the year [1, 12]. */
  uint8 month;
  /* day: Day of the month [1, 31]. */
  uint8 day;
  /* hour: Hour of the day [0, 23]. */
  uint8 hour;
  /* minute: Minute of the hour [0, 59]. */
  uint8 minute;
  /* second: Second of the minute [0, 60] (60 accommodates leap seconds). */
  uint8 second;
  /* nanosecond: Sub-second precision [0, 999999999]. */
  uint32 nanosecond;
  /* precision: The original precision reported for the used clock. */
  NexusTimePrecision precision;
  /* is_dst: Indicates whether Daylight Saving Time was in effect.
  (-1 = unknown, 0 = no, 1 = yes).
  */
  int8 is_dst;
} NexusDateTime;

/*
NexusDuration represents a signed time interval.
It is independent of any absolute point in time and can be positive or negative.
*/
typedef struct NexusDuration {
  /* nanoseconds: Signed duration in nanoseconds. */
  int64 nanoseconds;
  /* precision: Indicates the minimal granularity used to calculate this duration. */
  NexusTimePrecision precision;
} NexusDuration;

extern NexusDuration nexus_time_duration_from_nanoseconds(int64 nanoseconds);
extern NexusDuration nexus_time_duration_from_microseconds(int64 microseconds);
extern NexusDuration nexus_time_duration_from_milliseconds(int64 milliseconds);
extern NexusDuration nexus_time_duration_from_seconds(int64 seconds);

/*
nexus_time_add_duration safely adds a duration to a baseline time.
Automatically handles integer overflow by clamping to standard maximums.
*/
extern NexusTime nexus_time_add_duration(NexusTime time, NexusDuration duration);

/*
nexus_time_sub_duration safely subtracts a duration from a baseline time.
Automatically handles integer underflow by clamping to zero.
*/
extern NexusTime nexus_time_sub_duration(NexusTime time, NexusDuration duration);

/*
nexus_time_duration_add returns the sum of two durations.
The resulting precision is clamped to the least precise input.
*/
extern NexusDuration nexus_time_duration_add(NexusDuration duration1, NexusDuration duration2);

/*
nexus_time_duration_sub returns the difference between two durations.
The resulting precision is clamped to the least precise input.
*/
extern NexusDuration nexus_time_duration_sub(NexusDuration duration1, NexusDuration duration2);

/*
nexus_time_duration_mul scales a duration by an integer multiplier.
*/
extern NexusDuration nexus_time_duration_mul(NexusDuration duration, int64 scalar);

/*
nexus_time_duration_div scales a duration by an integer divisor.
Safely handles division by zero by yielding a zeroed duration.
*/
extern NexusDuration nexus_time_duration_div(NexusDuration duration, int64 scalar);

/*
nexus_time_duration_mul_f scales a duration by a floating-point multiplier.
WARNING: Precision will be truncated and lost for durations exceeding ~104 days
due to f_real mantissa limits.
*/
extern NexusDuration nexus_time_duration_mul_f(NexusDuration duration, f_real scalar);

/*
nexus_time_to_local_datetime converts a pure UTC Epoch directly into a localized
Human Time presentation structure.

This safely intercepts the OS timezone and DST rules without destroying the
mathematical integrity of the underlying integer.
*/
extern NexusDateTime nexus_time_to_local_datetime(NexusTime utc_time);

/* ---------------------------------------------------------------------------- */
/* COLOR                                                                        */
/* ---------------------------------------------------------------------------- */

/*
NexusColorRGBA8 stores 8 bit rgba colors.
*/
typedef struct NexusColorRGBA8 {
  uint8 red;
  uint8 green;
  uint8 blue;
  uint8 alpha;
} NexusColorRGBA8;

/*
nexus_color_rgba8_create creates a `NexusColorRGBA8` struct from red, green, blue, and alpha components.

This is strictly a utility function as you can just as well construct the struct directly.
*/
extern NexusColorRGBA8 nexus_color_rgba8_create(uint8 red, uint8 green, uint8 blue, uint8 alpha);

/*
nexus_color_rgba8_create_rgb creates a `NexusColorRGBA8` struct from red, green, and blue components.
It sets alpha to 255 as it is unused (meaning full opacity).

This is strictly a utility function as you can just as well construct the struct directly.
*/
extern NexusColorRGBA8 nexus_color_rgba8_create_rgb(uint8 red, uint8 green, uint8 blue);