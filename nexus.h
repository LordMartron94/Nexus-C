#pragma once

/*

NOTE: there are some defines here that error on compilation without support.

I will resolve this into proper dependable and portable code when I have more experience with C.

Written on:   15 June 2026
Resolved on:  17 June 2026

*/

#include <stdarg.h>

/* ---------------------------------------------------------------------------- */
/* GLOBAL DEFINES                                                               */
/* ---------------------------------------------------------------------------- */

/*
These typedefs have (in part) been sourced from Eskil Steenberg's Forge.
*/

/* PLATFORM DETECTION */

#ifndef NEXUS_PLATFORMS
#  define NEXUS_PLATFORMS

#  if defined(_WIN32) || defined(_WIN64)
#    define NEXUS_PLATFORM_WINDOWS 1
#  elif defined(__ANDROID__)
#    define NEXUS_PLATFORM_ANDROID 1
#  elif defined(__APPLE__) && defined(__MACH__)
#    if defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#      define NEXUS_PLATFORM_IOS 1
#    else
#      define NEXUS_PLATFORM_MACOS 1
#    endif
#  elif defined(__FreeBSD__)
#    define NEXUS_PLATFORM_FREEBSD 1
#    define NEXUS_PLATFORM_BSD     1
#  elif defined(__NetBSD__)
#    define NEXUS_PLATFORM_NETBSD 1
#    define NEXUS_PLATFORM_BSD    1
#  elif defined(__OpenBSD__)
#    define NEXUS_PLATFORM_OPENBSD 1
#    define NEXUS_PLATFORM_BSD     1
#  elif defined(__DragonFly__)
#    define NEXUS_PLATFORM_DRAGONFLY 1
#    define NEXUS_PLATFORM_BSD       1
#  elif defined(__bsdi__) || defined(__bsdi)
#    define NEXUS_PLATFORM_BSDI 1
#    define NEXUS_PLATFORM_BSD  1
#  elif defined(__linux__)
#    define NEXUS_PLATFORM_LINUX 1
#  elif defined(__unix__) || defined(__unix)
#    /* generic UNIX fallback; could be some other BSD or System V UNIX */
#    define NEXUS_PLATFORM_UNIX 1
#  else
#    define NEXUS_PLATFORM_UNKNOWN 1
#  endif

#  if defined(NEXUS_PLATFORM_LINUX) || defined(NEXUS_PLATFORM_MACOS) || defined(NEXUS_PLATFORM_ANDROID)
#    define NEXUS_PLATFORM_POSIX 1
#  endif
#endif /* PLATFORM DETECTION */

#if defined(NEXUS_PLATFORM_POSIX)
#  ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE /* NOLINT */
#  endif
#endif

#ifndef NEXUS_ARCHITECTURES
#  define NEXUS_ARCHITECTURES

/* Adapted from: https://stackoverflow.com/a/66249936 */
/* RISC-V macros: __riscv, __riscv32, __riscv64, __riscv_xlen (GCC/Clang); see
   https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/r4cUgIhWLbY */

#  define NEXUS_ARCH_UNKNOWN (-1)

/* --- X86 --- */
#  define NEXUS_ARCH_X86_64 1
#  define NEXUS_ARCH_X86_32 2

/* --- ARM --- */
#  define NEXUS_ARCH_ARM2   3
#  define NEXUS_ARCH_ARM3   4
#  define NEXUS_ARCH_ARM4T  5
#  define NEXUS_ARCH_ARM5   6
#  define NEXUS_ARCH_ARM6T2 7
#  define NEXUS_ARCH_ARM7   8
#  define NEXUS_ARCH_ARM7A  9
#  define NEXUS_ARCH_ARM7R  10
#  define NEXUS_ARCH_ARM7M  11
#  define NEXUS_ARCH_ARM7S  12
#  define NEXUS_ARCH_ARM64  13

/* --- MISC --- */
#  define NEXUS_ARCH_MIPS      14
#  define NEXUS_ARCH_SUPERH    15
#  define NEXUS_ARCH_POWERPC   16
#  define NEXUS_ARCH_POWERPC64 17
#  define NEXUS_ARCH_SPARC     18
#  define NEXUS_ARCH_M68K      19

/* --- RISC-V --- */
#  define NEXUS_ARCH_RISCV32 20
#  define NEXUS_ARCH_RISCV64 21

#  if defined(__x86_64__) || defined(_M_X64)
#    define NEXUS_ARCH NEXUS_ARCH_X86_64
#  elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#    define NEXUS_ARCH NEXUS_ARCH_X86_32
#  elif defined(__ARM_ARCH_2__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM2
#  elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM3
#  elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
#    define NEXUS_ARCH NEXUS_ARCH_ARM4T
#  elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
#    define NEXUS_ARCH NEXUS_ARCH_ARM5
#  elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
#    define NEXUS_ARCH NEXUS_ARCH_ARM6T2
#  elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM6
#  elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM7
#  elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM7A
#  elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM7R
#  elif defined(__ARM_ARCH_7M__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM7M
#  elif defined(__ARM_ARCH_7S__)
#    define NEXUS_ARCH NEXUS_ARCH_ARM7S
#  elif defined(__aarch64__) || defined(_M_ARM64)
#    define NEXUS_ARCH NEXUS_ARCH_ARM64
#  elif defined(mips) || defined(__mips__) || defined(__mips)
#    define NEXUS_ARCH NEXUS_ARCH_MIPS
#  elif defined(__sh__)
#    define NEXUS_ARCH NEXUS_ARCH_SUPERH
#  elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) ||      \
      defined(_ARCH_PPC)
#    define NEXUS_ARCH NEXUS_ARCH_POWERPC
#  elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
#    define NEXUS_ARCH NEXUS_ARCH_POWERPC64
#  elif defined(__sparc__) || defined(__sparc)
#    define NEXUS_ARCH NEXUS_ARCH_SPARC
#  elif defined(__m68k__)
#    define NEXUS_ARCH NEXUS_ARCH_M68K
#  elif defined(__riscv) || defined(__riscv__) || defined(_riscv)
#    if defined(__riscv64) || (defined(__riscv_xlen) && __riscv_xlen == 64)
#      define NEXUS_ARCH NEXUS_ARCH_RISCV64
#    elif defined(__riscv32) || (defined(__riscv_xlen) && __riscv_xlen == 32)
#      define NEXUS_ARCH NEXUS_ARCH_RISCV32
#    elif defined(__LP64__) || defined(_LP64)
#      define NEXUS_ARCH NEXUS_ARCH_RISCV64
#    else
#      define NEXUS_ARCH NEXUS_ARCH_RISCV32
#    endif
#  else
#    define NEXUS_ARCH NEXUS_ARCH_UNKNOWN
#  endif

#endif /* ARCHITECTURE DETECTION*/

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

#if !defined(_WIN32)
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

#include <limits.h>

/*
Fixed-width 64-bit integers (compiler extensions; Dependable C).

Semantics: values that are always exactly 64 bits wide, independent of CPU word size.
Used for durations in nanoseconds, epoch arithmetic, and the uint64/int64 primitives.
*/
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
__extension__ typedef signed long long   int64;
__extension__ typedef unsigned long long uint64;
#endif

/*
Architecture word width (pointer size).

Semantics: distinguishes 32-bit from 64-bit address spaces. Used when behavior must
follow allocatable object size rather than fixed 64-bit width.

Compile-time overrides (define before including this header):
  NEXUS_FORCE_32_BIT  Simulate a 32-bit address space on a 64-bit host. uint_large and
                      int_large become 32-bit; timestamp/int64/uint64 are unchanged.
                      Useful to validate client code paths without a separate toolchain.

NEXUS_ARCHITECTURE_BITS_NATIVE is the detected host width without overrides.
NEXUS_ARCHITECTURE_BITS is the effective width used by Nexus (may be forced to 32).
NEXUS_ARCHITECTURE_FORCED_32_BIT is TRUE when NEXUS_FORCE_32_BIT is active.
*/
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__) || defined(__amd64__) ||                   \
    defined(__riscv64) || (defined(__riscv_xlen) && __riscv_xlen == 64)
#  define NEXUS_ARCHITECTURE_BITS_NATIVE 64
#elif defined(_WIN32) || defined(__i386__) || defined(_M_IX86) || defined(__arm__) || defined(__ARMEL__) || defined(__ARMEB__) ||                    \
    defined(__riscv32) || (defined(__riscv_xlen) && __riscv_xlen == 32)
#  define NEXUS_ARCHITECTURE_BITS_NATIVE 32
#else
#  if defined(ULONG_MAX) && (ULONG_MAX > 4294967295UL)
#    define NEXUS_ARCHITECTURE_BITS_NATIVE 64
#  else
#    define NEXUS_ARCHITECTURE_BITS_NATIVE 32
#  endif
#endif

#if NEXUS_FORCE_32_BIT
#  define NEXUS_ARCHITECTURE_BITS          32
#  define NEXUS_ARCHITECTURE_FORCED_32_BIT TRUE
#else
#  define NEXUS_ARCHITECTURE_BITS          NEXUS_ARCHITECTURE_BITS_NATIVE
#  define NEXUS_ARCHITECTURE_FORCED_32_BIT FALSE
#endif

#define NEXUS_ARCHITECTURE_IS_32_BIT (NEXUS_ARCHITECTURE_BITS == 32)
#define NEXUS_ARCHITECTURE_IS_64_BIT (NEXUS_ARCHITECTURE_BITS == 64)

/*
Native word-sized integers.

Semantics: counts, lengths, indices, and buffer sizes that must fit any single
allocatable object on this architecture. On 32-bit builds this is 32 bits even when
the compiler also provides fixed-width 64-bit types.
*/
#if NEXUS_ARCHITECTURE_BITS == 64
typedef int64  int_large;
typedef uint64 uint_large;
#else
typedef int32  int_large;
typedef uint32 uint_large;
#endif

/*
timestamp stores absolute time values as unsigned nanoseconds since an epoch.

Uses uint64 on all supported targets so epoch math is full-range on both 32-bit and
64-bit CPUs. uint_large is not used here because address width does not limit time span.
*/
typedef uint64 timestamp;

/*
uint128 is a 128-bit unsigned integer stored as two 64-bit words.

lo holds the least significant 64 bits. hi holds the most significant 64 bits.
Arithmetic wraps modulo 2^128.
*/
typedef struct uint128 { /* NOLINT */
  uint64 lo;
  uint64 hi;
} uint128; /* NOLINT */

/*
nexus_uint128_make constructs a 128-bit value from low and high words.
*/
extern uint128 nexus_uint128_make(uint64 lo, uint64 hi);

/*
nexus_uint128_zero returns zero.
*/
extern uint128 nexus_uint128_zero(void);

/*
nexus_uint128_add returns left + right modulo 2^128.
*/
extern uint128 nexus_uint128_add(uint128 left, uint128 right);

/*
nexus_uint128_xor returns the bitwise exclusive OR of left and right.
*/
extern uint128 nexus_uint128_xor(uint128 left, uint128 right);

/*
nexus_uint128_mul returns left * right modulo 2^128.
*/
extern uint128 nexus_uint128_mul(uint128 left, uint128 right);

/*
nexus_uint128_shift_right performs a logical right shift by shift bits.
Shifts of 128 or more return zero.
*/
extern uint128 nexus_uint128_shift_right(uint128 value, uint32 shift);

/*
nexus_uint128_fold_to_uint64 folds a 128-bit value to 64 bits via hi ^ lo.
*/
extern uint64 nexus_uint128_fold_to_uint64(uint128 value);

/*
nexus_uint128_bytes_little_endian_write serializes value to 16 little-endian bytes.
*/
extern void nexus_uint128_bytes_little_endian_write(uint128 value, byte out_bytes[16]);

/*
nexus_uint128_bytes_little_endian_read deserializes 16 little-endian bytes into value.
*/
extern uint128 nexus_uint128_bytes_little_endian_read(const byte in_bytes[16]);

/*
nexus_uint128_bytes_big_endian_write serializes value to 16 big-endian bytes.
*/
extern void nexus_uint128_bytes_big_endian_write(uint128 value, byte out_bytes[16]);

/*
nexus_uint128_bytes_big_endian_read deserializes 16 big-endian bytes into value.
*/
extern uint128 nexus_uint128_bytes_big_endian_read(const byte in_bytes[16]);

/* PRECISIONS */

#ifndef NEXUS_FLOAT_DOUBLE_PRECISION
#  define NEXUS_FLOAT_DOUBLE_PRECISION                                                                                                               \
    TRUE /* if NEXUS_FLOAT_DOUBLE_PRECISION is turned on, the type "f_real" is defined as double otherwise it will be defined as                     \
    float. This is very useful if you want to write an application that can be compiled to use either 32 or 64 bit                                   \
    floating point math.                                                                                                                             \
    */
#endif

#if (NEXUS_FLOAT_DOUBLE_PRECISION)
typedef double f_real;
#else
typedef float f_real;
#endif

#include <float.h>

#define INT8_MAX_VAL  SCHAR_MAX
#define INT8_MIN_VAL  SCHAR_MIN
#define UINT8_MAX_VAL UCHAR_MAX

#define INT16_MAX_VAL  SHRT_MAX
#define INT16_MIN_VAL  SHRT_MIN
#define UINT16_MAX_VAL USHRT_MAX

#define INT32_MAX_VAL  INT_MAX
#define INT32_MIN_VAL  INT_MIN
#define UINT32_MAX_VAL UINT_MAX

/* 64-bit bounds (available on all supported targets). */
#if defined(_MSC_VER)
#  define INT64_MAX_VAL  _I64_MAX
#  define INT64_MIN_VAL  _I64_MIN
#  define UINT64_MAX_VAL _UI64_MAX
#else
#  define INT64_MAX_VAL  ((int64)9223372036854775807LL)
#  define INT64_MIN_VAL  ((int64)(-9223372036854775807LL - 1))
#  define UINT64_MAX_VAL ((uint64)18446744073709551615ULL)
#endif

#define TIMESTAMP_MAX_VAL UINT64_MAX_VAL

#if NEXUS_ARCHITECTURE_BITS == 64
#  define UINT_LARGE_MAX_VAL UINT64_MAX_VAL
#  define INT_LARGE_MAX_VAL  INT64_MAX_VAL
#  define INT_LARGE_MIN_VAL  INT64_MIN_VAL
#else
#  define UINT_LARGE_MAX_VAL UINT32_MAX_VAL
#  define INT_LARGE_MAX_VAL  INT32_MAX_VAL
#  define INT_LARGE_MIN_VAL  INT32_MIN_VAL
#endif

#define REAL32_MAX_VAL FLT_MAX
#define REAL32_MIN_VAL FLT_MIN
#define REAL32_EPSILON FLT_EPSILON

#define REAL64_MAX_VAL DBL_MAX
#define REAL64_MIN_VAL DBL_MIN
#define REAL64_EPSILON DBL_EPSILON

#if NEXUS_FLOAT_DOUBLE_PRECISION
#  define F_REAL_MAX     REAL64_MAX_VAL
#  define F_REAL_MIN     REAL64_MIN_VAL
#  define F_REAL_EPSILON REAL64_EPSILON
#else
#  define F_REAL_MAX     REAL32_MAX_VAL
#  define F_REAL_MIN     REAL32_MIN_VAL
#  define F_REAL_EPSILON REAL32_EPSILON
#endif

/* ---------------------------------------------------------------------------- */
/* ERRORS                                                                       */
/* ---------------------------------------------------------------------------- */

/*
NError is the universal 32-bit error format shared across all libraries.

The high 16 bits encode a two-character ASCII facility tag. The low 16 bits
encode a facility-specific error code. NEXUS_ERROR_NONE (0) is success.

Use nexus_errors_message_write to obtain a human-readable description of a
NError value. Nexus formats messages for its own 'N' 'X' facility. Other libraries
register a facility formatter via nexus_errors_message_formatter_register so the
same write path can describe their errors.
*/
typedef uint32 NError;

#define NEXUS_ERROR_NONE ((NError)0)

/*
NEXUS_ERROR_MAKE packs two ASCII facility characters and a 16-bit code into NError.
*/
#define NEXUS_ERROR_MAKE(c1, c2, code) (((((uint32)(c1)) << 24) | (((uint32)(c2)) << 16)) | ((uint16)(code)))

#define NEXUS_ERROR_FACILITY_BYTE_1(err) ((char)(((err) >> 24) & 0xFF))
#define NEXUS_ERROR_FACILITY_BYTE_2(err) ((char)(((err) >> 16) & 0xFF))
#define NEXUS_ERROR_CODE(err)            ((uint16)((err) & 0xFFFF))

/*
NEXUS_ERROR_LOCAL_ID extracts the facility-local 16-bit code from an NError value
or named error constant. Use this in formatter switch cases so the labels stay
tied to the named constants instead of raw numeric literals.
*/
#define NEXUS_ERROR_LOCAL_ID(err) NEXUS_ERROR_CODE(err)

#define NEXUS_ERROR_FILE_NOT_FOUND           NEXUS_ERROR_MAKE('N', 'X', 1)
#define NEXUS_ERROR_PERMISSION_DENIED        NEXUS_ERROR_MAKE('N', 'X', 2)
#define NEXUS_ERROR_ALREADY_EXISTS           NEXUS_ERROR_MAKE('N', 'X', 3)
#define NEXUS_ERROR_DIR_NOT_EMPTY            NEXUS_ERROR_MAKE('N', 'X', 4)
#define NEXUS_ERROR_DISK_FULL                NEXUS_ERROR_MAKE('N', 'X', 5)
#define NEXUS_ERROR_INVALID_ARGUMENT         NEXUS_ERROR_MAKE('N', 'X', 6)
#define NEXUS_ERROR_IO                       NEXUS_ERROR_MAKE('N', 'X', 7)
#define NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE NEXUS_ERROR_MAKE('N', 'X', 8)
#define NEXUS_ERROR_CAPACITY                 NEXUS_ERROR_MAKE('N', 'X', 9)

/*
NexusErrorMessageFormatter returns a human-readable description for a facility-specific
error code. Return a stable string (typically a string literal). Return NULL when the
code is unknown so nexus_errors_message_write can fall back to the generic "Error XX-code"
form.
*/
typedef const char *NexusErrorMessageFormatter(uint16 code);

/*
NEXUS_ERROR_MESSAGE_FORMATTER_CAPACITY is the maximum number of non-Nexus facility
formatters that may be registered at once.
*/
#define NEXUS_ERROR_MESSAGE_FORMATTER_CAPACITY 32

/*
nexus_errors_message_formatter_register associates formatter with the two-character
facility tag (facility_byte_1, facility_byte_2). Libraries call this during initialization
so nexus_errors_message_write can format their NError values.

The Nexus facility ('N', 'X') is reserved and cannot be registered.
Re-registering the same facility replaces the previous formatter.

Returns NEXUS_ERROR_NONE on success.
Returns NEXUS_ERROR_INVALID_ARGUMENT when formatter is NULL or the facility is reserved.
Returns NEXUS_ERROR_CAPACITY when the formatter table is full and the facility is new.
*/
extern NError nexus_errors_message_formatter_register(char facility_byte_1, char facility_byte_2, NexusErrorMessageFormatter *formatter);

/*
nexus_errors_message_formatter_unregister removes any formatter previously registered for
the given facility. Does nothing when no formatter is registered for that facility.
*/
extern void nexus_errors_message_formatter_unregister(char facility_byte_1, char facility_byte_2);

/*
nexus_errors_message_write copies a human-readable description of error into buffer,
optionally prefixed with prefix.

When prefix is NULL or an empty string, the message is written without a prefix.
Writes an empty string when error is NEXUS_ERROR_NONE. buffer must not be NULL and
buffer_max_length must be greater than zero.

Known facilities (Nexus 'N' 'X', plus any registered via
nexus_errors_message_formatter_register) produce descriptive messages. Unknown
facilities or unknown codes within a registered facility fall back to "Error XX-code".
*/
extern uint_large nexus_errors_message_write(NError error, char *buffer, uint_large buffer_max_length, const char *prefix);

/* ---------------------------------------------------------------------------- */
/* ASSERTIONS                                                                   */
/* ---------------------------------------------------------------------------- */

#ifndef NEXUS_ASSERTIONS_ENABLED
#  define NEXUS_ASSERTIONS_ENABLED 1
#endif

#ifndef NEXUS_DEBUG_ENABLED
#  define NEXUS_DEBUG_ENABLED 1
#endif

#ifndef NEXUS_ASSERTIONS_RUNTIME_ENABLED
#  define NEXUS_ASSERTIONS_RUNTIME_ENABLED 1
#endif

/*
ErrorMessageReportCallback is the callback that gets invoked when an assertion fails.

This is usually set to either an "ERROR" or "CRITICAL" report in a logger, depending on client interpretation.
*/
typedef void ErrorMessageReportCallback(void *user_data, const char *message, const char *file, uint32 line);

/*
nexus_assertions_is_active returns TRUE when compile-time assertions are enabled, runtime assertion gating is
enabled, and the runtime switch has not been cleared by nexus_assertions_runtime_enabled_set(FALSE).
*/
extern boolean nexus_assertions_is_active(void);

/*
nexus_assertions_runtime_enabled_get returns the current runtime assertion switch state.
*/
extern boolean nexus_assertions_runtime_enabled_get(void);

/*
nexus_assertions_runtime_enabled_set sets the runtime assertion switch without changing compile-time configuration.

When NEXUS_ASSERTIONS_RUNTIME_ENABLED is 0 at compile time, this is a no-op and assert macros are already removed.
When NEXUS_ASSERTIONS_ENABLED is 0 at compile time, this is a no-op and assert macros are already removed.
*/
extern void nexus_assertions_runtime_enabled_set(boolean enabled);

/*
nexus_assertions_error_callback_set sets the callback used for reporting assertion failures.
*/
extern void nexus_assertions_error_callback_set(ErrorMessageReportCallback *callback, void *user_data);

/*
nexus_assertions_error_callback_installed_get returns TRUE when an assertion failure callback is registered.
*/
extern boolean nexus_assertions_error_callback_installed_get(void);

/*
nexus_assertions_failure_report reports an assertion failure with a plain message string.
*/
extern void nexus_assertions_failure_report(const char *expression, const char *message, const char *file, uint32 line);

#if NEXUS_ASSERTIONS_ENABLED

#  if defined(_MSC_VER)
#    include <intrin.h>
#    define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
      __debugbreak();                                                                                                                                \
      abort();
#  elif defined(__GNUC__) || defined(__clang__)
/*
  Each trap instruction advances the PC; the trailing nop keeps that address inside the
  assertion call-site line range so debuggers stop on the NEXUS_ASSERT* invocation, not the next statement.
*/
#    if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                          \
        __asm__ __volatile__("brk #0\n\tnop");                                                                                                       \
        abort();
#    elif defined(__arm__) || defined(__ARM_ARCH) || defined(_M_ARM)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                          \
        __asm__ __volatile__("bkpt #0\n\tnop");                                                                                                      \
        abort();
#    elif defined(__riscv) || defined(__riscv__)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                          \
        __asm__ __volatile__("ebreak\n\tnop");                                                                                                       \
        abort();
#    elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                          \
        __asm__ __volatile__("int3\n\tnop");                                                                                                         \
        abort();
#    else
#      define NEXUS_ASSERTIONS_DEBUG_TRAP() __builtin_trap();
#    endif
#  else /* Generic fallback */
#    include <signal.h>
#    define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
      (void)raise(SIGTRAP);                                                                                                                          \
      abort();
#  endif /* NEXUS_ASSERTIONS_DEBUG_TRAP implementation selection */

#  if NEXUS_ASSERTIONS_ENABLED && NEXUS_ASSERTIONS_RUNTIME_ENABLED
/*
Expose runtime assertion state for inline macro evaluation in hot paths.
*/
extern boolean n_runtime_assertions_active;
#    define NEXUS_INTERNAL_ASSERT_ACTIVE() (n_runtime_assertions_active)
#  else
#    define NEXUS_INTERNAL_ASSERT_ACTIVE() (FALSE)
#  endif

#  if NEXUS_ASSERTIONS_RUNTIME_ENABLED

#    define NEXUS_ASSERT(expr)                                                                                                                       \
      do {                                                                                                                                           \
        if (NEXUS_INTERNAL_ASSERT_ACTIVE()) {                                                                                                        \
          if (!(expr)) {                                                                                                                             \
            nexus_assertions_failure_report(#expr, "", __FILE__, __LINE__);                                                                          \
          }                                                                                                                                          \
        }                                                                                                                                            \
      } while (0)

#    define NEXUS_ASSERT_MESSAGE(expr, message)                                                                                                      \
      do {                                                                                                                                           \
        if (NEXUS_INTERNAL_ASSERT_ACTIVE()) {                                                                                                        \
          if (!(expr)) {                                                                                                                             \
            nexus_assertions_failure_report(#expr, message, __FILE__, __LINE__);                                                                     \
          }                                                                                                                                          \
        }                                                                                                                                            \
      } while (0)

#    if NEXUS_DEBUG_ENABLED

#      define NEXUS_ASSERT_DEBUG(expr)                                                                                                               \
        do {                                                                                                                                         \
          if (NEXUS_INTERNAL_ASSERT_ACTIVE()) {                                                                                                      \
            if (!(expr)) {                                                                                                                           \
              nexus_assertions_failure_report(#expr, "", __FILE__, __LINE__);                                                                        \
            }                                                                                                                                        \
          }                                                                                                                                          \
        } while (0)

#      define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)                                                                                              \
        do {                                                                                                                                         \
          if (NEXUS_INTERNAL_ASSERT_ACTIVE()) {                                                                                                      \
            if (!(expr)) {                                                                                                                           \
              nexus_assertions_failure_report(#expr, message, __FILE__, __LINE__);                                                                   \
            }                                                                                                                                        \
          }                                                                                                                                          \
        } while (0)

#    else

#      define NEXUS_ASSERT_DEBUG(expr)
#      define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)

#    endif /* NEXUS_DEBUG_ENABLED */

#  else /* NEXUS_ASSERTIONS_RUNTIME_ENABLED */

#    define NEXUS_ASSERT(expr)
#    define NEXUS_ASSERT_MESSAGE(expr, message)
#    define NEXUS_ASSERT_DEBUG(expr)
#    define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)

#  endif /* NEXUS_ASSERTIONS_RUNTIME_ENABLED */

#else

#  define NEXUS_INTERNAL_ASSERT_ACTIVE() (FALSE)
#  define NEXUS_ASSERT(expr)
#  define NEXUS_ASSERT_MESSAGE(expr, message)
#  define NEXUS_ASSERT_DEBUG(expr)
#  define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)

#endif /* NEXUS_ASSERTIONS_ENABLED */

/* ---------------------------------------------------------------------------- */
/* REALS                                                                        */
/* ---------------------------------------------------------------------------- */

/*
NexusRealRoundMode selects how floating-point values are rounded before integer conversion.
*/
typedef enum NexusRealRoundMode {
  /*
  NRRM_FLOOR rounds toward negative infinity.
  */
  NRRM_FLOOR,

  /*
  NRRM_CEIL rounds toward positive infinity.
  */
  NRRM_CEIL,

  /*
  NRRM_TRUNC rounds toward zero.
  */
  NRRM_TRUNC,

  /*
  NRRM_ROUND rounds to the nearest integer; halves move away from zero.
  */
  NRRM_ROUND,

  /*
  NRRM_ROUND_EVEN rounds to the nearest integer; halves move to the nearest even value.
  */
  NRRM_ROUND_EVEN,

  /*
  NRRM_COUNT is the number of NexusRealRoundMode enum values.
  */
  NRRM_COUNT
} NexusRealRoundMode;

/*
nexus_real32_round rounds a 32-bit float using the selected mode.
*/
extern real32 nexus_real32_round(real32 value, NexusRealRoundMode mode);

/*
nexus_real64_round rounds a 64-bit float using the selected mode.
*/
extern real64 nexus_real64_round(real64 value, NexusRealRoundMode mode);

/*
nexus_real_round rounds the configured f_real type using the selected mode.
*/
extern f_real nexus_real_round(f_real value, NexusRealRoundMode mode);

/*
nexus_real_is_finite returns TRUE when value is neither NaN nor positive/negative infinity.
*/
static boolean nexus_real_is_finite(f_real value) /* NOLINT */ {
  return (value == value) && (value < REAL64_MAX_VAL) && (value > -REAL64_MAX_VAL);
}

/*
nexus_real_finite_or_zero returns value when finite, otherwise 0.
Use to scrub NaN/Inf before they poison accumulators or persisted LTM.
*/
static f_real nexus_real_finite_or_zero(f_real value) /* NOLINT */ {
  if (nexus_real_is_finite(value) != TRUE) {
    return 0.0;
  }
  return value;
}

/*
nexus_real_epsilon returns IEEE-754 machine epsilon for the configured f_real
(FLT_EPSILON or DBL_EPSILON). This is a silicon property, not a tunable.
*/
static f_real nexus_real_epsilon(void) /* NOLINT */ {
  return (f_real)F_REAL_EPSILON;
}

/*
nexus_real_log_epsilon returns ln(F_REAL_EPSILON), computed once and cached.

After a max-shifted softmax (max logit → 0), the sum is at least e^0 = 1.
Any exp(diff) with diff < ln(ε) is smaller than machine epsilon and cannot change
that sum in f_real — a perfect prune floor for exp. Not chess-/search-dependent.

If logits are later scaled by temperature T (exp(x/T)), the floor becomes
T · ln(ε); compute that product once when T is set, never per leaf.
*/
extern f_real nexus_real_log_epsilon(void);

/*
nexus_real_softmax_logit_prune_threshold returns the max-shifted logit floor for
skipping exp. temperature <= 0 or temperature == 1 → ln(ε); otherwise T · ln(ε).
*/
extern f_real nexus_real_softmax_logit_prune_threshold(f_real temperature);

/*
nexus_real32_round_to_int32 rounds then converts to int32. Debug builds assert range.
*/
extern int32 nexus_real32_round_to_int32(real32 value, NexusRealRoundMode mode);

/*
nexus_real32_round_to_int64 rounds then converts to int64. Debug builds assert range.
*/
extern int64 nexus_real32_round_to_int64(real32 value, NexusRealRoundMode mode);

/*
nexus_real32_round_to_uint32 rounds then converts to uint32. Debug builds assert range.
*/
extern uint32 nexus_real32_round_to_uint32(real32 value, NexusRealRoundMode mode);

/*
nexus_real32_round_to_uint64 rounds then converts to uint64. Debug builds assert range.
*/
extern uint64 nexus_real32_round_to_uint64(real32 value, NexusRealRoundMode mode);

/*
nexus_real64_round_to_int32 rounds then converts to int32. Debug builds assert range.
*/
extern int32 nexus_real64_round_to_int32(real64 value, NexusRealRoundMode mode);

/*
nexus_real64_round_to_int64 rounds then converts to int64. Debug builds assert range.
*/
extern int64 nexus_real64_round_to_int64(real64 value, NexusRealRoundMode mode);

/*
nexus_real64_round_to_uint32 rounds then converts to uint32. Debug builds assert range.
*/
extern uint32 nexus_real64_round_to_uint32(real64 value, NexusRealRoundMode mode);

/*
nexus_real64_round_to_uint64 rounds then converts to uint64. Debug builds assert range.
*/
extern uint64 nexus_real64_round_to_uint64(real64 value, NexusRealRoundMode mode);

/*
nexus_real_round_to_int32 rounds f_real then converts to int32. Debug builds assert range.
*/
extern int32 nexus_real_round_to_int32(f_real value, NexusRealRoundMode mode);

/*
nexus_real_round_to_int64 rounds f_real then converts to int64. Debug builds assert range.
*/
extern int64 nexus_real_round_to_int64(f_real value, NexusRealRoundMode mode);

/*
nexus_real_round_to_uint32 rounds f_real then converts to uint32. Debug builds assert range.
*/
extern uint32 nexus_real_round_to_uint32(f_real value, NexusRealRoundMode mode);

/*
nexus_real_round_to_uint64 rounds f_real then converts to uint64. Debug builds assert range.
*/
extern uint64 nexus_real_round_to_uint64(f_real value, NexusRealRoundMode mode);

/*
nexus_real_round_to_int_large rounds f_real then converts to int_large. Debug builds assert range.
*/
extern int_large nexus_real_round_to_int_large(f_real value, NexusRealRoundMode mode);

/*
nexus_real_round_to_uint_large rounds f_real then converts to uint_large. Debug builds assert range.
*/
extern uint_large nexus_real_round_to_uint_large(f_real value, NexusRealRoundMode mode);

/* ---------------------------------------------------------------------------- */
/* DEBUGGING AND MEMORY                                                         */
/* ---------------------------------------------------------------------------- */

/*
MergeSource / Forge memory debugger (f_mem_debug.c).

NEXUS_MEMORY_DEBUG_ENABLED hijacks malloc, calloc, free, and realloc with tracking wrappers.
Default: 1. Override before include to disable.

NEXUS_MEMORY_DEBUG_IMPLEMENTATION is set only in n_debug.c so that translation unit uses libc
allocators while implementing the wrappers.

NEXUS_EXIT_CRASH_ENABLED replaces exit() with exit_crash() for debugger-friendly termination.
Default: 1. Override before include to disable.

NEXUS_EXIT_CRASH_IMPLEMENTATION is set only in n_debug.c so allocator error paths use libc exit().
*/

#include <stddef.h>
#include <string.h>

#ifndef NEXUS_MEMORY_DEBUG_ENABLED
#  define NEXUS_MEMORY_DEBUG_ENABLED 1
#endif

#ifndef NEXUS_EXIT_CRASH_ENABLED
#  define NEXUS_EXIT_CRASH_ENABLED 1
#endif

#ifndef NEXUS_MEMORY_OVER_ALLOC
#  define NEXUS_MEMORY_OVER_ALLOC 64
#endif

#ifndef NEXUS_MEMORY_PRE_PADDING
#  define NEXUS_MEMORY_PRE_PADDING 16
#endif

#ifndef NEXUS_MEMORY_NULL_ALLOCATION_ERROR
#  define NEXUS_MEMORY_NULL_ALLOCATION_ERROR
#endif

#ifndef NEXUS_MEMORY_DOUBLE_FREE_CHECK
#  define NEXUS_MEMORY_DOUBLE_FREE_CHECK
#endif

#ifndef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
#  define NEXUS_MEMORY_USE_AFTER_FREE_CHECK
#endif

#ifndef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
#  define NEXUS_MEMORY_WARN_ON_REALLOC_NULL
#endif

#ifndef NEXUS_MEMORY_CALL_ON_ERROR
#  define NEXUS_MEMORY_CALL_ON_ERROR abort();
#endif

#ifndef NEXUS_MEMORY_STACK_GUESS_SIZE
#  define NEXUS_MEMORY_STACK_GUESS_SIZE ((size_t)1024 * (size_t)1024)
#endif

/*
nexus_debug_mem_thread_safe_init registers lock and unlock callbacks for the memory debugger.
Call once before any tracked allocation when more than one thread may allocate, free, or query
memory concurrently. Single-threaded programs may omit this call entirely.
lock and unlock must return zero on success. mutex is passed through to both callbacks unchanged.
*/
extern void nexus_debug_mem_thread_safe_init(int (*lock)(void *mutex), int (*unlock)(void *mutex), void *mutex);

/*
nexus_debug_mem_stack_pointer_set records the lowest address and size of the main thread stack.
This improves stack-pointer heuristics in nexus_debug_mem_check_stack_reference and related checks.
If unset, the debugger falls back to NEXUS_MEMORY_STACK_GUESS_SIZE when guessing stack addresses.
*/
extern void nexus_debug_mem_stack_pointer_set(void *lowest_stack_pointer, size_t stack_size_in_bytes);

/*
nexus_debug_mem_active enables or disables the full memory debugger at runtime.

When FALSE, malloc/calloc/realloc/free bypass over-allocation, canaries, and the tracking table
(except free/realloc of blocks that were still tracked from a prior active period), and
nexus_memory_bytes_copy/set/clear skip allocation queries. When TRUE, full tracking resumes for
new allocations. Prefer this over rebuilding with NEXUS_MEMORY_DEBUG_ENABLED 0 when the debugger
must stay available but hot paths need libc cost.
*/
extern void nexus_debug_mem_active(boolean active);

/*
nexus_debug_mem_active_get returns whether the full memory debugger is currently active.
*/
extern boolean nexus_debug_mem_active_get(void);

/*
nexus_debug_mem_active_exchange sets the debugger active flag and returns the previous value.

Use to temporarily suspend the full debugger around a hot path without assuming the prior state:
  previous = nexus_debug_mem_active_exchange(FALSE);
  ... work ...
  (void)nexus_debug_mem_active_exchange(previous);
*/
extern boolean nexus_debug_mem_active_exchange(boolean active);

/*
NexusDebugMemLogCallback is invoked for each tracked malloc, calloc, realloc, free, and for
nexus_memory_bytes_copy / set / clear when logging is enabled. message is fully formatted; file and
line identify the call site.
*/
typedef void NexusDebugMemLogCallback(void *user_data, const char *message, const char *file, uint32 line);

/*
nexus_debug_mem_log_callback_set registers a callback for allocation tracing.
Pass NULL callback to disable. The callback is not invoked re-entrantly (nested allocations during
logging are not logged). Thread-safe when initialized via nexus_debug_mem_thread_safe_init.
*/
extern void nexus_debug_mem_log_callback_set(NexusDebugMemLogCallback *callback, void *user_data);

/*
nexus_debug_mem_log_callback_installed_get returns TRUE when a memory log callback is registered.
*/
extern boolean nexus_debug_mem_log_callback_installed_get(void);

/*
nexus_debug_mem_malloc replaces malloc when NEXUS_MEMORY_DEBUG_ENABLED is set.
Allocates size bytes plus guard padding, records file and line, and returns the user pointer.
Returns NULL on failure. When NEXUS_MEMORY_NULL_ALLOCATION_ERROR is defined, failure triggers
NEXUS_MEMORY_CALL_ON_ERROR.
*/
extern void *nexus_debug_mem_malloc(size_t size, char *file, uint32 line);

/*
nexus_debug_mem_calloc replaces calloc when NEXUS_MEMORY_DEBUG_ENABLED is set.
Allocates num * size zero-filled bytes with guard padding and records file and line.
Returns NULL when num * size is zero or on allocation failure.
*/
extern void *nexus_debug_mem_calloc(size_t num, size_t size, char *file, uint32 line);

/*
nexus_debug_mem_realloc replaces realloc when NEXUS_MEMORY_DEBUG_ENABLED is set.
Resizes a previously tracked block or allocates afresh when pointer is NULL.
Returns NULL on failure or when size is zero (after freeing a non-NULL pointer).
Unrecognized pointers are reported and may fall through to libc realloc.
*/
extern void *nexus_debug_mem_realloc(void *pointer, size_t size, char *file, uint32 line);

/*
nexus_debug_mem_free replaces free when NEXUS_MEMORY_DEBUG_ENABLED is set.
Validates guard bytes, removes the block from tracking, and releases backing storage.
Detects double free, interior free, and suspected stack frees when the corresponding
NEXUS_MEMORY_*_CHECK options are enabled. file and line identify the call site.
*/
extern void nexus_debug_mem_free(void *buf, char *file, uint32 line);

/*
nexus_debug_mem_comment attaches an arbitrary label to a live allocation.
Useful in nexus_debug_mem_print output to identify buffers. Returns TRUE when buf is tracked.
*/
extern boolean nexus_debug_mem_comment(void *buf, char *comment);

/*
NexusDebugMemSummary holds aggregate allocation statistics recorded by the memory debugger.
Counters only advance while nexus_debug_mem_active is TRUE and are cleared by nexus_debug_mem_reset.
*/
typedef struct NexusDebugMemSummary {
  size_t     live_bytes;
  size_t     peak_live_bytes;
  uint32     live_block_count;
  uint32     peak_live_block_count;
  uint_large total_bytes_allocated;
  uint_large total_bytes_freed;
  uint_large allocation_count;
  uint_large free_count;
  uint32     call_site_count;
  size_t     largest_allocation_bytes;
} NexusDebugMemSummary;

/*
nexus_debug_mem_summary_get writes current allocation statistics into summary.
summary must not be NULL. Thread-safe when initialized via nexus_debug_mem_thread_safe_init.
*/
extern void nexus_debug_mem_summary_get(NexusDebugMemSummary *summary);

/*
nexus_debug_mem_summary_print writes a human-readable allocation statistics overview to stdout.
*/
extern void nexus_debug_mem_summary_print(void);

/*
nexus_debug_mem_print writes a human-readable leak report to stdout.
Lists each call site with more live blocks than min_allocs, including byte totals, live
allocation counts, and each live block with pointer, size, and any comment.
*/
extern void nexus_debug_mem_print(uint32 min_allocs);

/*
nexus_debug_mem_reset clears per-site byte totals and allocation counters without freeing live
memory or discarding tracked blocks. Use to ignore allocations made before a known baseline.
*/
extern void nexus_debug_mem_reset(void);

/*
NexusDebugMemMeasurement holds allocation statistics for a single measurement interval.
*/
typedef struct NexusDebugMemMeasurement {
  uint_large allocation_count;
  uint_large total_bytes_allocated;
  size_t     largest_allocation_bytes;
} NexusDebugMemMeasurement;

/*
NexusDebugMemMeasurementContext stores a baseline captured by measurement begin.
Pass the same context to measurement end to compute the interval delta.
*/
typedef struct NexusDebugMemMeasurementContext {
  uint_large baseline_allocation_count;
  uint_large baseline_total_bytes_allocated;
  size_t     interval_largest_allocation_bytes;
} NexusDebugMemMeasurementContext;

/*
nexus_debug_mem_measurement_begin snapshots current allocation statistics into context.
Global leak tracking is not reset; only the interval delta is reported by measurement end.
context must not be NULL.
*/
extern void nexus_debug_mem_measurement_begin(NexusDebugMemMeasurementContext *context);

/*
nexus_debug_mem_measurement_end writes allocation statistics for the interval since begin.
measurement and context must not be NULL. Pair with nexus_debug_mem_measurement_begin.
*/
extern void nexus_debug_mem_measurement_end(const NexusDebugMemMeasurementContext *context, NexusDebugMemMeasurement *measurement);

/*
nexus_debug_mem_consumption returns the sum of sizes for all currently tracked live allocations.
Thread-safe when initialized via nexus_debug_mem_thread_safe_init.
*/
extern size_t nexus_debug_mem_consumption(void);

/*
nexus_debug_mem_footprint returns the sum of sizes stored in the allocation table.
min_allocs is reserved for future filtering and is currently ignored.
*/
extern size_t nexus_debug_mem_footprint(uint32 min_allocs);

/*
nexus_debug_mem_query_allocation resolves pointer to the start of its owning live allocation.
When found, optionally writes source line, file path, and allocation size through line, file,
and size. Returns the allocation base address, or NULL when pointer is not inside a live block.
*/
extern void *nexus_debug_mem_query_allocation(void *pointer, uint32 *line, char **file, size_t *size);

/*
nexus_debug_mem_query_is_allocated checks whether size bytes at pointer are inside a live block.
Returns FALSE when the range extends past the allocation, overlaps freed memory, or lies on the
stack. When ignore_not_found is TRUE, an untracked pointer returns FALSE without a warning;
otherwise a warning is printed first. When the debugger is inactive, returns TRUE without scanning.
*/
extern boolean nexus_debug_mem_query_is_allocated(const void *pointer, size_t size, boolean ignore_not_found);

#if NEXUS_MEMORY_DEBUG_ENABLED

/*
nexus_debug_mem_bytes_copy is the memory-debugger path for nexus_memory_bytes_copy.
Validates dest and src against tracked heap allocations, emits a log callback event when installed,
then copies byte_count bytes. file and line identify the call site.
*/
extern void nexus_debug_mem_bytes_copy(void *dest, const void *src, uint_large byte_count, char *file, uint32 line);

/*
nexus_debug_mem_bytes_set is the memory-debugger path for nexus_memory_bytes_set.
Validates dest against tracked heap allocations, emits a log callback event when installed, then
writes byte into each of byte_count bytes. file and line identify the call site.
*/
extern void nexus_debug_mem_bytes_set(void *dest, uint8 byte, uint_large byte_count, char *file, uint32 line);

/*
nexus_debug_mem_bytes_clear is the memory-debugger path for nexus_memory_bytes_clear.
Validates dest against tracked heap allocations, emits a log callback event when installed, then
writes zero into each of byte_count bytes. file and line identify the call site.
*/
extern void nexus_debug_mem_bytes_clear(void *dest, uint_large byte_count, char *file, uint32 line);

#endif /* NEXUS_MEMORY_DEBUG_ENABLED */

/*
nexus_debug_mem_check_bounds scans all live allocations for guard-byte overruns and underruns,
and optionally use-after-free writes when NEXUS_MEMORY_USE_AFTER_FREE_CHECK is enabled.
Returns TRUE when any corruption is detected. Each error triggers NEXUS_MEMORY_CALL_ON_ERROR.
*/
extern boolean nexus_debug_mem_check_bounds(void);

/*
nexus_debug_mem_check_stack_reference scans live allocations for pointer values that may refer
to stack memory. Accuracy improves after nexus_debug_mem_stack_pointer_set. Returns TRUE when
any suspicious reference is reported.
*/
extern boolean nexus_debug_mem_check_stack_reference(void);

/*
nexus_debug_mem_check_heap_reference reports live allocations that cannot be reached from any
other tracked heap block or the configured stack scan. minimum_allocations skips call sites with
fewer total entries in the allocation table to reduce noise from short-lived helpers.
*/
extern void nexus_debug_mem_check_heap_reference(uint32 minimum_allocations);

/*
exit_crash terminates the process by writing through a null pointer.
Used when NEXUS_EXIT_CRASH_ENABLED replaces exit() so debuggers break at a deterministic fault
instead of a clean libc exit. status_code is currently unused but reserved for future use.
*/
extern void exit_crash(uint32 status_code);

#define NEXUS_SIZEOF(type) ((size_t)sizeof(type))

#define NEXUS_ALIGNOF(type)                                                                                                                          \
  ((size_t)&(((struct {                                                                                                                              \
               char c;                                                                                                                               \
               type t;                                                                                                                               \
             } *)0)                                                                                                                                  \
                 ->t))

#if NEXUS_MEMORY_DEBUG_ENABLED

#  include <stdlib.h>

#  if !defined(NEXUS_MEMORY_DEBUG_IMPLEMENTATION)
#    define malloc(n)     nexus_debug_mem_malloc(n, __FILE__, __LINE__)
#    define calloc(n, m)  nexus_debug_mem_calloc(n, m, __FILE__, __LINE__)
#    define realloc(n, m) nexus_debug_mem_realloc(n, m, __FILE__, __LINE__)
#    define free(n)       nexus_debug_mem_free(n, __FILE__, __LINE__)
#  endif

#else

#  ifndef NEXUS_MEMORY_DEBUG_INTERNAL
#    define nexus_debug_mem_thread_safe_init(n, m, k)
#    define nexus_debug_mem_stack_pointer_set(n, m)
#    define nexus_debug_mem_active(n)
#    define nexus_debug_mem_active_get()            TRUE
#    define nexus_debug_mem_active_exchange(active) TRUE
#    define nexus_debug_mem_log_callback_set(n, m)
#    define nexus_debug_mem_log_callback_installed_get() FALSE
#    define nexus_debug_mem_comment(n, m)
#    define nexus_debug_mem_print(n)
#    define nexus_debug_mem_summary_print()
#    define nexus_debug_mem_reset()
#    define nexus_debug_mem_consumption()                0
#    define nexus_debug_mem_footprint(n)                 0
#    define nexus_debug_mem_query_allocation(n, m, k, l) NULL
#    define nexus_debug_mem_query_is_allocated(n, m, k)  FALSE
#    define nexus_debug_mem_check_bounds()               FALSE
#    define nexus_debug_mem_check_stack_reference()      FALSE
#    define nexus_debug_mem_check_heap_reference(n)
#  endif

#endif

#define NEXUS_FREE_IF_NOT_NULL(ptr)                                                                                                                  \
  if ((ptr) != NULL) {                                                                                                                               \
    free((ptr));                                                                                                                                     \
  }

#if NEXUS_EXIT_CRASH_ENABLED

#  if !NEXUS_MEMORY_DEBUG_ENABLED
#    include <stdlib.h>
#  endif

#  if !defined(NEXUS_EXIT_CRASH_IMPLEMENTATION)
#    define exit(n) exit_crash(n)
#  endif

#endif

/* ---------------------------------------------------------------------------- */
/* VERSION                                                                      */
/* ---------------------------------------------------------------------------- */

#ifndef NEXUS_VERSION_PACK
#  define NEXUS_VERSION_PACK(variant, major, minor, patch)                                                                                           \
    ((NexusSemanticVersion)(((NexusSemanticVersion)(variant) & 0xFFFFu) << 48) | (((NexusSemanticVersion)(major) & 0xFFFFu) << 32) |                 \
     (((NexusSemanticVersion)(minor) & 0xFFFFu) << 16) | ((NexusSemanticVersion)(patch) & 0xFFFFu))
#endif

/*
NexusSemanticVersion stores packed variant/major/minor/patch fields in a uint64.
*/
typedef uint64 NexusSemanticVersion;

/*
nexus_version_pack packs variant/major/minor/patch into a semantic version.
*/
extern NexusSemanticVersion nexus_version_pack(uint16 variant, uint16 major, uint16 minor, uint16 patch);

/*
nexus_version_unpack unpacks a semantic version into its component fields.
*/
extern void nexus_version_unpack(NexusSemanticVersion version, uint16 *out_variant, uint16 *out_major, uint16 *out_minor, uint16 *out_patch);

/*
nexus_version_format writes a semantic version as "variant.major.minor.patch".
*/
extern void nexus_version_format(NexusSemanticVersion version, char *out_buffer, uint_large out_buffer_size);

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

/* ---------------------------------------------------------------------------- */
/* TERMINAL                                                                     */
/* ---------------------------------------------------------------------------- */

/*
NexusTerminalOutputCapabilities describes terminal-oriented output behavior for a file descriptor.
*/
typedef struct NexusTerminalOutputCapabilities {
  boolean is_terminal;
  boolean is_smart_terminal;
  boolean color_disabled;
  boolean color_forced;
} NexusTerminalOutputCapabilities;

/*
NexusColorOutputCapability classifies the color fidelity supported for terminal output.
*/
typedef enum NexusColorOutputCapability {
  NCOC_NONE = 0,
  NCOC_ANSI,
  NCOC_TRUECOLOR
} NexusColorOutputCapability;

/*
nexus_terminal_color_output_capability_get maps terminal capabilities to a color output mode.
*/
extern NexusColorOutputCapability nexus_terminal_color_output_capability_get(const NexusTerminalOutputCapabilities *capabilities);

/*
nexus_terminal_output_capabilities_get inspects file_descriptor and environment variables.

is_terminal reflects whether the descriptor is connected to a terminal device.
is_smart_terminal is TRUE when COLORTERM or TERM indicate truecolor or 256-color support.
color_disabled is TRUE when NO_COLOR is set. color_forced is TRUE when FORCE_COLOR is set.
*/
extern NError nexus_terminal_output_capabilities_get(int file_descriptor, NexusTerminalOutputCapabilities *capabilities);

/*
nexus_terminal_stdout_capabilities_get is a convenience wrapper for STDOUT_FILENO.
*/
extern NError nexus_terminal_stdout_capabilities_get(NexusTerminalOutputCapabilities *capabilities);

/* ---------------------------------------------------------------------------- */
/* STDIO                                                                        */
/* ---------------------------------------------------------------------------- */

/*
nexus_stdio_stdout_write writes raw bytes to stdout.
*/
extern NError nexus_stdio_stdout_write(const byte *bytes, uint_large byte_length, uint_large *out_bytes_written);

/*
nexus_stdio_stdout_write_cstring writes a null-terminated string to stdout.
*/
extern NError nexus_stdio_stdout_write_cstring(const char *text);

/*
nexus_stdio_stdout_write_formatted formats and writes text to stdout.
*/
extern NError nexus_stdio_stdout_write_formatted(const char *format, ...);

/*
nexus_stdio_stdout_flush flushes stdout.
*/
extern NError nexus_stdio_stdout_flush(void);

/*
nexus_stdio_stderr_write writes raw bytes to stderr.
*/
extern NError nexus_stdio_stderr_write(const byte *bytes, uint_large byte_length, uint_large *out_bytes_written);

/*
nexus_stdio_stderr_write_cstring writes a null-terminated string to stderr.
*/
extern NError nexus_stdio_stderr_write_cstring(const char *text);

/*
nexus_stdio_stderr_write_formatted formats and writes text to stderr.
*/
extern NError nexus_stdio_stderr_write_formatted(const char *format, ...);

/*
nexus_stdio_stderr_flush flushes stderr.
*/
extern NError nexus_stdio_stderr_flush(void);

/*
nexus_stdio_stdin_read_line reads one line from stdin into buffer.

When stdin reaches end-of-file before a line is read, sets out_reached_eof to TRUE.
Interrupted reads are retried so callers can observe external cancellation first.
*/
extern NError nexus_stdio_stdin_read_line(char *buffer, uint_large buffer_max_length, boolean *out_reached_eof);

/*
nexus_stdio_stdin_is_terminal returns TRUE when stdin is connected to a terminal device.
*/
extern boolean nexus_stdio_stdin_is_terminal(void);

/* ---------------------------------------------------------------------------- */
/* STRINGS                                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusStringFormatResult reports the outcome of a bounded string format operation.

written_length: Characters written into the destination buffer, excluding the null terminator.
required_length: Characters required for the full formatted output, excluding the null terminator.
truncated: TRUE when required_length exceeds what could fit within max_string_length.
success: TRUE when formatting completed according to the selected function variant.
*/
typedef struct NexusStringFormatResult {
  uint_large written_length;
  uint_large required_length;
  uint8      truncated;
  uint8      success;
} NexusStringFormatResult;

/*
nexus_strings_string_format formats a string with a `max_string_length`

If the formatted string would be more than the `max_string_length`, this implementation does not write
to the destination buffer and returns success=FALSE with truncated=TRUE.
Otherwise, it returns success=TRUE with written_length equal to required_length.

Performance: prefer `nexus_strings_string_format_with_truncation` because it does not do double work.
*/
extern NexusStringFormatResult nexus_strings_string_format(char *string, uint_large max_string_length, const char *format, ...);

/*
nexus_strings_string_format_with_truncation formats a string with a `max_string_length`

If the formatted string would be more than the `max_string_length`, this implementation stops at the boundary,
null-terminates the destination buffer, and returns truncated=TRUE with success=TRUE.
Otherwise, it returns success=TRUE with written_length equal to required_length.
*/
extern NexusStringFormatResult nexus_strings_string_format_with_truncation(char *string, uint_large max_string_length, const char *format, ...);

/*
nexus_strings_vstring_format formats a vstring with a `max_string_length`

If the formatted string would be more than the `max_string_length`, this implementation does not write
to the destination buffer and returns success=FALSE with truncated=TRUE.
Otherwise, it returns success=TRUE with written_length equal to required_length.

Performance: prefer `nexus_strings_vstring_format_with_truncation` because it does not do double work.
*/
extern NexusStringFormatResult nexus_strings_vstring_format(char *string, uint_large max_string_length, const char *format, va_list args);

/*
nexus_strings_vstring_format_with_truncation formats a vstring with a `max_string_length`

If the formatted string would be more than the `max_string_length`, this implementation stops at the boundary,
null-terminates the destination buffer, and returns truncated=TRUE with success=TRUE.
Otherwise, it returns success=TRUE with written_length equal to required_length.
*/
extern NexusStringFormatResult nexus_strings_vstring_format_with_truncation(char *string, uint_large max_string_length, const char *format,
                                                                            va_list args);

/*
nexus_strings_string_format_required_length returns the character count required to format the
arguments, excluding the null terminator. No destination buffer is written.
*/
extern NexusStringFormatResult nexus_strings_string_format_required_length(const char *format, ...);

/*
nexus_strings_vstring_format_required_length returns the character count required to format the
arguments, excluding the null terminator. No destination buffer is written.
*/
extern NexusStringFormatResult nexus_strings_vstring_format_required_length(const char *format, va_list args);

/*
nexus_strings_bytes_format writes byte_count as a human-readable binary size (B, KiB, MiB, GiB, TiB).
Uses IEC binary prefixes (1024). string must not be NULL and max_string_length must be greater than zero.
*/
extern NexusStringFormatResult nexus_strings_bytes_format(char *string, uint_large max_string_length, uint_large byte_count);

#ifndef NEXUS_STRINGS_QUANTITY_DEFAULT_DECIMAL_PLACES
#  define NEXUS_STRINGS_QUANTITY_DEFAULT_DECIMAL_PLACES 3u
#endif

#ifndef NEXUS_STRINGS_PERCENT_DEFAULT_DECIMAL_PLACES
#  define NEXUS_STRINGS_PERCENT_DEFAULT_DECIMAL_PLACES 2u
#endif

/*
nexus_strings_quantity_format writes value as a decimal SI-scaled unitless quantity.
Uses 1000-based prefixes: "", "K", "M", "G", "T".
*/
extern NexusStringFormatResult nexus_strings_quantity_format(char *string, uint_large max_string_length, uint64 value);

/*
nexus_strings_quantity_format_f_real writes value as a decimal SI-scaled unitless quantity.
Uses 1000-based prefixes: "", "K", "M", "G", "T". Preserves sign; NaN and infinity render as NaN, Inf, and -Inf.
Uses NEXUS_STRINGS_QUANTITY_DEFAULT_DECIMAL_PLACES fractional digits.
*/
extern NexusStringFormatResult nexus_strings_quantity_format_f_real(char *string, uint_large max_string_length, f_real value);

/*
nexus_strings_quantity_format_f_real_precision is like nexus_strings_quantity_format_f_real but uses decimal_places
for fractional output (clamped to 0–9).
*/
extern NexusStringFormatResult nexus_strings_quantity_format_f_real_precision(char *string, uint_large max_string_length, f_real value,
                                                                              uint32 decimal_places);

/*
nexus_strings_percent_format_f_real writes a percentage (0–100 scale) with a trailing percent sign.
Preserves sign; NaN and infinity render as NaN%, Inf%, and -Inf%.
Uses NEXUS_STRINGS_PERCENT_DEFAULT_DECIMAL_PLACES fractional digits.
*/
extern NexusStringFormatResult nexus_strings_percent_format_f_real(char *string, uint_large max_string_length, f_real percent_value);

/*
nexus_strings_percent_format_f_real_precision is like nexus_strings_percent_format_f_real but uses decimal_places
for fractional output (clamped to 0–9).
*/
extern NexusStringFormatResult nexus_strings_percent_format_f_real_precision(char *string, uint_large max_string_length, f_real percent_value,
                                                                             uint32 decimal_places);

/*
nexus_strings_string_replace_non_alphanumeric copies src into dest, replacing each character that is not
an ASCII letter or digit with replacement. dest is always null-terminated within dest_max_len.
*/
extern NexusStringFormatResult nexus_strings_string_replace_non_alphanumeric(char *dest, uint_large dest_max_len, const char *src, char replacement);

#ifndef NEXUS_STRINGS_PREFORMAT_MESSAGE_MAX
#  define NEXUS_STRINGS_PREFORMAT_MESSAGE_MAX 256
#endif

/*
nexus_strings_string_preformat formats into an internal scratch buffer and returns a pointer to it.

The returned pointer is valid until the next call to nexus_strings_string_preformat on the same thread.
Output longer than NEXUS_STRINGS_PREFORMAT_MESSAGE_MAX - 1 characters is truncated. format must not be NULL.
*/
extern const char *nexus_strings_string_preformat(const char *format, ...);

/*
nexus_strings_display_width_get returns the terminal column width of text.

ANSI SGR sequences (ESC [ ... m) contribute zero width. Combining marks (e.g. the
macron on δ̄) contribute zero width. Other UTF-8 codepoints use a single column
unless they fall in a common double-width range (CJK/fullwidth).
*/
extern uint32 nexus_strings_display_width_get(const char *text);

/*
nexus_strings_display_width_prefix_length_get returns the byte length of the
longest prefix of text whose display width is at most max_display_width. The
prefix never splits a UTF-8 codepoint.
*/
extern uint_large nexus_strings_display_width_prefix_length_get(const char *text, uint32 max_display_width);

/*
Performs a safe, bounded copy of a string. Guarantees null-termination.

Truncates when src does not fit and discards the outcome. Prefer
nexus_strings_string_copy_with_truncation when truncation must be detected.

dest, src must not be NULL and dest_max_len must be greater than zero.
*/
extern void nexus_strings_string_copy(char *dest, uint_large dest_max_len, const char *src);

/*
nexus_strings_string_append appends src to dest with bounded null-terminated concatenation.

When the combined string does not fit, dest is truncated and null-terminated.
Returns TRUE when the full suffix was appended.
*/
extern boolean nexus_strings_string_append(char *dest, uint_large dest_max_len, const char *src);

/*
nexus_strings_string_copy_exact copies src into dest when the full string fits.

If src would require more than dest_max_len characters including the null terminator,
dest is not modified and success=FALSE with truncated=TRUE is returned.
Otherwise, success=TRUE with written_length equal to required_length.

Performance: prefer nexus_strings_string_copy_with_truncation because it does not require
knowing src length before writing.
*/
extern NexusStringFormatResult nexus_strings_string_copy_exact(char *dest, uint_large dest_max_len, const char *src);

/*
nexus_strings_string_copy_with_truncation copies src into dest up to dest_max_len - 1 characters.

When src is longer than the destination capacity, the copy is truncated, dest is null-terminated,
and truncated=TRUE with success=TRUE is returned. Otherwise, success=TRUE with written_length
equal to required_length.
*/
extern NexusStringFormatResult nexus_strings_string_copy_with_truncation(char *dest, uint_large dest_max_len, const char *src);

/*
nexus_strings_string_parse_uint8 parses a base-10 unsigned integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the uint8 range.
*/
extern NError nexus_strings_string_parse_uint8(const char *string, uint8 *out_value);

/*
nexus_strings_string_parse_uint16 parses a base-10 unsigned integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the uint16 range.
*/
extern NError nexus_strings_string_parse_uint16(const char *string, uint16 *out_value);

/*
nexus_strings_string_parse_uint32 parses a base-10 unsigned integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the uint32 range.
*/
extern NError nexus_strings_string_parse_uint32(const char *string, uint32 *out_value);

/*
nexus_strings_string_parse_uint64 parses a base-10 unsigned integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the uint64 range.
*/
extern NError nexus_strings_string_parse_uint64(const char *string, uint64 *out_value);

/*
nexus_strings_string_parse_int8 parses a base-10 signed integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the int8 range.
*/
extern NError nexus_strings_string_parse_int8(const char *string, int8 *out_value);

/*
nexus_strings_string_parse_int16 parses a base-10 signed integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the int16 range.
*/
extern NError nexus_strings_string_parse_int16(const char *string, int16 *out_value);

/*
nexus_strings_string_parse_int32 parses a base-10 signed integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the int32 range.
*/
extern NError nexus_strings_string_parse_int32(const char *string, int32 *out_value);

/*
nexus_strings_string_parse_int64 parses a base-10 signed integer string into out_value.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when string is empty,
not numeric, or outside the int64 range.
*/
extern NError nexus_strings_string_parse_int64(const char *string, int64 *out_value);

/*
nexus_strings_string_parse_f_real parses a floating-point string using the active C numeric locale.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_INVALID_ARGUMENT when parsing fails.
Callers that require a stable radix should pair this with nexus_locale_numeric_c_push and nexus_locale_numeric_c_pop.
*/
extern NError nexus_strings_string_parse_real32(const char *string, real32 *out_value);

/*
nexus_strings_string_parse_real64 parses a 64-bit floating-point string using the active C numeric locale.
*/
extern NError nexus_strings_string_parse_real64(const char *string, real64 *out_value);

/*
nexus_strings_string_parse_f_real parses the configured f_real string using the active C numeric locale.
*/
extern NError nexus_strings_string_parse_f_real(const char *string, f_real *out_value);

/*
nexus_strings_string_parse_hex_uint8 parses an unsigned hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_uint8(const char *string, uint8 *out_value);

/*
nexus_strings_string_parse_hex_uint16 parses an unsigned hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_uint16(const char *string, uint16 *out_value);

/*
nexus_strings_string_parse_hex_uint32 parses an unsigned hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_uint32(const char *string, uint32 *out_value);

/*
nexus_strings_string_parse_hex_uint64 parses an unsigned hexadecimal string into out_value.

Accepts an optional 0x or 0X prefix. Returns NEXUS_ERROR_INVALID_ARGUMENT on empty or invalid input.
*/
extern NError nexus_strings_string_parse_hex_uint64(const char *string, uint64 *out_value);

/*
nexus_strings_string_parse_hex_int8 parses a signed hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_int8(const char *string, int8 *out_value);

/*
nexus_strings_string_parse_hex_int16 parses a signed hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_int16(const char *string, int16 *out_value);

/*
nexus_strings_string_parse_hex_int32 parses a signed hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_int32(const char *string, int32 *out_value);

/*
nexus_strings_string_parse_hex_int64 parses a signed hexadecimal string into out_value.
*/
extern NError nexus_strings_string_parse_hex_int64(const char *string, int64 *out_value);

/*
nexus_strings_string_format_hex_uint8 formats value as a lowercase hexadecimal string with 0x prefix.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_uint8(char *string, uint_large max_string_length, uint8 value);

/*
nexus_strings_string_format_hex_uint16 formats value as a lowercase hexadecimal string with 0x prefix.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_uint16(char *string, uint_large max_string_length, uint16 value);

/*
nexus_strings_string_format_hex_uint32 formats value as a lowercase hexadecimal string with 0x prefix.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_uint32(char *string, uint_large max_string_length, uint32 value);

/*
nexus_strings_string_format_hex_uint64 formats value as a lowercase hexadecimal string with 0x prefix.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_uint64(char *string, uint_large max_string_length, uint64 value);

/*
nexus_strings_string_format_hex_f_real_bits formats the IEEE-754 bit pattern of value as hexadecimal text.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_f_real_bits(char *string, uint_large max_string_length, f_real value);

/*
nexus_strings_string_format_hex_real32_bits formats the IEEE-754 bit pattern of value as hexadecimal text.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_real32_bits(char *string, uint_large max_string_length, real32 value);

/*
nexus_strings_string_format_hex_real64_bits formats the IEEE-754 bit pattern of value as hexadecimal text.
*/
extern NexusStringFormatResult nexus_strings_string_format_hex_real64_bits(char *string, uint_large max_string_length, real64 value);

/*
nexus_strings_string_find locates the first occurrence of needle inside haystack.

When found, writes the pointer to the match into out_position when out_position is not NULL, and returns TRUE. Otherwise returns FALSE.
*/
extern boolean nexus_strings_string_find(const char *haystack, const char *needle, const char **out_position);

/*
nexus_strings_string_split_on_first_delimiter splits string at the first delimiter occurrence.

Copies the left and right segments into the provided buffers. Returns NEXUS_ERROR_INVALID_ARGUMENT when
delimiter is absent or either segment does not fit.
*/
extern NError nexus_strings_string_split_on_first_delimiter(const char *string, char delimiter, char *left_buffer, uint_large left_max_length,
                                                            char *right_buffer, uint_large right_max_length);

/*
nexus_strings_string_read_word skips leading whitespace, copies the next token into buffer, and advances cursor.

Returns NEXUS_ERROR_NONE when a token is read. Returns NEXUS_ERROR_INVALID_ARGUMENT when cursor is NULL,
buffer is NULL, or no token remains.
*/
extern NError nexus_strings_string_read_word(const char **cursor, char *buffer, uint_large buffer_max_length);

/*
nexus_strings_string_compare_length compares up to max_length characters of string_a and string_b.

Returns 0 if they are identical up to that length or until a null-terminator is reached.
Returns < 0 if string_a is lexicographically less, or > 0 if greater.
*/
extern int nexus_strings_string_compare_length(const char *string_a, const char *string_b, uint_large max_length);

/*
nexus_strings_string_length gets the current length of a string.

string must not be NULL.
*/
static uint_large nexus_strings_string_length(const char *string) /* NOLINT */ {
  const char *ptr;

  NEXUS_ASSERT_DEBUG(string != NULL);
  ptr = string;
  while (*ptr) {
    ptr++;
  }
  return (uint_large)(ptr - string);
}

/*
Checks if a string exactly starts with the provided prefix.

string and prefix must not be NULL.
*/
static boolean nexus_strings_string_starts_with(const char *string, const char *prefix) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(prefix != NULL);
  while (*prefix) {
    if (*string != *prefix) {
      return FALSE;
    }
    string++;
    prefix++;
  }
  return TRUE;
}

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
static int32 nexus_strings_string_compare(const char *str1, const char *str2) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const unsigned char *)str1 - *(const unsigned char *)str2;
}

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
static int32 nexus_strings_string_compare_unsigned(const unsigned char *str1, const unsigned char *str2) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
static int32 nexus_strings_string_compare_mixed(const unsigned char *str1, const char *str2) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*str1 == *(const unsigned char *)str2)) {
    str1++;
    str2++;
  }
  return *str1 - *(const unsigned char *)str2;
}

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
static int32 nexus_strings_string_compare_mixed_alt(const char *str1, const unsigned char *str2) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*(const unsigned char *)str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const unsigned char *)str1 - *str2;
}

/*
Checks if two strings are equal.

Convenience wrapper around `nexus_strings_string_compare`
*/
static boolean nexus_strings_string_equals(const char *str1, const char *str2) /* NOLINT */ {
  return nexus_strings_string_compare(str1, str2) == 0;
}

/*
Checks if two unsigned strings are equal.

Convenience wrapper around `nexus_strings_string_compare_unsigned`
*/
static boolean nexus_strings_string_equals_unsigned(const unsigned char *str1, const unsigned char *str2) /* NOLINT */ {
  return nexus_strings_string_compare_unsigned(str1, str2) == 0;
}

/*
Checks if two mixed strings are equal.

Convenience wrapper around `nexus_strings_string_compare_mixed`
*/
static boolean nexus_strings_string_equals_mixed(const unsigned char *str1, const char *str2) /* NOLINT */ {
  return nexus_strings_string_compare_mixed(str1, str2) == 0;
}

/*
Checks if two mixed strings are equal.

Convenience wrapper around `nexus_strings_string_compare_mixed_alt`
*/
static boolean nexus_strings_string_equals_mixed_alt(const char *str1, const unsigned char *str2) /* NOLINT */ {
  return nexus_strings_string_compare_mixed_alt(str1, str2) == 0;
}

/* ---------------------------------------------------------------------------- */
/* TABULAR                                                                      */
/* ---------------------------------------------------------------------------- */

#define NEXUS_TABULAR_DEFAULT_LABEL_WIDTH   22
#define NEXUS_TABULAR_DEFAULT_BANNER_WIDTH  97
#define NEXUS_TABULAR_MAX_COLUMNS           32
#define NEXUS_TABULAR_BUFFERED_ROWS_INITIAL 16
#define NEXUS_TABULAR_MAX_LABEL_LENGTH      64
#define NEXUS_TABULAR_MAX_CELL_LENGTH       128

typedef enum NexusTabularAlign {
  NEXUS_TABULAR_ALIGN_LEFT  = 0,
  NEXUS_TABULAR_ALIGN_RIGHT = 1
} NexusTabularAlign;

/*
NexusTabularReport is the output sink for monospace text reports. Use the Vulkan-style two-pass
pattern: begin with sizing_pass TRUE and buffer NULL to measure size, allocate, then render.

During sizing_pass, offset accumulates the full required length (even with a NULL buffer).
During a normal render, offset is only the number of bytes actually written; truncation does
not inflate it (so dumping offset bytes never emits uninitialized tail garbage).
*/
typedef struct NexusTabularReport {
  char      *buffer;
  uint_large max_len;
  uint_large offset;
  boolean    sizing_pass;
} NexusTabularReport;

typedef struct NexusTabularColumn {
  const char       *title;
  uint32            width;
  uint32            fit_width;
  NexusTabularAlign align;
} NexusTabularColumn;

typedef struct NexusTabularBufferedRow {
  char   row_label[NEXUS_TABULAR_MAX_LABEL_LENGTH];
  char   cell_values[NEXUS_TABULAR_MAX_COLUMNS][NEXUS_TABULAR_MAX_CELL_LENGTH];
  char  *cell_pointers[NEXUS_TABULAR_MAX_COLUMNS];
  uint32 cell_count;
} NexusTabularBufferedRow;

/*
NexusTabularTable formats pipe-separated rows from column definitions. The table emits header text,
dash separators, and aligned data rows. Column widths come from the declared width, header title
length, and any values passed to column_fit before the header is written.

When header emission is deferred, rows are staged with column_fit and label_fit until emit is called.
Staged rows live in a heap buffer that grows as needed (no fixed row cap). begin/emit/end own that
buffer; call end (or emit) before the table goes out of scope if rows were staged.
*/
typedef struct NexusTabularTable {
  NexusTabularReport      *report;
  NexusTabularColumn       columns[NEXUS_TABULAR_MAX_COLUMNS];
  uint32                   column_count;
  uint32                   label_width;
  uint32                   label_fit_width;
  boolean                  header_written;
  boolean                  header_deferred;
  const char              *deferred_label_title;
  NexusTabularBufferedRow *buffered_rows;
  uint32                   buffered_row_count;
  uint32                   buffered_row_capacity;
  uint32                   session_tag;
} NexusTabularTable;

extern void       nexus_tabular_report_begin(NexusTabularReport *report, char *buffer, uint_large max_len, boolean sizing_pass);
extern void       nexus_tabular_report_section(NexusTabularReport *report, const char *title, uint32 banner_width);
extern void       nexus_tabular_report_line(NexusTabularReport *report, const char *format, ...);
extern void       nexus_tabular_report_vline(NexusTabularReport *report, const char *format, va_list args);
extern void       nexus_tabular_report_blank_line(NexusTabularReport *report);
extern uint_large nexus_tabular_report_offset_get(const NexusTabularReport *report);
extern uint64     nexus_tabular_report_required_size_get(const NexusTabularReport *report);

extern void   nexus_tabular_table_begin(NexusTabularTable *table, NexusTabularReport *report, uint32 label_width);
extern uint32 nexus_tabular_table_column_add(NexusTabularTable *table, const char *title, uint32 width, NexusTabularAlign align);
extern void   nexus_tabular_table_column_fit(NexusTabularTable *table, uint32 column_index, const char *value);
extern void   nexus_tabular_table_label_fit(NexusTabularTable *table, const char *value);
extern void   nexus_tabular_table_header_defer(NexusTabularTable *table, const char *label_title);
extern void   nexus_tabular_table_row_stage(NexusTabularTable *table, const char *row_label, const char *const *cell_values, uint32 cell_count);
extern void   nexus_tabular_table_emit(NexusTabularTable *table);
extern void   nexus_tabular_table_header_write(NexusTabularTable *table, const char *label_title);
extern void   nexus_tabular_table_row_write(NexusTabularTable *table, const char *row_label, const char *const *cell_values, uint32 cell_count);
extern void   nexus_tabular_table_end(NexusTabularTable *table);

/* ---------------------------------------------------------------------------- */
/* LOCALE                                                                       */
/* ---------------------------------------------------------------------------- */

#ifndef NEXUS_LOCALE_NUMERIC_BUFFER_LENGTH
#  define NEXUS_LOCALE_NUMERIC_BUFFER_LENGTH 64
#endif

/*
NexusLocaleNumericScope stores the previous LC_NUMERIC locale while the C locale is active.
*/
typedef struct NexusLocaleNumericScope {
  char    previous_locale[NEXUS_LOCALE_NUMERIC_BUFFER_LENGTH];
  boolean active;
} NexusLocaleNumericScope;

/*
nexus_locale_numeric_c_push switches LC_NUMERIC to the C locale and records the previous setting in scope.
*/
extern void nexus_locale_numeric_c_push(NexusLocaleNumericScope *scope);

/*
nexus_locale_numeric_c_pop restores LC_NUMERIC from scope when nexus_locale_numeric_c_push succeeded.
*/
extern void nexus_locale_numeric_c_pop(NexusLocaleNumericScope *scope);

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
NexusClockOrigin identifies which OS clock produced a NexusTime value.
*/
typedef enum NexusClockOrigin {
  /*
  NCO_REAL indicates wall-clock UTC epoch time from nexus_time_get_real.
  */
  NCO_REAL,

  /*
  NCO_MONOTONIC indicates a monotonic hardware clock from nexus_time_get_monotonic.
  */
  NCO_MONOTONIC,

  /*
  NCO_COUNT is the number of NexusClockOrigin enum values.
  */
  NCO_COUNT
} NexusClockOrigin;

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
  is defined by clock_origin.
  */
  timestamp time;

  /*
  precision stores the reported precision for the used clock (`clock_getres` semantics).
  This indicates the mathematical trustworthiness of the sub-second field.
  */
  NexusTimePrecision precision;

  /*
  clock_origin records which clock produced this value. Debug builds assert that
  operations combining NexusTime values use matching origins.
  */
  NexusClockOrigin clock_origin;
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
nexus_time_duration_between returns the signed elapsed time from start to end.
The result is negative when end precedes start. Debug builds assert matching clock_origin.
*/
extern NexusDuration nexus_time_duration_between(NexusTime start, NexusTime end);

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

/*
nexus_time_from_local_datetime converts a localized Human Time presentation structure into UTC epoch time.

date_time is interpreted in the current local timezone using the platform calendar rules.
*/
extern NexusTime nexus_time_from_local_datetime(NexusDateTime date_time);

/*
nexus_time_datetime_parse parses a local calendar timestamp string produced by nexus_time_datetime_format.

Returns TRUE when the string matches YYYY-MM-DD HH:MM:SS[.nnnnnnnnn].
*/
extern boolean nexus_time_datetime_parse(const char *string, NexusDateTime *out_date_time);

/*
nexus_time_from_local_datetime_string parses a local calendar timestamp string into UTC epoch time.
*/
extern boolean nexus_time_from_local_datetime_string(const char *string, NexusTime *out_time);

/*
nexus_time_duration_format writes duration as a human-readable interval (ns, us, ms, s).
Uses decimal SI scaling (1000). string must not be NULL and max_string_length must be greater than zero.
*/
extern NexusStringFormatResult nexus_time_duration_format(char *string, uint_large max_string_length, NexusDuration duration);

/*
nexus_time_datetime_format writes date_time as a local calendar timestamp string.
Format: YYYY-MM-DD HH:MM:SS.nnnnnnnnn. string must not be NULL and max_string_length must be greater than zero.
*/
extern NexusStringFormatResult nexus_time_datetime_format(char *string, uint_large max_string_length, NexusDateTime date_time);

/* ---------------------------------------------------------------------------- */
/* PATHS                                                                        */
/* ---------------------------------------------------------------------------- */

#ifndef NEXUS_MAX_PATH_LENGTH
#  define NEXUS_MAX_PATH_LENGTH 1024
#endif

/*
NexusPath encapsulates a platform-agnostic path representation.
*/
typedef struct NexusPath {
  char   buffer[NEXUS_MAX_PATH_LENGTH];
  uint16 length;
} NexusPath;

/*
nexus_paths_path_create initializes a path from a raw C string.
*/
extern NexusPath nexus_paths_path_create(const char *base_path);

/*
nexus_paths_path_append appends a new element to the path, automatically handling separators.
*/
extern void nexus_paths_path_append(NexusPath *path, const char *element);

/*
NexusPathWalkCallback gets called during path walking.
*/
typedef void NexusPathWalkCallback(NexusPath path, void *user_data);

/*
nexus_paths_path_walk walks a path using the provided configuration and calls the provided callback.

If both files_only and dirs_only is set, files_only takes precedence (it does not error)
*/
extern void nexus_paths_path_walk(NexusPath path, NexusPathWalkCallback *callback, void *user_data, boolean recursive, boolean files_only,
                                  boolean dirs_only);

/*
NexusPathList holds paths collected by nexus_paths_path_list_collect or nexus_paths_path_list_collect_allocated.
*/
typedef struct NexusPathList {
  NexusPath *paths;
  uint32     count;
} NexusPathList;

/*
nexus_paths_path_list_collect gathers matching paths using the same flags as nexus_paths_path_walk.

Vulkan-style two-pass usage on the same function:
  Pass 1: Set path_list->paths to NULL. path_list->count is ignored. On return, path_list->count
          holds the number of matching paths.
  Pass 2: Allocate path_list->paths with at least path_list->count elements, keeping count from pass 1.
          On return, paths[0] through paths[count - 1] are populated.

path_list must not be NULL.
*/
extern void nexus_paths_path_list_collect(NexusPath path, boolean recursive, boolean files_only, boolean dirs_only, NexusPathList *path_list);

/*
nexus_paths_path_list_collect_allocated gathers matching paths into a Nexus-owned buffer.
The caller must release the result with nexus_paths_path_list_destroy.
*/
extern NexusPathList nexus_paths_path_list_collect_allocated(NexusPath path, boolean recursive, boolean files_only, boolean dirs_only);

/*
nexus_paths_path_list_destroy releases memory owned by a NexusPathList from nexus_paths_path_list_collect_allocated.
*/
extern void nexus_paths_path_list_destroy(NexusPathList *path_list);

/* Returns a pointer to the base file name within the path buffer. No allocation. */
extern const char *nexus_paths_path_base_name_get(const NexusPath *path);

/*
nexus_paths_path_is_absolute checks whether a path is absolute or relative.
*/
extern boolean nexus_paths_path_is_absolute(NexusPath path);

/*
nexus_paths_path_relative_to_absolute converts a relative path to an absolute path.

Safe to use if the path is already absolute.
*/
extern NexusPath nexus_paths_path_relative_to_absolute(NexusPath path);

/*
nexus_paths_path_absolute_to_relative converts an absolute path to a relative path.

Safe to use if the path is already relative.
*/
extern NexusPath nexus_paths_path_absolute_to_relative(NexusPath path);

/*
nexus_paths_path_is_within_directory returns TRUE when candidate resolves to a path inside boundary_directory.
Both paths are normalized to absolute form before comparison.
*/
extern boolean nexus_paths_path_is_within_directory(NexusPath candidate, NexusPath boundary_directory);

/*
nexus_paths_path_parent returns the parent directory of path.

The result has no trailing separator. Parent of a filesystem root (/ on POSIX, drive root on Windows) is the root itself.
*/
extern NexusPath nexus_paths_path_parent(NexusPath path);

/*
nexus_paths_path_find_ancestor_with_marker walks from start_path toward the filesystem root and returns the first
ancestor directory that contains marker_name as a direct child path.

out_directory receives the ancestor directory when found. start_path is normalized to an absolute path first.
*/
extern boolean nexus_paths_path_find_ancestor_with_marker(NexusPath start_path, const char *marker_name, NexusPath *out_directory);

/* ---------------------------------------------------------------------------- */
/* ENVIRONMENT                                                                  */
/* ---------------------------------------------------------------------------- */

/*
nexus_environment_variable_set sets or replaces an environment variable in the current process.
*/
extern NError nexus_environment_variable_set(const char *name, const char *value);

/*
nexus_environment_variable_get reads an environment variable into buffer.

Returns NEXUS_ERROR_NONE on success. Returns NEXUS_ERROR_FILE_NOT_FOUND when the variable is absent.
*/
extern NError nexus_environment_variable_get(const char *name, char *buffer, uint_large buffer_max_length);

/*
nexus_environment_variable_unset removes an environment variable from the current process.
*/
extern NError nexus_environment_variable_unset(const char *name);

/* ---------------------------------------------------------------------------- */
/* PROCESS                                                                      */
/* ---------------------------------------------------------------------------- */

/*
nexus_process_replace replaces the current process image with executable_path.

argv must be a NULL-terminated array whose first element is the executable path.
When environment is NULL, the current process environment is inherited.
This function does not return on success.
*/
extern NError nexus_process_replace(NexusPath executable_path, char *const *argv, char *const *environment);

/*
NexusProcessSpawnResult reports the exit status of a spawned child process.
*/
typedef struct NexusProcessSpawnResult {
  int32 exit_code;
} NexusProcessSpawnResult;

/*
nexus_process_spawn_wait starts executable_path in a child process and waits for completion.

argv must be a NULL-terminated array whose first element is the executable path.
When environment is NULL, the current process environment is inherited.
*/
extern NError nexus_process_spawn_wait(NexusPath executable_path, char *const *argv, char *const *environment, NexusProcessSpawnResult *result);

/*
nexus_process_id_get returns the current process identifier.
*/
extern uint32 nexus_process_id_get(void);

/* ---------------------------------------------------------------------------- */
/* FILESYSTEM                                                                   */
/* ---------------------------------------------------------------------------- */

/*
Filesystem functions that can fail return NError. Output data is written only through
explicit out-parameters and is valid only when NEXUS_ERROR_NONE is returned.
*/

/*
NexusFileHandle is a handle to a file connection.
*/
typedef void NexusFileHandle;

/*
NexusFileMode encapsulates the modes for opening files.
*/
typedef enum NexusFileMode {
  NFM_READ,
  NFM_READ_BINARY,
  NFM_WRITE,
  NFM_WRITE_BINARY,
  NFM_APPEND,
  NFM_APPEND_BINARY,
  NFM_READ_PLUS,
  NFM_READ_BINARY_PLUS,
  NFM_WRITE_PLUS,
  NFM_WRITE_BINARY_PLUS,
  NFM_APPEND_PLUS,
  NFM_APPEND_BINARY_PLUS
} NexusFileMode;

/*
nexus_filesystem_directory_create creates a single directory.

Returns NEXUS_ERROR_NONE on success or if the directory already exists.
*/
extern NError nexus_filesystem_directory_create(NexusPath directory_path);

/*
nexus_filesystem_directory_create_parents creates directory_path and any missing parent directories.

Returns NEXUS_ERROR_NONE on success or when directory_path already exists as a directory.
*/
extern NError nexus_filesystem_directory_create_parents(NexusPath directory_path);

/*
nexus_filesystem_path_is_dir checks whether a path is a directory.

Sets out_is_dir to FALSE when the path does not exist. Returns a non-zero NError for
failures other than a missing path.
*/
extern NError nexus_filesystem_path_is_dir(NexusPath path, boolean *out_is_dir);

/*
nexus_filesystem_path_exists checks if a path exists.

Sets out_exists to FALSE when the path does not exist. Returns a non-zero NError for
failures other than a missing path.
*/
extern NError nexus_filesystem_path_exists(NexusPath path, boolean *out_exists);

/*
nexus_filesystem_file_delete deletes a file if it exists.

Returns NEXUS_ERROR_NONE when the file is deleted or was already absent.
*/
extern NError nexus_filesystem_file_delete(NexusPath file_path);

/*
nexus_filesystem_file_size_get writes the byte size of a regular file at file_path to out_byte_size.

Returns NEXUS_ERROR_FILE_NOT_FOUND when the path does not exist and NEXUS_ERROR_INVALID_ARGUMENT
when the path is not a regular file.
*/
extern NError nexus_filesystem_file_size_get(NexusPath file_path, uint_large *out_byte_size);

/*
nexus_filesystem_temp_directory_get writes the platform temporary directory path into buffer.
*/
extern NError nexus_filesystem_temp_directory_get(char *buffer, uint_large buffer_max_length);

/*
nexus_filesystem_directory_delete deletes a directory if it exists.

When recursive is not set to true, this function fails when the directory is not empty.

Returns NEXUS_ERROR_NONE when the directory is deleted or was already absent.
*/
extern NError nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive);

/*
nexus_filesystem_file_open opens a connection to a given file_path.

On success, writes the handle to out_file_handle and returns NEXUS_ERROR_NONE.
*/
extern NError nexus_filesystem_file_open(NexusPath file_path, NexusFileMode mode, NexusFileHandle **out_file_handle);

/*
nexus_filesystem_file_close closes a currently opened file-connection.
*/
extern NError nexus_filesystem_file_close(NexusFileHandle *file_handle);

/*
nexus_filesystem_file_rename renames/moves a file.
*/
extern NError nexus_filesystem_file_rename(NexusPath old_path, NexusPath new_path);

/*
nexus_filesystem_file_ensure_parent_directory creates every missing parent directory of file_path.

file_path is treated as a file path: only its parent directory tree is created, not file_path itself.
Returns NEXUS_ERROR_NONE when the parent already exists as a directory.
*/
extern NError nexus_filesystem_file_ensure_parent_directory(NexusPath file_path);

/*
nexus_filesystem_file_copy copies the contents of source_path onto destination_path.

Creates missing parent directories of destination_path. Overwrites destination_path when it already
exists. Returns NEXUS_ERROR_FILE_NOT_FOUND when source_path is absent.
*/
extern NError nexus_filesystem_file_copy(NexusPath source_path, NexusPath destination_path);

/*
nexus_filesystem_file_write writes bytes to an opened file.

Writes the number of bytes written to out_bytes_written. A short write without a
stream error is not treated as failure.
*/
extern NError nexus_filesystem_file_write(NexusFileHandle *file_handle, const byte *bytes, uint_large length, uint_large *out_bytes_written);

/*
nexus_filesystem_file_flush flushes a file.
*/
extern NError nexus_filesystem_file_flush(NexusFileHandle *file_handle);

/*
nexus_filesystem_file_read reads up to byte_length bytes from start_byte into buffer.

Writes the number of bytes read to out_bytes_read. A short read at EOF is not an error.
*/
extern NError nexus_filesystem_file_read(NexusFileHandle *file_handle, byte *buffer, uint32 start_byte, uint_large byte_length,
                                         uint_large *out_bytes_read);

/*
nexus_filesystem_file_read_line reads one line from an opened text file into buffer.

The line terminator is not copied. A short read at EOF returns the bytes read with NEXUS_ERROR_NONE.
Returns NEXUS_ERROR_INVALID_ARGUMENT when buffer_max_length is zero.
*/
extern NError nexus_filesystem_file_read_line(NexusFileHandle *file_handle, char *buffer, uint_large buffer_max_length, uint_large *out_bytes_read);

/* ---------------------------------------------------------------------------- */
/* MEMORY                                                                       */
/* ---------------------------------------------------------------------------- */

/*
NEXUS_MEMORY_PREFETCH_LOCALITY_* map to temporal-locality hints for nexus_memory_prefetch.

Higher locality keeps the line closer to the core; lower locality minimizes cache pollution.
*/
#define NEXUS_MEMORY_PREFETCH_LOCALITY_NONE   0
#define NEXUS_MEMORY_PREFETCH_LOCALITY_LOW    1
#define NEXUS_MEMORY_PREFETCH_LOCALITY_MEDIUM 2
#define NEXUS_MEMORY_PREFETCH_LOCALITY_HIGH   3

/*
NEXUS_MEMORY_PREFETCH hints the CPU to load the cache line containing address before it is accessed.

read_write: FALSE prepares for read; TRUE prepares for read-modify-write.
locality: one of NEXUS_MEMORY_PREFETCH_LOCALITY_*.
No-op when the platform has no prefetch intrinsic.
*/
#if defined(__GNUC__) || defined(__clang__)
#  define NEXUS_MEMORY_PREFETCH(address, read_write, locality) __builtin_prefetch((const void *)(address), (read_write) != FALSE, (int)(locality))
#elif defined(_MSC_VER)
#  include <intrin.h>
#  include <xmmintrin.h>
#  define NEXUS_MEMORY_PREFETCH(address, read_write, locality)                                                                                       \
    do {                                                                                                                                             \
      if ((read_write) != FALSE) {                                                                                                                   \
        _m_prefetchw((void *)(address));                                                                                                             \
      } else if ((locality) >= NEXUS_MEMORY_PREFETCH_LOCALITY_MEDIUM) {                                                                              \
        _mm_prefetch((const char *)(address), _MM_HINT_T0);                                                                                          \
      } else if ((locality) >= NEXUS_MEMORY_PREFETCH_LOCALITY_LOW) {                                                                                 \
        _mm_prefetch((const char *)(address), _MM_HINT_T1);                                                                                          \
      } else {                                                                                                                                       \
        _mm_prefetch((const char *)(address), _MM_HINT_T2);                                                                                          \
      }                                                                                                                                              \
    } while (0)
#else
#  define NEXUS_MEMORY_PREFETCH(address, read_write, locality) ((void)(address), (void)(read_write), (void)(locality))
#endif

/*
nexus_memory_bytes_copy copies byte_count bytes from src into dest.

dest and src must not be NULL when byte_count is greater than zero.
The regions must not overlap.

When NEXUS_MEMORY_DEBUG_ENABLED is set, this routes through nexus_debug_mem_bytes_copy so dest and
src are checked against tracked heap allocations and a memory-debugger log event is emitted when a
log callback is installed.
*/
#if NEXUS_MEMORY_DEBUG_ENABLED && !defined(NEXUS_MEMORY_DEBUG_IMPLEMENTATION)
#  define nexus_memory_bytes_copy(dest, src, byte_count) nexus_debug_mem_bytes_copy((dest), (src), (byte_count), __FILE__, __LINE__)
#else
static void nexus_memory_bytes_copy(void *dest, const void *src, uint_large byte_count) /* NOLINT */ {
  if (byte_count == 0) {
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(src != NULL);

  memcpy(dest, src, (size_t)byte_count);
}
#endif

/*
nexus_memory_bytes_set writes byte into each of byte_count bytes starting at dest.

dest must not be NULL when byte_count is greater than zero.

When NEXUS_MEMORY_DEBUG_ENABLED is set, this routes through nexus_debug_mem_bytes_set so dest is
checked against tracked heap allocations and a memory-debugger log event is emitted when a log
callback is installed.
*/
#if NEXUS_MEMORY_DEBUG_ENABLED && !defined(NEXUS_MEMORY_DEBUG_IMPLEMENTATION)
#  define nexus_memory_bytes_set(dest, byte, byte_count) nexus_debug_mem_bytes_set((dest), (byte), (byte_count), __FILE__, __LINE__)
#else
static void nexus_memory_bytes_set(void *dest, uint8 byte, uint_large byte_count) /* NOLINT */ {
  if (byte_count == 0) {
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);

  memset(dest, (int)byte, (size_t)byte_count);
}
#endif

/*
nexus_memory_bytes_clear writes zero into each of byte_count bytes starting at dest.

dest must not be NULL when byte_count is greater than zero.

When NEXUS_MEMORY_DEBUG_ENABLED is set, this routes through nexus_debug_mem_bytes_clear so dest is
checked against tracked heap allocations and a memory-debugger log event is emitted when a log
callback is installed.
*/
#if NEXUS_MEMORY_DEBUG_ENABLED && !defined(NEXUS_MEMORY_DEBUG_IMPLEMENTATION)
#  define nexus_memory_bytes_clear(dest, byte_count) nexus_debug_mem_bytes_clear((dest), (byte_count), __FILE__, __LINE__)
#else
static void nexus_memory_bytes_clear(void *dest, uint_large byte_count) /* NOLINT */ {
  nexus_memory_bytes_set(dest, 0, byte_count);
}
#endif

/* ---------------------------------------------------------------------------- */
/* BITS                                                                        */
/* ---------------------------------------------------------------------------- */

/*
Endian-safe encoders and decoders for packed binary data.

LSB functions treat bytes[0] as the least significant byte (little-endian wire order).
MSB functions treat bytes[0] as the most significant byte (big-endian wire order).

Integer signed variants cast two's complement bit patterns directly.
Floating-point variants reinterpret IEEE-754 bit patterns via union conversion.

out and bytes buffers must not be NULL. Each function writes or reads exactly
sizeof(type) bytes.
*/

/* --- Bit Reinterpretation Helpers --- */

static uint32 nexus_bits_uint32_from_real32(real32 value) /* NOLINT */ {
  union {
    real32 value;
    uint32 bit_pattern;
  } converter;

  converter.value = value;
  return converter.bit_pattern;
}

static real32 nexus_bits_real32_from_uint32(uint32 bits) /* NOLINT */ {
  union {
    uint32 bit_pattern;
    real32 value;
  } converter;

  converter.bit_pattern = bits;
  return converter.value;
}

static uint64 nexus_bits_uint64_from_real64(real64 value) /* NOLINT */ {
  union {
    real64 value;
    uint64 bit_pattern;
  } converter;

  converter.value = value;
  return converter.bit_pattern;
}

static real64 nexus_bits_real64_from_uint64(uint64 bits) /* NOLINT */ {
  union {
    uint64 bit_pattern;
    real64 value;
  } converter;

  converter.bit_pattern = bits;
  return converter.value;
}

static uint32 nexus_bits_uint32_from_f_real(f_real value) /* NOLINT */ {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (uint32)nexus_bits_uint64_from_real64((real64)value);
#else
  return nexus_bits_uint32_from_real32((real32)value);
#endif
}

static uint64 nexus_bits_uint64_from_f_real(f_real value) /* NOLINT */ {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_bits_uint64_from_real64((real64)value);
#else
  return (uint64)nexus_bits_uint32_from_real32((real32)value);
#endif
}

static f_real nexus_bits_f_real_from_uint32(uint32 bits) /* NOLINT */ {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (f_real)nexus_bits_real64_from_uint64((uint64)bits);
#else
  return (f_real)nexus_bits_real32_from_uint32(bits);
#endif
}

static f_real nexus_bits_f_real_from_uint64(uint64 bits) /* NOLINT */ {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return (f_real)nexus_bits_real64_from_uint64(bits);
#else
  return (f_real)nexus_bits_real32_from_uint32((uint32)bits);
#endif
}

/* --- Little-Endian (LSB) Encoders (Packing) --- */

static void nexus_bits_uint16_to_bytes_lsb(byte *out, uint16 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out != NULL);
  out[0] = (byte)(value & 0xFFu);
  out[1] = (byte)((value >> 8) & 0xFFu);
}

static void nexus_bits_uint32_to_bytes_lsb(byte *out, uint32 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out != NULL);
  out[0] = (byte)(value & 0xFFu);
  out[1] = (byte)((value >> 8) & 0xFFu);
  out[2] = (byte)((value >> 16) & 0xFFu);
  out[3] = (byte)((value >> 24) & 0xFFu);
}

static void nexus_bits_uint64_to_bytes_lsb(byte *out, uint64 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out != NULL);
  out[0] = (byte)(value & 0xFFu);
  out[1] = (byte)((value >> 8) & 0xFFu);
  out[2] = (byte)((value >> 16) & 0xFFu);
  out[3] = (byte)((value >> 24) & 0xFFu);
  out[4] = (byte)((value >> 32) & 0xFFu);
  out[5] = (byte)((value >> 40) & 0xFFu);
  out[6] = (byte)((value >> 48) & 0xFFu);
  out[7] = (byte)((value >> 56) & 0xFFu);
}

static void nexus_bits_int16_to_bytes_lsb(byte *out, int16 value) /* NOLINT */ {
  nexus_bits_uint16_to_bytes_lsb(out, (uint16)value);
}

static void nexus_bits_int32_to_bytes_lsb(byte *out, int32 value) /* NOLINT */ {
  nexus_bits_uint32_to_bytes_lsb(out, (uint32)value);
}

static void nexus_bits_int64_to_bytes_lsb(byte *out, int64 value) /* NOLINT */ {
  nexus_bits_uint64_to_bytes_lsb(out, (uint64)value);
}

static void nexus_bits_real32_to_bytes_lsb(byte *out, real32 value) /* NOLINT */ {
  nexus_bits_uint32_to_bytes_lsb(out, nexus_bits_uint32_from_real32(value));
}

static void nexus_bits_real64_to_bytes_lsb(byte *out, real64 value) /* NOLINT */ {
  nexus_bits_uint64_to_bytes_lsb(out, nexus_bits_uint64_from_real64(value));
}

/* --- Little-Endian (LSB) Decoders (Unpacking) --- */

static uint16 nexus_bits_uint16_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (uint16)((uint16)bytes[0] | ((uint16)bytes[1] << 8));
}

static uint32 nexus_bits_uint32_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (uint32)bytes[0] | ((uint32)bytes[1] << 8) | ((uint32)bytes[2] << 16) | ((uint32)bytes[3] << 24);
}

static uint64 nexus_bits_uint64_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (uint64)bytes[0] | ((uint64)bytes[1] << 8) | ((uint64)bytes[2] << 16) | ((uint64)bytes[3] << 24) | ((uint64)bytes[4] << 32) |
         ((uint64)bytes[5] << 40) | ((uint64)bytes[6] << 48) | ((uint64)bytes[7] << 56);
}

static int16 nexus_bits_int16_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  return (int16)nexus_bits_uint16_from_bytes_lsb(bytes);
}

static int32 nexus_bits_int32_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  return (int32)nexus_bits_uint32_from_bytes_lsb(bytes);
}

static int64 nexus_bits_int64_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  return (int64)nexus_bits_uint64_from_bytes_lsb(bytes);
}

static real32 nexus_bits_real32_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  return nexus_bits_real32_from_uint32(nexus_bits_uint32_from_bytes_lsb(bytes));
}

static real64 nexus_bits_real64_from_bytes_lsb(const byte *bytes) /* NOLINT */ {
  return nexus_bits_real64_from_uint64(nexus_bits_uint64_from_bytes_lsb(bytes));
}

/* --- Big-Endian (MSB) Encoders (Packing) --- */

static void nexus_bits_uint16_to_bytes_msb(byte *out, uint16 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out != NULL);
  out[0] = (byte)((value >> 8) & 0xFFu);
  out[1] = (byte)(value & 0xFFu);
}

static void nexus_bits_uint32_to_bytes_msb(byte *out, uint32 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out != NULL);
  out[0] = (byte)((value >> 24) & 0xFFu);
  out[1] = (byte)((value >> 16) & 0xFFu);
  out[2] = (byte)((value >> 8) & 0xFFu);
  out[3] = (byte)(value & 0xFFu);
}

static void nexus_bits_uint64_to_bytes_msb(byte *out, uint64 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out != NULL);
  out[0] = (byte)((value >> 56) & 0xFFu);
  out[1] = (byte)((value >> 48) & 0xFFu);
  out[2] = (byte)((value >> 40) & 0xFFu);
  out[3] = (byte)((value >> 32) & 0xFFu);
  out[4] = (byte)((value >> 24) & 0xFFu);
  out[5] = (byte)((value >> 16) & 0xFFu);
  out[6] = (byte)((value >> 8) & 0xFFu);
  out[7] = (byte)(value & 0xFFu);
}

static void nexus_bits_int16_to_bytes_msb(byte *out, int16 value) /* NOLINT */ {
  nexus_bits_uint16_to_bytes_msb(out, (uint16)value);
}

static void nexus_bits_int32_to_bytes_msb(byte *out, int32 value) /* NOLINT */ {
  nexus_bits_uint32_to_bytes_msb(out, (uint32)value);
}

static void nexus_bits_int64_to_bytes_msb(byte *out, int64 value) /* NOLINT */ {
  nexus_bits_uint64_to_bytes_msb(out, (uint64)value);
}

static void nexus_bits_real32_to_bytes_msb(byte *out, real32 value) /* NOLINT */ {
  nexus_bits_uint32_to_bytes_msb(out, nexus_bits_uint32_from_real32(value));
}

static void nexus_bits_real64_to_bytes_msb(byte *out, real64 value) /* NOLINT */ {
  nexus_bits_uint64_to_bytes_msb(out, nexus_bits_uint64_from_real64(value));
}

/* --- Big-Endian (MSB) Decoders (Unpacking) --- */

static uint16 nexus_bits_uint16_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return (uint16)(((uint16)bytes[0] << 8) | (uint16)bytes[1]);
}

static uint32 nexus_bits_uint32_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return ((uint32)bytes[0] << 24) | ((uint32)bytes[1] << 16) | ((uint32)bytes[2] << 8) | (uint32)bytes[3];
}

static uint64 nexus_bits_uint64_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  return ((uint64)bytes[0] << 56) | ((uint64)bytes[1] << 48) | ((uint64)bytes[2] << 40) | ((uint64)bytes[3] << 32) | ((uint64)bytes[4] << 24) |
         ((uint64)bytes[5] << 16) | ((uint64)bytes[6] << 8) | (uint64)bytes[7];
}

static int16 nexus_bits_int16_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  return (int16)nexus_bits_uint16_from_bytes_msb(bytes);
}

static int32 nexus_bits_int32_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  return (int32)nexus_bits_uint32_from_bytes_msb(bytes);
}

static int64 nexus_bits_int64_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  return (int64)nexus_bits_uint64_from_bytes_msb(bytes);
}

static real32 nexus_bits_real32_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  return nexus_bits_real32_from_uint32(nexus_bits_uint32_from_bytes_msb(bytes));
}

static real64 nexus_bits_real64_from_bytes_msb(const byte *bytes) /* NOLINT */ {
  return nexus_bits_real64_from_uint64(nexus_bits_uint64_from_bytes_msb(bytes));
}

/* --- Bitfield Manipulation Operations --- */

static boolean nexus_bits_uint8_get(uint8 value, uint32 bit_index) /* NOLINT */ {
  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 8u, "bit_index out of range for uint8.");
  return (boolean)((value >> bit_index) & 1u);
}

static uint8 nexus_bits_uint8_set(uint8 value, uint32 bit_index, boolean bit_value) /* NOLINT */ {
  const uint8 mask = (uint8)(1u << bit_index);

  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 8u, "bit_index out of range for uint8.");

  if (bit_value) {
    return (uint8)(value | mask);
  }

  return (uint8)(value & (uint8)~mask);
}

static boolean nexus_bits_uint16_get(uint16 value, uint32 bit_index) /* NOLINT */ {
  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 16u, "bit_index out of range for uint16.");
  return (boolean)((value >> bit_index) & 1u);
}

static uint16 nexus_bits_uint16_set(uint16 value, uint32 bit_index, boolean bit_value) /* NOLINT */ {
  const uint16 mask = (uint16)(1u << bit_index);

  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 16u, "bit_index out of range for uint16.");

  if (bit_value) {
    return (uint16)(value | mask);
  }

  return (uint16)(value & (uint16)~mask);
}

static boolean nexus_bits_uint32_get(uint32 value, uint32 bit_index) /* NOLINT */ {
  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 32u, "bit_index out of range for uint32.");
  return (boolean)((value >> bit_index) & 1u);
}

static uint32 nexus_bits_uint32_set(uint32 value, uint32 bit_index, boolean bit_value) /* NOLINT */ {
  const uint32 mask = (uint32)(1u << bit_index);

  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 32u, "bit_index out of range for uint32.");

  if (bit_value) {
    return value | mask;
  }

  return value & ~mask;
}

static boolean nexus_bits_uint64_get(uint64 value, uint32 bit_index) /* NOLINT */ {
  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 64u, "bit_index out of range for uint64.");
  return (boolean)((value >> bit_index) & 1u);
}

static uint64 nexus_bits_uint64_set(uint64 value, uint32 bit_index, boolean bit_value) /* NOLINT */ {
  const uint64 mask = ((uint64)1) << bit_index;

  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < 64u, "bit_index out of range for uint64.");

  if (bit_value) {
    return value | mask;
  }

  return value & ~mask;
}

static boolean nexus_bits_uint_large_get(uint_large value, uint32 bit_index) /* NOLINT */ {
  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < (uint32)(NEXUS_SIZEOF(uint_large) * 8u), "bit_index out of range for uint_large.");
  return (boolean)((value >> bit_index) & 1u);
}

static uint_large nexus_bits_uint_large_set(uint_large value, uint32 bit_index, boolean bit_value) /* NOLINT */ {
  const uint_large mask = ((uint_large)1) << bit_index;

  NEXUS_ASSERT_MESSAGE_DEBUG(bit_index < (uint32)(NEXUS_SIZEOF(uint_large) * 8u), "bit_index out of range for uint_large.");

  if (bit_value) {
    return value | mask;
  }

  return value & ~mask;
}

/* ---------------------------------------------------------------------------- */
/* SUB-WORD PACKING & UNPACKING                                                 */
/* ---------------------------------------------------------------------------- */

/* --- Half Extraction Helpers (Architecture-Agnostic) --- */

static uint8 nexus_bits_uint16_lo_u8(uint16 value) /* NOLINT */ {
  return (uint8)(value & 0xFFu);
}

static uint8 nexus_bits_uint16_hi_u8(uint16 value) /* NOLINT */ {
  return (uint8)((value >> 8) & 0xFFu);
}

static uint16 nexus_bits_uint32_lo_u16(uint32 value) /* NOLINT */ {
  return (uint16)(value & 0xFFFFu);
}

static uint16 nexus_bits_uint32_hi_u16(uint32 value) /* NOLINT */ {
  return (uint16)((value >> 16) & 0xFFFFu);
}

static uint32 nexus_bits_uint64_lo_u32(uint64 value) /* NOLINT */ {
  return (uint32)(value & 0xFFFFFFFFULL);
}

static uint32 nexus_bits_uint64_hi_u32(uint64 value) /* NOLINT */ {
  return (uint32)((value >> 32) & 0xFFFFFFFFULL);
}

/* --- 16-Bit Operations (2x uint8) --- */

static uint16 nexus_bits_uint16_pack_2u8_lsb(uint8 b0, uint8 b1) /* NOLINT */ {
  return (uint16)((uint16)b0 | ((uint16)b1 << 8));
}

static uint16 nexus_bits_uint16_pack_2u8_msb(uint8 b0, uint8 b1) /* NOLINT */ {
  return (uint16)(((uint16)b0 << 8) | (uint16)b1);
}

static void nexus_bits_uint16_unpack_2u8_lsb(uint16 value, uint8 *out_b0, uint8 *out_b1) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_b0 != NULL);
  NEXUS_ASSERT_DEBUG(out_b1 != NULL);

  *out_b0 = (uint8)(value & 0xFFu);
  *out_b1 = (uint8)((value >> 8) & 0xFFu);
}

static void nexus_bits_uint16_unpack_2u8_msb(uint16 value, uint8 *out_b0, uint8 *out_b1) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_b0 != NULL);
  NEXUS_ASSERT_DEBUG(out_b1 != NULL);

  *out_b0 = (uint8)((value >> 8) & 0xFFu);
  *out_b1 = (uint8)(value & 0xFFu);
}

/* --- 32-Bit Operations (2x uint16, 4x uint8) --- */

static uint32 nexus_bits_uint32_pack_2u16_lsb(uint16 w0, uint16 w1) /* NOLINT */ {
  return (uint32)w0 | ((uint32)w1 << 16);
}

static uint32 nexus_bits_uint32_pack_2u16_msb(uint16 w0, uint16 w1) /* NOLINT */ {
  return ((uint32)w0 << 16) | (uint32)w1;
}

static void nexus_bits_uint32_unpack_2u16_lsb(uint32 value, uint16 *out_w0, uint16 *out_w1) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_w0 != NULL);
  NEXUS_ASSERT_DEBUG(out_w1 != NULL);

  *out_w0 = (uint16)(value & 0xFFFFu);
  *out_w1 = (uint16)((value >> 16) & 0xFFFFu);
}

static void nexus_bits_uint32_unpack_2u16_msb(uint32 value, uint16 *out_w0, uint16 *out_w1) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_w0 != NULL);
  NEXUS_ASSERT_DEBUG(out_w1 != NULL);

  *out_w0 = (uint16)((value >> 16) & 0xFFFFu);
  *out_w1 = (uint16)(value & 0xFFFFu);
}

static uint32 nexus_bits_uint32_pack_4u8_lsb(uint8 b0, uint8 b1, uint8 b2, uint8 b3) /* NOLINT */ {
  return (uint32)b0 | ((uint32)b1 << 8) | ((uint32)b2 << 16) | ((uint32)b3 << 24);
}

static uint32 nexus_bits_uint32_pack_4u8_msb(uint8 b0, uint8 b1, uint8 b2, uint8 b3) /* NOLINT */ {
  return ((uint32)b0 << 24) | ((uint32)b1 << 16) | ((uint32)b2 << 8) | (uint32)b3;
}

static void nexus_bits_uint32_unpack_4u8_lsb(uint32 value, uint8 *out_b0, uint8 *out_b1, uint8 *out_b2, uint8 *out_b3) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_b0 != NULL);
  NEXUS_ASSERT_DEBUG(out_b1 != NULL);
  NEXUS_ASSERT_DEBUG(out_b2 != NULL);
  NEXUS_ASSERT_DEBUG(out_b3 != NULL);

  *out_b0 = (uint8)(value & 0xFFu);
  *out_b1 = (uint8)((value >> 8) & 0xFFu);
  *out_b2 = (uint8)((value >> 16) & 0xFFu);
  *out_b3 = (uint8)((value >> 24) & 0xFFu);
}

static void nexus_bits_uint32_unpack_4u8_msb(uint32 value, uint8 *out_b0, uint8 *out_b1, uint8 *out_b2, uint8 *out_b3) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_b0 != NULL);
  NEXUS_ASSERT_DEBUG(out_b1 != NULL);
  NEXUS_ASSERT_DEBUG(out_b2 != NULL);
  NEXUS_ASSERT_DEBUG(out_b3 != NULL);

  *out_b0 = (uint8)((value >> 24) & 0xFFu);
  *out_b1 = (uint8)((value >> 16) & 0xFFu);
  *out_b2 = (uint8)((value >> 8) & 0xFFu);
  *out_b3 = (uint8)(value & 0xFFu);
}

/* --- 64-Bit Operations (2x uint32, 4x uint16, 8x uint8) --- */

static uint64 nexus_bits_uint64_pack_2u32_lsb(uint32 d0, uint32 d1) /* NOLINT */ {
  return (uint64)d0 | ((uint64)d1 << 32);
}

static uint64 nexus_bits_uint64_pack_2u32_msb(uint32 d0, uint32 d1) /* NOLINT */ {
  return ((uint64)d0 << 32) | (uint64)d1;
}

static void nexus_bits_uint64_unpack_2u32_lsb(uint64 value, uint32 *out_d0, uint32 *out_d1) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_d0 != NULL);
  NEXUS_ASSERT_DEBUG(out_d1 != NULL);

  *out_d0 = (uint32)(value & 0xFFFFFFFFULL);
  *out_d1 = (uint32)((value >> 32) & 0xFFFFFFFFULL);
}

static void nexus_bits_uint64_unpack_2u32_msb(uint64 value, uint32 *out_d0, uint32 *out_d1) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_d0 != NULL);
  NEXUS_ASSERT_DEBUG(out_d1 != NULL);

  *out_d0 = (uint32)((value >> 32) & 0xFFFFFFFFULL);
  *out_d1 = (uint32)(value & 0xFFFFFFFFULL);
}

static uint64 nexus_bits_uint64_pack_4u16_lsb(uint16 w0, uint16 w1, uint16 w2, uint16 w3) /* NOLINT */ {
  return (uint64)w0 | ((uint64)w1 << 16) | ((uint64)w2 << 32) | ((uint64)w3 << 48);
}

static uint64 nexus_bits_uint64_pack_4u16_msb(uint16 w0, uint16 w1, uint16 w2, uint16 w3) /* NOLINT */ {
  return ((uint64)w0 << 48) | ((uint64)w1 << 32) | ((uint64)w2 << 16) | (uint64)w3;
}

static void nexus_bits_uint64_unpack_4u16_lsb(uint64 value, uint16 *out_w0, uint16 *out_w1, uint16 *out_w2, uint16 *out_w3) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_w0 != NULL);
  NEXUS_ASSERT_DEBUG(out_w1 != NULL);
  NEXUS_ASSERT_DEBUG(out_w2 != NULL);
  NEXUS_ASSERT_DEBUG(out_w3 != NULL);

  *out_w0 = (uint16)(value & 0xFFFFu);
  *out_w1 = (uint16)((value >> 16) & 0xFFFFu);
  *out_w2 = (uint16)((value >> 32) & 0xFFFFu);
  *out_w3 = (uint16)((value >> 48) & 0xFFFFu);
}

static void nexus_bits_uint64_unpack_4u16_msb(uint64 value, uint16 *out_w0, uint16 *out_w1, uint16 *out_w2, uint16 *out_w3) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_w0 != NULL);
  NEXUS_ASSERT_DEBUG(out_w1 != NULL);
  NEXUS_ASSERT_DEBUG(out_w2 != NULL);
  NEXUS_ASSERT_DEBUG(out_w3 != NULL);

  *out_w0 = (uint16)((value >> 48) & 0xFFFFu);
  *out_w1 = (uint16)((value >> 32) & 0xFFFFu);
  *out_w2 = (uint16)((value >> 16) & 0xFFFFu);
  *out_w3 = (uint16)(value & 0xFFFFu);
}

static uint64 nexus_bits_uint64_pack_8u8_lsb(uint8 b0, uint8 b1, uint8 b2, uint8 b3, /* NOLINT */
                                             uint8 b4, uint8 b5, uint8 b6, uint8 b7) /* NOLINT */ {
  return (uint64)b0 | ((uint64)b1 << 8) | ((uint64)b2 << 16) | ((uint64)b3 << 24) | ((uint64)b4 << 32) | ((uint64)b5 << 40) | ((uint64)b6 << 48) |
         ((uint64)b7 << 56);
}

static uint64 nexus_bits_uint64_pack_8u8_msb(uint8 b0, uint8 b1, uint8 b2, uint8 b3, /* NOLINT */
                                             uint8 b4, uint8 b5, uint8 b6, uint8 b7) /* NOLINT */ {
  return ((uint64)b0 << 56) | ((uint64)b1 << 48) | ((uint64)b2 << 40) | ((uint64)b3 << 32) | ((uint64)b4 << 24) | ((uint64)b5 << 16) |
         ((uint64)b6 << 8) | (uint64)b7;
}

static void nexus_bits_uint64_unpack_8u8_lsb(uint64 value, uint8 *out_b0, uint8 *out_b1, uint8 *out_b2, uint8 *out_b3, /* NOLINT */
                                             uint8 *out_b4, uint8 *out_b5, uint8 *out_b6, uint8 *out_b7) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_b0 != NULL);
  NEXUS_ASSERT_DEBUG(out_b1 != NULL);
  NEXUS_ASSERT_DEBUG(out_b2 != NULL);
  NEXUS_ASSERT_DEBUG(out_b3 != NULL);
  NEXUS_ASSERT_DEBUG(out_b4 != NULL);
  NEXUS_ASSERT_DEBUG(out_b5 != NULL);
  NEXUS_ASSERT_DEBUG(out_b6 != NULL);
  NEXUS_ASSERT_DEBUG(out_b7 != NULL);

  *out_b0 = (uint8)(value & 0xFFu);
  *out_b1 = (uint8)((value >> 8) & 0xFFu);
  *out_b2 = (uint8)((value >> 16) & 0xFFu);
  *out_b3 = (uint8)((value >> 24) & 0xFFu);
  *out_b4 = (uint8)((value >> 32) & 0xFFu);
  *out_b5 = (uint8)((value >> 40) & 0xFFu);
  *out_b6 = (uint8)((value >> 48) & 0xFFu);
  *out_b7 = (uint8)((value >> 56) & 0xFFu);
}

static void nexus_bits_uint64_unpack_8u8_msb(uint64 value, uint8 *out_b0, uint8 *out_b1, uint8 *out_b2, uint8 *out_b3, /* NOLINT */
                                             uint8 *out_b4, uint8 *out_b5, uint8 *out_b6, uint8 *out_b7) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(out_b0 != NULL);
  NEXUS_ASSERT_DEBUG(out_b1 != NULL);
  NEXUS_ASSERT_DEBUG(out_b2 != NULL);
  NEXUS_ASSERT_DEBUG(out_b3 != NULL);
  NEXUS_ASSERT_DEBUG(out_b4 != NULL);
  NEXUS_ASSERT_DEBUG(out_b5 != NULL);
  NEXUS_ASSERT_DEBUG(out_b6 != NULL);
  NEXUS_ASSERT_DEBUG(out_b7 != NULL);

  *out_b0 = (uint8)((value >> 56) & 0xFFu);
  *out_b1 = (uint8)((value >> 48) & 0xFFu);
  *out_b2 = (uint8)((value >> 40) & 0xFFu);
  *out_b3 = (uint8)((value >> 32) & 0xFFu);
  *out_b4 = (uint8)((value >> 24) & 0xFFu);
  *out_b5 = (uint8)((value >> 16) & 0xFFu);
  *out_b6 = (uint8)((value >> 8) & 0xFFu);
  *out_b7 = (uint8)(value & 0xFFu);
}

/* --- Bit Count & Bit Scan Operations --- */

static uint64 nexus_bits_uint64_clear_lowest_set(uint64 value) /* NOLINT */ {
  return value & (value - 1ULL);
}

static uint32 nexus_bits_uint64_trailing_zeros(uint64 value) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(value != 0ULL);

#if defined(__GNUC__) || defined(__clang__)
  return (uint32)__builtin_ctzll(value);
#else
  {
    static const uint8 index64[64] = {0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61, 54, 58, 35, 52, 50, 42,
                                      21, 44, 38, 32, 29, 23, 17, 11, 4,  62, 46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43,
                                      31, 22, 10, 45, 25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};

    return (uint32)index64[(uint32)(((value ^ (value - 1ULL)) * 0x03f79d71b4cb0a89ULL) >> 58)];
  }
#endif
}

static uint32 nexus_bits_uint64_popcount(uint64 value) /* NOLINT */ {
#if defined(__GNUC__) || defined(__clang__)
  return (uint32)__builtin_popcountll(value);
#else
  {
    uint32 count;

    count = 0;
    while (value != 0ULL) {
      value = nexus_bits_uint64_clear_lowest_set(value);
      count++;
    }
    return count;
  }
#endif
}

static uint64 nexus_bits_hash_mix_u64(uint64 hash, uint64 value) /* NOLINT */ {
  hash ^= value;
  hash *= 0x9E3779B97F4A7C15ULL;
  return hash;
}

/* ---------------------------------------------------------------------------- */
/* HASHING                                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusHash is reserved for future general-purpose hash containers.
*/
typedef struct NexusHash NexusHash;

/*
NexusHashEntropyFillCallback writes byte_count random bytes into out_bytes.

Callers supply any RNG backend (for example Blaze entropy) without Nexus taking a
dependency on that backend. The callback must write exactly byte_count bytes.
user_data is the opaque pointer passed to nexus_hash_zobrist_table_fill.
out_bytes must not be NULL when byte_count is greater than zero.
*/
typedef void NexusHashEntropyFillCallback(void *user_data, byte *out_bytes, uint64 byte_count);

/*
NexusHashZobristTable holds caller-owned Zobrist keys used for incremental XOR hashing.

keys must remain valid for the lifetime of the table. Nexus does not allocate or free keys.
See https://en.wikipedia.org/wiki/Zobrist_hashing
*/
typedef struct NexusHashZobristTable {
  uint64 *keys;
  uint64  key_count;
} NexusHashZobristTable;

/*
nexus_hash_zobrist_table_fill fills each key in table with an independent random bitstring.

Each uint64 key is assembled little-endian from eight bytes produced by fill.
table and fill must not be NULL. table->keys must not be NULL when table->key_count is greater
than zero. key_count may be zero.
*/
static void nexus_hash_zobrist_table_fill(NexusHashZobristTable *table, NexusHashEntropyFillCallback *fill, void *user_data) /* NOLINT */ {
  uint64 i;
  byte   key_bytes[8];

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(fill != NULL);
  NEXUS_ASSERT_DEBUG(table->key_count == 0 || table->keys != NULL);

  for (i = 0; i < table->key_count; i++) {
    fill(user_data, key_bytes, 8);
    table->keys[i] = nexus_bits_uint64_from_bytes_lsb(key_bytes);
  }
}

/*
nexus_hash_zobrist_key_get returns the key at index.

table must not be NULL. index must be less than table->key_count.
*/
static uint64 nexus_hash_zobrist_key_get(const NexusHashZobristTable *table, uint64 index) /* NOLINT */ {
  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->keys != NULL);
  NEXUS_ASSERT_MESSAGE_DEBUG(index < table->key_count, "Zobrist key index out of range.");

  return table->keys[index];
}

/*
nexus_hash_zobrist_hash_update XORs key into hash.

This is the incremental Zobrist step: adding and removing a feature use the same
operation because XOR is involutory. Prefer this over recomputing a full position hash
during search make/unmake.
*/
static uint64 nexus_hash_zobrist_hash_update(uint64 hash, uint64 key) /* NOLINT */ {
  return hash ^ key;
}

/*
nexus_hash_zobrist_hash_replace removes remove_key and adds add_key in one update
(hash ^ remove_key ^ add_key). Use when a feature changes identity (piece relocation).
*/
static uint64 nexus_hash_zobrist_hash_replace(uint64 hash, uint64 remove_key, uint64 add_key) /* NOLINT */ {
  return nexus_hash_zobrist_hash_update(nexus_hash_zobrist_hash_update(hash, remove_key), add_key);
}

/*
nexus_hash_zobrist_hash_xor is an alias of nexus_hash_zobrist_hash_update.
*/
static uint64 nexus_hash_zobrist_hash_xor(uint64 hash, uint64 key) /* NOLINT */ {
  return nexus_hash_zobrist_hash_update(hash, key);
}

/*
nexus_hash_zobrist_hash_from_keys XORs key_count keys starting at keys into an initial zero hash.

keys must not be NULL when key_count is greater than zero.
*/
static uint64 nexus_hash_zobrist_hash_from_keys(const uint64 *keys, uint64 key_count) /* NOLINT */ {
  uint64 hash;
  uint64 i;

  NEXUS_ASSERT_DEBUG(key_count == 0 || keys != NULL);

  hash = 0;
  for (i = 0; i < key_count; i++) {
    hash = nexus_hash_zobrist_hash_update(hash, keys[i]);
  }

  return hash;
}

/*
nexus_hash_fnv1a64_begin returns the FNV-1a 64-bit offset basis for a new hash stream.

See https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
*/
extern uint64 nexus_hash_fnv1a64_begin(void);

/*
nexus_hash_fnv1a64_byte folds one byte into hash using FNV-1a 64-bit.
*/
extern uint64 nexus_hash_fnv1a64_byte(uint64 hash, uint8 value);

/*
nexus_hash_fnv1a64_bytes folds byte_count bytes starting at data into hash using FNV-1a 64-bit.

data must not be NULL when byte_count is greater than zero. byte_count may be zero.
*/
extern uint64 nexus_hash_fnv1a64_bytes(uint64 hash, const void *data, uint64 byte_count);

/*
nexus_hash_fnv1a64 hashes byte_count bytes starting at data from the FNV-1a 64-bit offset basis.

Equivalent to nexus_hash_fnv1a64_bytes(nexus_hash_fnv1a64_begin(), data, byte_count).
data must not be NULL when byte_count is greater than zero. byte_count may be zero.
*/
extern uint64 nexus_hash_fnv1a64(const void *data, uint64 byte_count);

/* ---------------------------------------------------------------------------- */
/* IDs                                                                          */
/* ---------------------------------------------------------------------------- */

typedef struct NexusUUID NexusUUID;

/* ---------------------------------------------------------------------------- */
/* Hardware                                                                     */
/* ---------------------------------------------------------------------------- */

/*
nexus_hardware_cpu_clock_cycles_get retrieves the current CPU clock cycle count.

The returned value is architecture-specific: x86 uses RDTSC, AArch64 uses the
virtual counter (cntvct_el0), AArch32 uses the architected timer count, RISC-V
uses rdcycle, PowerPC uses the time base, MIPS uses the coprocessor cycle count,
and SPARC uses the tick register. Unsupported architectures return
NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE. User-mode access may still require platform
or kernel configuration on some targets.

On success it sets the `cycle_count` pointer to the current cycle count.
Returns NEXUS_ERROR_INVALID_ARGUMENT when cycle_count is NULL.
On failure it returns an NError.
*/
extern NError nexus_hardware_cpu_clock_cycles_get(uint64 *cycle_count);

/*
nexus_hardware_voluntary_context_switches_get retrieves the cumulative voluntary context switch
count for the current thread when supported, otherwise for the current process. On Windows this
returns the current thread's cumulative context switch count via NtQuerySystemInformation because
Windows does not expose the POSIX voluntary/involuntary split outside of ETW.

Returns NEXUS_ERROR_INVALID_ARGUMENT when count is NULL.
Returns NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE when the platform does not expose this counter.
Returns NEXUS_ERROR_IO when the platform query fails.
*/
extern NError nexus_hardware_voluntary_context_switches_get(uint64 *count);

/*
nexus_hardware_involuntary_context_switches_get retrieves the cumulative involuntary context
switch count for the current thread when supported, otherwise for the current process. This
counter is not available on Windows outside of ETW.

Returns NEXUS_ERROR_INVALID_ARGUMENT when count is NULL.
Returns NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE when the platform does not expose this counter.
Returns NEXUS_ERROR_IO when the platform query fails.
*/
extern NError nexus_hardware_involuntary_context_switches_get(uint64 *count);

/*
nexus_hardware_major_page_faults_get retrieves the cumulative major page fault count for the
current thread when supported, otherwise for the current process. On Windows this maps to the
current process hard fault count from NtQuerySystemInformation.

Returns NEXUS_ERROR_INVALID_ARGUMENT when count is NULL.
Returns NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE when the platform does not expose this counter.
Returns NEXUS_ERROR_IO when the platform query fails.
*/
extern NError nexus_hardware_major_page_faults_get(uint64 *count);

/*
nexus_hardware_minor_page_faults_get retrieves the cumulative minor page fault count for the
current thread when supported, otherwise for the current process. On Windows this maps to the
process-wide cumulative page fault count from GetProcessMemoryInfo.

Returns NEXUS_ERROR_INVALID_ARGUMENT when count is NULL.
Returns NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE when the platform does not expose this counter.
Returns NEXUS_ERROR_IO when the platform query fails.
*/
extern NError nexus_hardware_minor_page_faults_get(uint64 *count);

/*
nexus_hardware_cache_misses_get retrieves the cumulative hardware cache miss count for the
current thread when supported. On Linux this uses perf_event_open to program the CPU PMU.
User access may require permissive perf_event_paranoid sysctl settings.

Returns NEXUS_ERROR_INVALID_ARGUMENT when count is NULL.
Returns NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE when the platform does not expose this counter.
Returns NEXUS_ERROR_PERMISSION_DENIED when the kernel denies PMU access.
Returns NEXUS_ERROR_IO when the platform query fails.
*/
extern NError nexus_hardware_cache_misses_get(uint64 *count);

/*
nexus_hardware_floating_point_denormal_flush_push enables FTZ/DAZ on the calling
thread and returns the previous floating-point control word so
nexus_hardware_floating_point_denormal_flush_pop can restore it.

On x86/x86_64 this reads/writes SSE MXCSR (FTZ bit 15, DAZ bit 6). On other
architectures both push and pop are no-ops and push returns 0.

Use as a scoped pair around hot decay loops so the rest of the process keeps
IEEE subnormal behavior.
*/
extern uint32 nexus_hardware_floating_point_denormal_flush_push(void);

/*
nexus_hardware_floating_point_denormal_flush_pop restores the floating-point
control word previously returned by nexus_hardware_floating_point_denormal_flush_push.
*/
extern void nexus_hardware_floating_point_denormal_flush_pop(uint32 previous_control);

/* ---------------------------------------------------------------------------- */
/* ALGORITHMS & DATA STRUCTURES                                                 */
/* ---------------------------------------------------------------------------- */

#define NEXUS_CONCAT_IMPL(a, b) a##_##b
#define NEXUS_CONCAT(a, b)      NEXUS_CONCAT_IMPL(a, b)

#define NEXUS_KEY_VALUE_PAIR_DECLARE(key_type, val_type, suffix)                                                                                     \
  typedef struct NEXUS_CONCAT(NexusKeyValuePair, suffix) {                                                                                           \
    key_type key;                                                                                                                                    \
    val_type value;                                                                                                                                  \
  } NEXUS_CONCAT(NexusKeyValuePair, suffix);                                                                                                         \
                                                                                                                                                     \
  extern uint32 NEXUS_CONCAT(nexus_search_binary_kv, suffix)(const NEXUS_CONCAT(NexusKeyValuePair, suffix) * items, uint32 count, key_type target_key)

/* Source Definition Template */
#define NEXUS_KEY_VALUE_PAIR_DEFINE(key_type, val_type, suffix)                                                                                      \
  uint32 NEXUS_CONCAT(nexus_search_binary_kv, suffix)(const NEXUS_CONCAT(NexusKeyValuePair, suffix) * items, uint32 count, key_type target_key) {    \
    uint32 low;                                                                                                                                      \
    uint32 high;                                                                                                                                     \
    uint32 mid;                                                                                                                                      \
                                                                                                                                                     \
    if (count == 0) {                                                                                                                                \
      return 0;                                                                                                                                      \
    }                                                                                                                                                \
                                                                                                                                                     \
    low  = 0;                                                                                                                                        \
    high = count - 1;                                                                                                                                \
                                                                                                                                                     \
    while (low <= high) {                                                                                                                            \
      mid = low + ((high - low) / 2);                                                                                                                \
      if (items[mid].key == target_key) {                                                                                                            \
        return items[mid].value;                                                                                                                     \
      }                                                                                                                                              \
      if (items[mid].key < target_key) {                                                                                                             \
        low = mid + 1;                                                                                                                               \
      } else {                                                                                                                                       \
        if (mid == 0)                                                                                                                                \
          break;                                                                                                                                     \
        high = mid - 1;                                                                                                                              \
      }                                                                                                                                              \
    }                                                                                                                                                \
    return count;                                                                                                                                    \
  }

#define NEXUS_DATA_HEAP_MIN_INDEX_DECLARE(key_type, suffix)                                                                                          \
  extern void   NEXUS_CONCAT(nexus_data_heap_min_index_push, suffix)(uint32 * priority_queue, uint32 * priority_queue_count, uint32 index,           \
                                                                   const key_type *keys);                                                          \
  extern uint32 NEXUS_CONCAT(nexus_data_heap_min_index_pop, suffix)(uint32 * priority_queue, uint32 * priority_queue_count, const key_type *keys)

#define NEXUS_DATA_HEAP_MIN_INDEX_DEFINE(key_type, suffix)                                                                                           \
  void NEXUS_CONCAT(nexus_data_heap_min_index_push, suffix)(uint32 * priority_queue, uint32 * priority_queue_count, uint32 index,                    \
                                                            const key_type *keys) {                                                                  \
    uint32 current;                                                                                                                                  \
    uint32 parent;                                                                                                                                   \
    uint32 swap_tmp;                                                                                                                                 \
                                                                                                                                                     \
    current                 = *priority_queue_count;                                                                                                 \
    priority_queue[current] = index;                                                                                                                 \
    (*priority_queue_count)++;                                                                                                                       \
                                                                                                                                                     \
    while (current > 0) {                                                                                                                            \
      parent = (current - 1) / 2;                                                                                                                    \
      if (keys[priority_queue[current]] < keys[priority_queue[parent]]) {                                                                            \
        swap_tmp                = priority_queue[current];                                                                                           \
        priority_queue[current] = priority_queue[parent];                                                                                            \
        priority_queue[parent]  = swap_tmp;                                                                                                          \
        current                 = parent;                                                                                                            \
      } else {                                                                                                                                       \
        break;                                                                                                                                       \
      }                                                                                                                                              \
    }                                                                                                                                                \
  }                                                                                                                                                  \
                                                                                                                                                     \
  uint32 NEXUS_CONCAT(nexus_data_heap_min_index_pop, suffix)(uint32 * priority_queue, uint32 * priority_queue_count, const key_type *keys) {         \
    uint32 root;                                                                                                                                     \
    uint32 current;                                                                                                                                  \
    uint32 left;                                                                                                                                     \
    uint32 right;                                                                                                                                    \
    uint32 smallest;                                                                                                                                 \
    uint32 swap_tmp;                                                                                                                                 \
                                                                                                                                                     \
    root    = priority_queue[0];                                                                                                                     \
    current = 0;                                                                                                                                     \
    (*priority_queue_count)--;                                                                                                                       \
    priority_queue[0] = priority_queue[*priority_queue_count];                                                                                       \
                                                                                                                                                     \
    while (1) {                                                                                                                                      \
      left     = (2 * current) + 1;                                                                                                                  \
      right    = (2 * current) + 2;                                                                                                                  \
      smallest = current;                                                                                                                            \
                                                                                                                                                     \
      if (left < *priority_queue_count && keys[priority_queue[left]] < keys[priority_queue[smallest]]) {                                             \
        smallest = left;                                                                                                                             \
      }                                                                                                                                              \
      if (right < *priority_queue_count && keys[priority_queue[right]] < keys[priority_queue[smallest]]) {                                           \
        smallest = right;                                                                                                                            \
      }                                                                                                                                              \
                                                                                                                                                     \
      if (smallest != current) {                                                                                                                     \
        swap_tmp                 = priority_queue[current];                                                                                          \
        priority_queue[current]  = priority_queue[smallest];                                                                                         \
        priority_queue[smallest] = swap_tmp;                                                                                                         \
        current                  = smallest;                                                                                                         \
      } else {                                                                                                                                       \
        break;                                                                                                                                       \
      }                                                                                                                                              \
    }                                                                                                                                                \
    return root;                                                                                                                                     \
  }

/* Table for Key-Value Pairs: ENTRY(key_type, value_type, suffix_name) */
#define NEXUS_KV_TYPE_TABLE(ENTRY)                                                                                                                   \
  ENTRY(uint32, uint32, uint32)                                                                                                                      \
  ENTRY(uint64, uint32, uint64_uint32)                                                                                                               \
  ENTRY(uint64, uint64, uint64)                                                                                                                      \
  ENTRY(int32, uint32, int32_uint32)                                                                                                                 \
  ENTRY(int64, uint32, int64_uint32)                                                                                                                 \
  ENTRY(uint_large, uint32, uint_large)                                                                                                              \
  ENTRY(f_real, uint32, f_real_uint32)

/* Table for Key-Only Heaps: ENTRY(key_type, suffix_name) */
#define NEXUS_HEAP_TYPE_TABLE(ENTRY)                                                                                                                 \
  ENTRY(uint32, uint32)                                                                                                                              \
  ENTRY(uint64, uint64)                                                                                                                              \
  ENTRY(int32, int32)                                                                                                                                \
  ENTRY(int64, int64)                                                                                                                                \
  ENTRY(uint_large, uint_large)                                                                                                                      \
  ENTRY(f_real, f_real)

/* Instantiate all Key-Value struct definitions and function declarations */
#define X_KV_DECL(key_type, val_type, suffix) NEXUS_KEY_VALUE_PAIR_DECLARE(key_type, val_type, suffix);
NEXUS_KV_TYPE_TABLE(X_KV_DECL)
#undef X_KV_DECL

/* Instantiate all Heap function declarations */
#define X_HEAP_DECL(key_type, suffix) NEXUS_DATA_HEAP_MIN_INDEX_DECLARE(key_type, suffix);
NEXUS_HEAP_TYPE_TABLE(X_HEAP_DECL)
#undef X_HEAP_DECL
