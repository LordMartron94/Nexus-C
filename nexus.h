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

static boolean nexus_uint128_compare(uint128 value_a, uint128 value_b) { /* NOLINT */
  return value_a.hi == value_b.hi && value_a.lo == value_b.lo;
}

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
#define NEXUS_ERROR_INTERRUPTED              NEXUS_ERROR_MAKE('N', 'X', 10)

/*
NexusErrorMessageFormatter returns a human-readable description for a facility-specific
error code. Return a stable string (typically a string literal). Return NULL when the
code is unknown so nexus_errors_message_write can fall back to the generic
"Error XX-<decimal> (0x<hex>)" form.
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
nexus_errors_message_formatter_register) produce descriptive messages. Every
non-success message includes its facility-local code in decimal and hexadecimal
form. Unknown facilities or unknown codes within a registered facility fall
back to "Error XX-<decimal> (0x<hex>)".
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
Nexus memory debugging is based on the MergeSource / Forge memory debugger
(f_mem_debug.c), adapted for Nexus.

When NEXUS_MEMORY_DEBUG_ENABLED is non-zero, malloc, calloc, realloc, and free
are redirected through Nexus debug-memory functions. The debugger tracks live
allocations, allocation sites, guard regions, aggregate statistics, optional
freed-memory history, and optional allocation logging.

The public allocation interface remains the standard C allocation interface.
Code using malloc/calloc/realloc/free therefore does not need to know whether
memory debugging is enabled.

NEXUS_MEMORY_DEBUG_IMPLEMENTATION must only be defined by the translation unit
implementing the memory debugger. It prevents that translation unit's own libc
allocation calls from being redirected back into the debugger.

The memory debugger begins in the active state. It can temporarily be suspended
at runtime through nexus_debug_mem_active and related functions without
recompiling Nexus.

The debugger supports concurrent allocations when synchronization has been
configured through nexus_debug_mem_thread_safe_init. nexus_threads_create
automatically performs this configuration before creating the first Nexus
secondary thread.

Memory-log callbacks may execute concurrently on different threads. Recursive
logging caused by allocations made by the callback itself is suppressed on a
per-thread basis.
*/

#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------------------- */
/* THREAD-LOCAL STORAGE                                                         */
/* ---------------------------------------------------------------------------- */

/*
NEXUS_THREAD_LOCAL declares an object with one independent instance per thread.

Nexus uses compiler-supported TLS extensions when compiling in C89 mode.
The abstraction is intentionally allocation-free and may therefore be used by
low-level facilities such as the memory debugger where heap-backed TLS would
create an allocator dependency cycle.

Static initialization must be sufficient for objects declared through this
macro. Do not use it for dynamically constructed per-thread resources that
require explicit destruction.
*/
#ifndef NEXUS_THREAD_LOCAL
#  if defined(_MSC_VER)
#    define NEXUS_THREAD_LOCAL __declspec(thread)
#  elif defined(__GNUC__) || defined(__clang__)
#    define NEXUS_THREAD_LOCAL __thread
#  elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#    define NEXUS_THREAD_LOCAL _Thread_local
#  else
#    error "Nexus requires compiler thread-local storage support"
#  endif
#endif

/* ---------------------------------------------------------------------------- */
/* MEMORY DEBUGGER CONFIGURATION                                                */
/* ---------------------------------------------------------------------------- */

/*
NEXUS_MEMORY_DEBUG_ENABLED controls whether C heap allocation calls are routed
through the Nexus memory debugger.

When enabled:
- malloc, calloc, realloc, and free are redirected to Nexus wrappers;
- live allocations are tracked by source location;
- guard regions are installed around tracked allocations;
- memory utility functions can validate tracked ranges;
- allocation statistics and reports are available.

Default: enabled.
*/
#ifndef NEXUS_MEMORY_DEBUG_ENABLED
#  define NEXUS_MEMORY_DEBUG_ENABLED 1
#endif

/*
NEXUS_MEMORY_OVER_ALLOC specifies the total amount of guard storage reserved
around each tracked allocation.

NEXUS_MEMORY_PRE_PADDING bytes are placed before the user allocation. The
remaining bytes are placed after it.

NEXUS_MEMORY_OVER_ALLOC must be greater than or equal to
NEXUS_MEMORY_PRE_PADDING.
*/
#ifndef NEXUS_MEMORY_OVER_ALLOC
#  define NEXUS_MEMORY_OVER_ALLOC 64
#endif

/*
NEXUS_MEMORY_PRE_PADDING specifies how many bytes of the debug allocation guard
region precede the user-visible allocation.
*/
#ifndef NEXUS_MEMORY_PRE_PADDING
#  define NEXUS_MEMORY_PRE_PADDING 16
#endif

/*
NEXUS_MEMORY_NULL_ALLOCATION_ERROR causes failed tracked allocations to invoke
NEXUS_MEMORY_CALL_ON_ERROR after diagnostic information has been emitted.

Define this macro to enable that behavior.
*/
#ifndef NEXUS_MEMORY_NULL_ALLOCATION_ERROR
#  define NEXUS_MEMORY_NULL_ALLOCATION_ERROR
#endif

/*
NEXUS_MEMORY_DOUBLE_FREE_CHECK enables retention of recently freed allocation
metadata so attempts to free the same pointer twice can be diagnosed.

This increases debugger memory usage.
*/
#ifndef NEXUS_MEMORY_DOUBLE_FREE_CHECK
#  define NEXUS_MEMORY_DOUBLE_FREE_CHECK
#endif

/*
NEXUS_MEMORY_USE_AFTER_FREE_CHECK retains recently freed allocation storage and
fills it with a known byte pattern.

Later integrity scans can detect writes made after the allocation was freed.

This substantially increases debugger memory usage because recently freed
storage must remain resident until displaced from the freed-allocation history.
*/
#ifndef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
#  define NEXUS_MEMORY_USE_AFTER_FREE_CHECK
#endif

/*
NEXUS_MEMORY_WARN_ON_REALLOC_NULL enables diagnostics when realloc is called
with a NULL pointer.
*/
#ifndef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
#  define NEXUS_MEMORY_WARN_ON_REALLOC_NULL
#endif

/*
NEXUS_MEMORY_CALL_ON_ERROR specifies the action performed after the debugger
detects a fatal memory error.

The default terminates through abort(). Applications may override this macro
before including Nexus when another debugger-break or failure mechanism is
required.
*/
#ifndef NEXUS_MEMORY_CALL_ON_ERROR
#  define NEXUS_MEMORY_CALL_ON_ERROR abort();
#endif

/*
NEXUS_MEMORY_STACK_GUESS_SIZE specifies the approximate stack range used by
stack-reference diagnostics when the main thread stack has not explicitly been
registered through nexus_debug_mem_stack_pointer_set.
*/
#ifndef NEXUS_MEMORY_STACK_GUESS_SIZE
#  define NEXUS_MEMORY_STACK_GUESS_SIZE ((size_t)1024 * (size_t)1024)
#endif

/* ---------------------------------------------------------------------------- */
/* MEMORY DEBUGGER SYNCHRONIZATION                                              */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_thread_safe_init configures synchronization for the process-wide
memory debugger.

lock and unlock receive mutex unchanged and must return zero on success.

The supplied synchronization primitive must:
- require no tracked heap allocation to operate;
- remain valid for the lifetime of the process;
- be initialized before more than one thread can enter the memory debugger.

The allocator must not depend on a normal heap-backed NexusMutex for its own
internal synchronization, because constructing or destroying such a mutex would
itself require the allocator.

nexus_threads_create automatically installs Nexus' allocation-free internal
memory-debugger lock before creating the first secondary Nexus thread.

The first valid synchronization configuration is intended to remain installed
for the process lifetime. Replacing allocator synchronization while threads are
active is unsupported.

Single-threaded applications may omit explicit configuration.
*/
extern void nexus_debug_mem_thread_safe_init(int (*lock)(void *mutex), int (*unlock)(void *mutex), void *mutex);

/* ---------------------------------------------------------------------------- */
/* MEMORY DEBUGGER RUNTIME STATE                                                */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_active enables or suspends full memory debugging.

When active is TRUE:
- new allocations receive guard regions;
- new allocations are entered into the tracking table;
- allocation statistics are updated;
- memory range validation is performed;
- memory-debug logging may be emitted.

When active is FALSE:
- new allocations use the underlying libc allocator directly;
- memory-debug range checks are bypassed;
- allocation-site tracking for new libc allocations is suppressed;
- allocation-event statistics and active measurement intervals still record
  successful malloc, calloc, and realloc operations.

Allocations created while the debugger was active remain recognizable after the
debugger is suspended. They can therefore still be safely freed or reallocated.

This is intended for temporarily removing debugger overhead from hot paths
without losing the ability to resume debugging later.
*/
extern void nexus_debug_mem_active(boolean active);

/*
nexus_debug_mem_active_get returns TRUE when full memory debugging is currently
active and FALSE when it is suspended.

The returned state is synchronized when thread-safe memory debugging has been
configured.
*/
extern boolean nexus_debug_mem_active_get(void);

/*
nexus_debug_mem_active_exchange changes the active state and returns the
previous state.

This is the preferred operation for temporary suspension because it allows the
caller to restore the exact state that was present before entering a scope.

Example:

  previous = nexus_debug_mem_active_exchange(FALSE);

  ... performance-sensitive work ...

  (void)nexus_debug_mem_active_exchange(previous);
*/
extern boolean nexus_debug_mem_active_exchange(boolean active);

/* ---------------------------------------------------------------------------- */
/* MEMORY DEBUGGER STACK INFORMATION                                            */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_stack_pointer_set registers the known main-thread stack range.

lowest_stack_pointer identifies the lowest address belonging to the stack and
stack_size_in_bytes specifies the complete registered range.

The information is used by:
- nexus_debug_mem_query_is_allocated;
- nexus_debug_mem_check_stack_reference;
- heap/reference diagnostics which distinguish stack addresses from heap
  addresses.

If no stack range is registered, stack-reference diagnostics fall back to
NEXUS_MEMORY_STACK_GUESS_SIZE and heuristic distance checks.

This API currently describes the main thread stack rather than maintaining a
registry of every thread stack.
*/
extern void nexus_debug_mem_stack_pointer_set(void *lowest_stack_pointer, size_t stack_size_in_bytes);

/* ---------------------------------------------------------------------------- */
/* MEMORY DEBUGGER LOGGING                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusDebugMemLogCallback receives memory-debug trace messages.

user_data is the opaque value supplied through
nexus_debug_mem_log_callback_set.

message contains the fully formatted memory operation description.

file and line identify the source location responsible for the operation.

Callbacks can be generated by:
- tracked malloc;
- tracked calloc;
- tracked realloc;
- tracked free;
- nexus_memory_bytes_copy;
- nexus_memory_bytes_set;
- nexus_memory_bytes_clear.

Callbacks are never invoked while the allocator metadata lock is held.

Consequently:
- callbacks may themselves perform memory allocations;
- allocator-to-logger lock-order deadlocks are avoided;
- callbacks from separate threads may execute concurrently.

Allocations performed recursively by the callback on the same thread do not
generate further memory-log callbacks. Recursive suppression is thread-local,
so one thread emitting a callback does not suppress logging on other threads.

Implementations of this callback must therefore be thread-safe if allocation
can occur from multiple threads.
*/
typedef void NexusDebugMemLogCallback(void *user_data, const char *message, const char *file, uint32 line);

/*
nexus_debug_mem_log_callback_set installs or removes the memory-debug logging
callback.

Pass a non-NULL callback to enable logging.

Pass NULL as callback to disable logging. user_data is ignored when callback is
NULL.

Callback registration state is synchronized with the memory debugger.

A callback invocation that already obtained the previous callback and user_data
may finish after another thread replaces or disables the callback. Therefore,
the lifetime of callback user_data must extend until all threads capable of
performing memory-debug operations have been quiesced or joined.

The callback is invoked outside the memory-debugger metadata lock.
*/
extern void nexus_debug_mem_log_callback_set(NexusDebugMemLogCallback *callback, void *user_data);

/*
nexus_debug_mem_log_callback_installed_get returns TRUE when a memory-log
callback is currently registered.

The returned registration state is synchronized when thread-safe memory
debugging has been configured.
*/
extern boolean nexus_debug_mem_log_callback_installed_get(void);

/* ---------------------------------------------------------------------------- */
/* TRACKED ALLOCATION OPERATIONS                                                */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_malloc implements the debug-memory path for malloc.

size specifies the number of user-visible bytes requested.

file and line identify the source allocation site.

When full debugging is active, the function:
- validates the requested size;
- allocates user storage plus debug guard regions;
- initializes guard regions;
- initializes user storage with the debugger initialization pattern;
- records the allocation and source site;
- updates allocation statistics;
- optionally emits a memory-log callback.

Integer overflow while calculating the backing allocation size is treated as
allocation failure.

Returns the user-visible pointer on success or NULL on failure.

When NEXUS_MEMORY_NULL_ALLOCATION_ERROR is enabled, allocation failure also
invokes NEXUS_MEMORY_CALL_ON_ERROR.
*/
extern void *nexus_debug_mem_malloc(size_t size, char *file, uint32 line);

/*
nexus_debug_mem_calloc implements the debug-memory path for calloc.

num specifies the number of elements and size specifies the size of each
element.

The multiplication num * size is checked for size_t overflow before allocating.

When full debugging is active, the resulting user allocation is zero-filled,
guarded, tracked, and included in debugger statistics.

Returns the user-visible pointer on success or NULL for a zero-sized request or
allocation failure.
*/
extern void *nexus_debug_mem_calloc(size_t num, size_t size, char *file, uint32 line);

/*
nexus_debug_mem_realloc implements the debug-memory path for realloc.

pointer may refer to:
- a currently tracked allocation;
- NULL;
- an allocation created while full memory debugging was suspended.

When pointer is NULL, the operation follows the debugger's malloc path.

For a tracked allocation, the function preserves up to min(old_size, new_size)
bytes, rebuilds guard regions, updates tracking metadata, and preserves debugger
consistency as one synchronized operation.

When the debugger is suspended, previously tracked allocations remain
recognizable and can be transitioned safely to ordinary libc-backed storage.

Allocation-size arithmetic is checked for overflow.

Returns the replacement pointer on success or NULL on failure.
*/
extern void *nexus_debug_mem_realloc(void *pointer, size_t size, char *file, uint32 line);

/*
nexus_debug_mem_free implements the debug-memory path for free.

For tracked allocations it:
- validates pre-allocation and post-allocation guard regions;
- removes the allocation from live tracking;
- updates debugger statistics;
- records freed-allocation metadata when enabled;
- fills retained freed storage with the freed-memory pattern when
  NEXUS_MEMORY_USE_AFTER_FREE_CHECK is enabled;
- releases backing storage when retention is not required;
- optionally emits a memory-log callback.

Depending on enabled debugger features, the operation can diagnose:
- buffer underruns;
- buffer overruns;
- double frees;
- frees of pointers into the interior of an allocation.

Allocations originating from a debugger-suspended period are released through
the underlying libc allocator.

Passing NULL follows the debugger implementation's normal free semantics.
*/
extern void nexus_debug_mem_free(void *buf, char *file, uint32 line);

/* ---------------------------------------------------------------------------- */
/* ALLOCATION METADATA                                                          */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_comment associates a descriptive label with a tracked live
allocation.

buf must identify the start of a currently tracked allocation.

comment is copied into debugger-owned storage. The caller therefore does not
need to preserve the source string after this function returns.

Replacing an existing comment releases the previous debugger-owned copy.

Comments are displayed by nexus_debug_mem_print and can be used to distinguish
allocations originating from the same source location.

Returns TRUE when buf identifies a tracked allocation and the comment operation
succeeds. Returns FALSE when no matching allocation exists or the comment
cannot be stored.
*/
extern boolean nexus_debug_mem_comment(void *buf, char *comment);

/* ---------------------------------------------------------------------------- */
/* MEMORY STATISTICS                                                            */
/* ---------------------------------------------------------------------------- */

/*
NexusDebugMemSummary contains process-wide memory-debug statistics.

live_bytes:
  Number of user-visible bytes currently represented by tracked live
  allocations.

peak_live_bytes:
  Highest live_bytes value observed since debugger initialization or the last
  statistics reset.

live_block_count:
  Number of currently tracked live allocations.

peak_live_block_count:
  Highest live_block_count observed since debugger initialization or the last
  statistics reset.

total_bytes_allocated:
  Cumulative user-visible bytes allocated through the Nexus allocation
  wrappers, including allocations made while full debugging was suspended.

total_bytes_freed:
  Cumulative user-visible bytes freed from tracked allocations.

allocation_count:
  Number of allocation events made through the Nexus allocation wrappers,
  including events observed while full debugging was suspended.

free_count:
  Number of tracked free events.

call_site_count:
  Number of distinct source allocation sites known to the debugger.

largest_allocation_bytes:
  Largest individual allocation observed through the Nexus allocation wrappers
  during the current statistics interval.
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
nexus_debug_mem_summary_get copies a consistent snapshot of process-wide
memory-debug statistics into summary.

summary must not be NULL.

The snapshot is taken while debugger metadata is synchronized, but naturally
represents only the instant at which it was captured; other threads may allocate
or free memory immediately afterward.
*/
extern void nexus_debug_mem_summary_get(NexusDebugMemSummary *summary);

/*
nexus_debug_mem_summary_print writes a compact human-readable summary of current
memory-debug statistics to standard output.

The function obtains a consistent statistics snapshot before formatting it.
*/
extern void nexus_debug_mem_summary_print(void);

/*
nexus_debug_mem_print writes the current tracked-allocation report to standard
output.

For each qualifying allocation site the report includes:
- source file and line;
- currently live bytes;
- currently live allocation count;
- allocation/free statistics;
- individual live pointers and sizes;
- optional allocation comments.

min_allocs filters allocation sites according to their live allocation count.

Debugger metadata remains protected while the report traverses the tracking
table so concurrent allocation cannot invalidate report data.
*/
extern void nexus_debug_mem_print(uint32 min_allocs);

/*
nexus_debug_mem_reset resets interval-style statistics without freeing or
forgetting currently live allocations.

The operation resets:
- cumulative allocation-event counters;
- cumulative free-event counters;
- interval allocation byte totals;
- peak statistics;
- largest-allocation statistics;
- corresponding per-site historical counters.

Current live allocations remain tracked.

In particular, current live_bytes and live_block_count remain valid so that
later frees cannot underflow or corrupt live-allocation accounting.

After reset, current live usage becomes the new baseline from which subsequent
peak values grow.
*/
extern void nexus_debug_mem_reset(void);

/* ---------------------------------------------------------------------------- */
/* MEMORY MEASUREMENTS                                                          */
/* ---------------------------------------------------------------------------- */

/*
NexusDebugMemMeasurement contains allocation activity observed during a
measurement interval.

allocation_count is the number of allocation events during the interval.

total_bytes_allocated is the cumulative number of user-visible bytes allocated
during the interval.

largest_allocation_bytes is the size of the largest individual allocation
observed during the interval.
*/
typedef struct NexusDebugMemMeasurement {
  uint_large allocation_count;
  uint_large total_bytes_allocated;
  size_t     largest_allocation_bytes;
} NexusDebugMemMeasurement;

/*
NexusDebugMemMeasurementContext stores the baseline needed to calculate one
allocation measurement interval.

Applications should treat its fields as implementation-managed state after
passing the context to nexus_debug_mem_measurement_begin.

The structure remains public for ABI compatibility and stack allocation.
*/
typedef struct NexusDebugMemMeasurementContext {
  uint_large baseline_allocation_count;
  uint_large baseline_total_bytes_allocated;
  size_t     interval_largest_allocation_bytes;
} NexusDebugMemMeasurementContext;

/*
nexus_debug_mem_measurement_begin starts an allocation measurement using
context.

The function snapshots the current allocation counters and registers context as
an active measurement interval.

It does not reset global allocation tracking or global statistics.

Multiple measurement contexts may be active concurrently. Allocations occurring
while several measurements are active contribute to each applicable interval.

context must remain valid until the corresponding
nexus_debug_mem_measurement_end call.
*/
extern void nexus_debug_mem_measurement_begin(NexusDebugMemMeasurementContext *context);

/*
nexus_debug_mem_measurement_end finishes the interval represented by context and
writes the resulting statistics into measurement.

The function calculates differences from the baseline captured by
nexus_debug_mem_measurement_begin and unregisters the context from active
measurement tracking.

context and measurement must not be NULL.

The same context should not be ended more than once without first beginning a
new interval.
*/
extern void nexus_debug_mem_measurement_end(const NexusDebugMemMeasurementContext *context, NexusDebugMemMeasurement *measurement);

/* ---------------------------------------------------------------------------- */
/* MEMORY USAGE QUERIES                                                         */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_consumption returns the total number of user-visible bytes
belonging to currently tracked live allocations.

The result does not include:
- debugger guard bytes;
- debugger metadata;
- allocations made while full debugging was suspended.

The value is obtained from synchronized debugger state.
*/
extern size_t nexus_debug_mem_consumption(void);

/*
nexus_debug_mem_footprint returns the sum of user-visible sizes represented by
the live allocation table.

min_allocs is retained for API compatibility and possible future filtering.

The function currently reports total tracked live allocation size.
*/
extern size_t nexus_debug_mem_footprint(uint32 min_allocs);

/* ---------------------------------------------------------------------------- */
/* ALLOCATION QUERIES                                                           */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_query_allocation searches for the tracked live allocation
containing pointer.

pointer may identify either the beginning or an interior byte of an allocation.

When a matching allocation is found:
- line receives its source line when non-NULL;
- file receives its source file string when non-NULL;
- size receives its user-visible allocation size when non-NULL.

The returned pointer is the beginning of the containing allocation.

The debugger owns the returned file string. Its storage is stable for the
lifetime of the corresponding allocation-site metadata and must not be modified
or freed by the caller.

Returns NULL when pointer does not belong to a tracked live allocation.
*/
extern void *nexus_debug_mem_query_allocation(void *pointer, uint32 *line, char **file, size_t *size);

/*
nexus_debug_mem_query_is_allocated validates whether the byte range

  [pointer, pointer + size)

is contained within a tracked live allocation.

Returns TRUE when the complete range is valid.

Returns FALSE when:
- pointer is not within a tracked live allocation;
- the requested range extends beyond the allocation;
- the range refers to known freed storage;
- the range is identified as stack storage.

When ignore_not_found is FALSE, an unrecognized pointer produces a diagnostic.
When TRUE, absence from the tracking table is silent.

When full memory debugging is suspended, the function returns TRUE without
performing tracked-allocation validation.
*/
extern boolean nexus_debug_mem_query_is_allocated(const void *pointer, size_t size, boolean ignore_not_found);

/* ---------------------------------------------------------------------------- */
/* DEBUGGED MEMORY OPERATIONS                                                   */
/* ---------------------------------------------------------------------------- */

#if NEXUS_MEMORY_DEBUG_ENABLED

/*
nexus_debug_mem_bytes_copy is the debug-memory implementation path for
nexus_memory_bytes_copy.

When full debugging is active, dest and src are checked against tracked
allocation boundaries before the copy.

A memory-log callback is emitted when logging is installed.

The actual operation has memcpy semantics: source and destination regions must
not overlap.

file and line identify the source operation site.
*/
extern void nexus_debug_mem_bytes_copy(void *dest, const void *src, uint_large byte_count, char *file, uint32 line);

/*
nexus_debug_mem_bytes_set is the debug-memory implementation path for
nexus_memory_bytes_set.

When full debugging is active, the destination range is validated against
tracked allocation boundaries before writing.

A memory-log callback is emitted when logging is installed.

file and line identify the source operation site.
*/
extern void nexus_debug_mem_bytes_set(void *dest, uint8 byte, uint_large byte_count, char *file, uint32 line);

/*
nexus_debug_mem_bytes_clear is the debug-memory implementation path for
nexus_memory_bytes_clear.

When full debugging is active, the destination range is validated against
tracked allocation boundaries before clearing.

A memory-log callback is emitted when logging is installed.

file and line identify the source operation site.
*/
extern void nexus_debug_mem_bytes_clear(void *dest, uint_large byte_count, char *file, uint32 line);

#endif /* NEXUS_MEMORY_DEBUG_ENABLED */

/* ---------------------------------------------------------------------------- */
/* MEMORY INTEGRITY CHECKS                                                      */
/* ---------------------------------------------------------------------------- */

/*
nexus_debug_mem_check_bounds scans tracked allocations for memory corruption.

The function checks:
- pre-allocation guard regions for buffer underruns;
- post-allocation guard regions for buffer overruns;
- retained freed blocks for writes after free when
  NEXUS_MEMORY_USE_AFTER_FREE_CHECK is enabled.

Detected corruption emits diagnostic information and invokes
NEXUS_MEMORY_CALL_ON_ERROR according to debugger configuration.

Returns TRUE when corruption is detected and FALSE otherwise.

The tracking state is synchronized for the duration of the scan.
*/
extern boolean nexus_debug_mem_check_bounds(void);

/*
nexus_debug_mem_check_stack_reference scans tracked heap allocations for pointer
values that appear to reference stack memory.

When nexus_debug_mem_stack_pointer_set has registered a known stack range, that
range is used directly.

Otherwise the function uses NEXUS_MEMORY_STACK_GUESS_SIZE and heuristic stack
distance detection.

The check is diagnostic and may produce false positives because arbitrary
allocation contents can resemble pointer values.

Returns TRUE when at least one suspicious stack reference is found.
*/
extern boolean nexus_debug_mem_check_stack_reference(void);

/*
nexus_debug_mem_check_heap_reference searches for tracked heap allocations which
appear to be unreachable from other tracked heap allocations or the configured
stack-reference search.

minimum_allocations suppresses reporting for allocation sites below the
specified allocation threshold and can be used to reduce noise from transient
helper allocations.

This is a heuristic debugging facility rather than a tracing garbage collector;
absence of a discovered reference does not prove that an allocation is leaked.
*/
extern void nexus_debug_mem_check_heap_reference(uint32 minimum_allocations);

/* ---------------------------------------------------------------------------- */
/* CORE MEMORY/TYPE HELPERS                                                     */
/* ---------------------------------------------------------------------------- */

#define NEXUS_SIZEOF(type)                             ((size_t)sizeof(type))
#define NEXUS_OFFSETOF(type, field)                    ((size_t)offsetof(type, field))
#define NEXUS_ARRAY_SIZE_BYTES(array)                  ((size_t)sizeof(array))
#define NEXUS_ARRAY_SIZE_ELEMENTS(array)               ((uint64)sizeof(array) / (uint64)sizeof((array)[0]))
#define NEXUS_MEMORY_OFFSET(base_pointer, byte_offset) ((void *)((unsigned char *)(base_pointer) + (size_t)(byte_offset)))

#define NEXUS_ALIGNOF(type)                                                                                                                          \
  ((size_t)&(((struct {                                                                                                                              \
               char c;                                                                                                                               \
               type t;                                                                                                                               \
             } *)0)                                                                                                                                  \
                 ->t))

/* ---------------------------------------------------------------------------- */
/* STANDARD ALLOCATOR REDIRECTION                                               */
/* ---------------------------------------------------------------------------- */

#if NEXUS_MEMORY_DEBUG_ENABLED

#  include <stdlib.h>

/*
Do not redirect libc allocation calls inside the memory debugger implementation
itself. That translation unit defines NEXUS_MEMORY_DEBUG_IMPLEMENTATION before
including Nexus and therefore retains direct access to libc allocation.
*/
#  if !defined(NEXUS_MEMORY_DEBUG_IMPLEMENTATION)
#    define malloc(n)     nexus_debug_mem_malloc((n), __FILE__, __LINE__)
#    define calloc(n, m)  nexus_debug_mem_calloc((n), (m), __FILE__, __LINE__)
#    define realloc(n, m) nexus_debug_mem_realloc((n), (m), __FILE__, __LINE__)
#    define free(n)       nexus_debug_mem_free((n), __FILE__, __LINE__)
#  endif

#else

/*
When memory debugging is compiled out, debugger-only operations collapse to
zero-cost or constant-result forms.

NEXUS_MEMORY_DEBUG_INTERNAL prevents these substitutions inside debugger-related
implementation units which still need declarations or implementation details.
*/
#  ifndef NEXUS_MEMORY_DEBUG_INTERNAL
#    define nexus_debug_mem_thread_safe_init(n, m, k)
#    define nexus_debug_mem_stack_pointer_set(n, m)
#    define nexus_debug_mem_active(n)
#    define nexus_debug_mem_active_get()            TRUE
#    define nexus_debug_mem_active_exchange(active) TRUE
#    define nexus_debug_mem_log_callback_set(n, m)
#    define nexus_debug_mem_log_callback_installed_get() FALSE
#    define nexus_debug_mem_comment(n, m)                FALSE
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

/* ---------------------------------------------------------------------------- */
/* GENERAL MEMORY OPERATIONS                                                    */
/* ---------------------------------------------------------------------------- */

/*
NEXUS_MEMORY_PREFETCH_LOCALITY_* specifies the expected temporal locality of
memory passed to NEXUS_MEMORY_PREFETCH.

Higher locality indicates that the prefetched cache line is expected to be used
again soon and should therefore be retained closer to the processor.

NEXUS_MEMORY_PREFETCH_LOCALITY_NONE:
  The data is not expected to be reused after the imminent access. Prefer
  minimal cache pollution.

NEXUS_MEMORY_PREFETCH_LOCALITY_LOW:
  The data may be reused, but only with relatively low temporal locality.

NEXUS_MEMORY_PREFETCH_LOCALITY_MEDIUM:
  The data is expected to be reused with moderate temporal locality.

NEXUS_MEMORY_PREFETCH_LOCALITY_HIGH:
  The data is expected to be reused soon and should preferably remain in the
  nearest practical cache level.

These values correspond directly to the locality range expected by GCC and
Clang's __builtin_prefetch.
*/
#define NEXUS_MEMORY_PREFETCH_LOCALITY_NONE   0
#define NEXUS_MEMORY_PREFETCH_LOCALITY_LOW    1
#define NEXUS_MEMORY_PREFETCH_LOCALITY_MEDIUM 2
#define NEXUS_MEMORY_PREFETCH_LOCALITY_HIGH   3

/*
NEXUS_MEMORY_PREFETCH provides a non-binding hint that the cache line containing
address is likely to be accessed soon.

address identifies the memory location to prefetch.

read_write specifies the expected access:
- FALSE indicates a read-oriented access;
- TRUE indicates an access which may modify the memory.

locality must be one of NEXUS_MEMORY_PREFETCH_LOCALITY_*.

Prefetching does not alter program semantics. Implementations may ignore the
request entirely.

On GCC and Clang this maps to __builtin_prefetch.

On MSVC, appropriate x86/x64 prefetch intrinsics are used.

On platforms or compilers where no supported prefetch primitive is available,
the macro evaluates its arguments only sufficiently to suppress unused-value
warnings and otherwise performs no operation.

Callers should use prefetching only when measurement demonstrates a benefit.
Incorrect or excessive prefetching can reduce performance by consuming memory
bandwidth and polluting processor caches.
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

dest and src must point to valid ranges containing at least byte_count bytes when
byte_count is greater than zero.

The source and destination ranges must not overlap. This operation has memcpy
semantics rather than memmove semantics.

When byte_count is zero, the operation performs no work and dest/src may be
NULL.

When NEXUS_MEMORY_DEBUG_ENABLED is active, calls originating outside the memory
debugger are redirected through nexus_debug_mem_bytes_copy. The debugger then:
- validates source and destination ranges where possible;
- records source provenance through __FILE__ and __LINE__;
- emits a memory-debug logging event when a callback is installed.

The memory-debugger implementation itself bypasses this redirection to prevent
recursive entry and uses the direct implementation below.
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
nexus_memory_bytes_set writes byte into each of byte_count consecutive bytes
beginning at dest.

dest must point to a valid writable range containing at least byte_count bytes
when byte_count is greater than zero.

When byte_count is zero, the operation performs no work and dest may be NULL.

When NEXUS_MEMORY_DEBUG_ENABLED is active, calls originating outside the memory
debugger are redirected through nexus_debug_mem_bytes_set. The debugger then:
- validates the destination range where possible;
- records source provenance through __FILE__ and __LINE__;
- emits a memory-debug logging event when a callback is installed.

The memory-debugger implementation itself bypasses this redirection to prevent
recursive entry.
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
nexus_memory_bytes_clear writes zero into each of byte_count consecutive bytes
beginning at dest.

This is equivalent to:

  nexus_memory_bytes_set(dest, 0, byte_count)

but expresses the semantic intent of clearing or zero-initializing storage.

dest must point to a valid writable range containing at least byte_count bytes
when byte_count is greater than zero.

When byte_count is zero, the operation performs no work and dest may be NULL.

When NEXUS_MEMORY_DEBUG_ENABLED is active, calls originating outside the memory
debugger are redirected through nexus_debug_mem_bytes_clear so range validation,
source provenance, and memory-debug logging remain available.
*/
#if NEXUS_MEMORY_DEBUG_ENABLED && !defined(NEXUS_MEMORY_DEBUG_IMPLEMENTATION)

#  define nexus_memory_bytes_clear(dest, byte_count) nexus_debug_mem_bytes_clear((dest), (byte_count), __FILE__, __LINE__)

#else

static void nexus_memory_bytes_clear(void *dest, uint_large byte_count) /* NOLINT */ {
  nexus_memory_bytes_set(dest, 0, byte_count);
}

#endif

/*
nexus_memory_bytes_compare compares size bytes beginning at bytes_a and bytes_b.

The comparison has memcmp semantics:

- a value less than zero indicates that the first differing byte in bytes_a is
  less than the corresponding byte in bytes_b;
- zero indicates that both byte ranges are identical;
- a value greater than zero indicates that the first differing byte in bytes_a
  is greater than the corresponding byte in bytes_b.

Both pointers must identify readable ranges containing at least size bytes when
size is greater than zero.

Unlike nexus_memory_bytes_copy/set/clear, this operation currently does not
route through the memory debugger and therefore does not perform tracked-range
validation or emit memory-debug logging.
*/
static int nexus_memory_bytes_compare(const void *bytes_a, const void *bytes_b, uint_large size) { /* NOLINT(clang-diagnostic-unused-function) */
  return memcmp(bytes_a, bytes_b, (size_t)size);
}

/*
NEXUS_FREE_IF_NOT_NULL frees ptr when ptr is not NULL.

The macro exists primarily as a concise cleanup helper for optional allocations.

When memory debugging is enabled, free resolves through the normal Nexus
allocation redirection and the deallocation therefore remains tracked.

The macro does not assign NULL back to ptr after freeing it.

ptr should be a simple pointer expression. Because the expression may be
evaluated more than once, expressions with side effects must not be supplied.
*/
#define NEXUS_FREE_IF_NOT_NULL(ptr)                                                                                                                  \
  do {                                                                                                                                               \
    if ((ptr) != NULL) {                                                                                                                             \
      free((ptr));                                                                                                                                   \
    }                                                                                                                                                \
  } while (0)

/* ---------------------------------------------------------------------------- */
/* DEBUGGER-FRIENDLY PROCESS TERMINATION                                        */
/* ---------------------------------------------------------------------------- */

/*
NEXUS_EXIT_CRASH_ENABLED redirects exit to exit_crash.

This is intended for development builds where process termination should stop at
a deterministic debugger-visible fault rather than silently terminating through
the C runtime.

Default: enabled.
*/
#ifndef NEXUS_EXIT_CRASH_ENABLED
#  define NEXUS_EXIT_CRASH_ENABLED 1
#endif

/*
exit_crash terminates execution through an intentional invalid memory access so
an attached debugger breaks at the termination site.

status_code is retained for exit-compatible call sites and possible future use.
*/
extern void exit_crash(uint32 status_code);

#if NEXUS_EXIT_CRASH_ENABLED

#  if !NEXUS_MEMORY_DEBUG_ENABLED
#    include <stdlib.h>
#  endif

/*
NEXUS_EXIT_CRASH_IMPLEMENTATION must only be defined by the translation unit
implementing exit_crash so that its own implementation is not redirected.
*/
#  if !defined(NEXUS_EXIT_CRASH_IMPLEMENTATION)
#    define exit(n) exit_crash((uint32)(n))
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

The trailing line terminator is not included in the resulting string.
On CRLF input, both '\r' and '\n' are removed.

When stdin reaches end-of-file before a line is read, sets out_reached_eof to TRUE.

Returns NEXUS_ERROR_INTERRUPTED when the blocking read is interrupted by a
captured process signal.
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
NexusStringWriter incrementally builds one null-terminated caller buffer.

The writer never allocates. truncated becomes TRUE after any append does not
fully fit; later appends remain safe and preserve null termination.
*/
typedef struct NexusStringWriter {
  char      *buffer;
  uint_large capacity;
  uint_large length;
  boolean    truncated;
} NexusStringWriter;

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
Initializes a writer over buffer, replacing its previous contents with an empty
string. buffer_capacity must be nonzero.
*/
extern void nexus_strings_string_writer_initialize(NexusStringWriter *writer, char *buffer, uint_large buffer_capacity);

/*
Appends text or formatted text to a writer.

The returned result describes the current append. Use truncated_get() to learn
whether any append since initialization was truncated.
*/
extern NexusStringFormatResult nexus_strings_string_writer_string_append(NexusStringWriter *writer, const char *string);
extern NexusStringFormatResult nexus_strings_string_writer_format_append(NexusStringWriter *writer, const char *format, ...);
extern NexusStringFormatResult nexus_strings_string_writer_vformat_append(NexusStringWriter *writer, const char *format, va_list args);

extern uint_large nexus_strings_string_writer_length_get(const NexusStringWriter *writer);
extern boolean    nexus_strings_string_writer_truncated_get(const NexusStringWriter *writer);

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

/*
nexus_strings_string_duplicate allocates and returns an owned copy of string.

Returns NULL when string is NULL or allocation fails.
The returned allocation must be released with free.
*/
extern char *nexus_strings_string_duplicate(const char *string);

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
/* SIGNALS                                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusSignal identifies process-level signals that Nexus can capture portably.
*/
typedef enum NexusSignal {
  NEXUS_SIGNAL_INTERRUPT = 0,
  NEXUS_SIGNAL_TERMINATE,

  NEXUS_SIGNAL_COUNT
} NexusSignal;

/*
nexus_signals_capture installs Nexus handling for signal.

The signal is recorded when received and may subsequently be queried using
nexus_signals_received_get or consumed using nexus_signals_received_exchange.

Signal handlers perform no application cleanup themselves.
*/
extern NError nexus_signals_capture(NexusSignal signal);

/*
nexus_signals_release restores the platform's default handling for signal.
*/
extern NError nexus_signals_release(NexusSignal signal);

/*
nexus_signals_received_get returns TRUE when signal has been received since
capture or the most recent clear/exchange.
*/
extern boolean nexus_signals_received_get(NexusSignal signal);

/*
nexus_signals_received_exchange returns the current received state and clears it.
*/
extern boolean nexus_signals_received_exchange(NexusSignal signal);

/*
nexus_signals_received_clear clears the received state for signal.
*/
extern void nexus_signals_received_clear(NexusSignal signal);

/* ---------------------------------------------------------------------------- */
/* PROCESS                                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusProcessSpawnResult reports the exit status of a spawned child process.
*/
typedef struct NexusProcessSpawnResult {
  int32 exit_code;
} NexusProcessSpawnResult;

/*
NexusProcess is an owned handle for a live or completed child process created by
nexus_process_spawn_piped or nexus_process_spawn_with_child_channel.

For nexus_process_spawn_piped, the child receives a pipe/socket as stdin and
stdout while stderr remains inherited. nexus_process_spawn_with_child_channel
preserves all standard streams and provides its separate channel instead.
*/
typedef struct NexusProcess NexusProcess;

/*
NexusProcessChildChannel is a dedicated bidirectional byte stream between a
parent process and one directly launched child. The child endpoint is conveyed
through a caller-selected environment variable and must be adopted with
nexus_process_child_channel_open_from_environment.
*/
typedef struct NexusProcessChildChannel NexusProcessChildChannel;

/*
NexusProcessChildChannelStorage permits a child to adopt one inherited channel
without a heap allocation. It is intentionally opaque; callers must preserve
the storage until nexus_process_child_channel_destroy has been called.
*/
typedef union NexusProcessChildChannelStorage {
  uint_large native_values[4];
  real64     alignment;
} NexusProcessChildChannelStorage;

/*
Starts executable_path without redirecting its standard streams and creates a
dedicated parent/child byte channel. Nexus adds child_channel_environment_name
to the child environment so the child can adopt its endpoint after exec.

When environment is NULL, the current environment is inherited. Otherwise the
provided environment is copied with any existing value for
child_channel_environment_name replaced.
*/
extern NError nexus_process_spawn_with_child_channel(NexusPath executable_path, char *const *argv, char *const *environment,
                                                     const char *child_channel_environment_name, NexusProcess **out_process,
                                                     NexusProcessChildChannel **out_child_channel);

/*
Starts executable_path with a dedicated parent/child byte channel while
capturing the child's standard error stream.

Standard input and standard output remain inherited. Standard error is
available to the parent through nexus_process_stderr_read.

The standard error stream must be drained while the child is running to avoid
blocking a child that produces enough output to fill the underlying pipe.
*/
extern NError nexus_process_spawn_with_child_channel_stderr_piped(NexusPath executable_path, char *const *argv, char *const *environment,
                                                                  const char *child_channel_environment_name, NexusProcess **out_process,
                                                                  NexusProcessChildChannel **out_child_channel);

/*
Adopts this process' endpoint of a channel created by
nexus_process_spawn_with_child_channel. The adopted endpoint is made
non-inheritable immediately so descendant processes do not keep the channel
alive accidentally.
*/
extern NError nexus_process_child_channel_open_from_environment(const char *environment_name, NexusProcessChildChannel **out_channel);

/*
Equivalent to nexus_process_child_channel_open_from_environment, but uses
caller-owned storage instead of allocating the endpoint object.
*/
extern NError nexus_process_child_channel_open_from_environment_in_place(const char *environment_name, NexusProcessChildChannelStorage *storage,
                                                                         NexusProcessChildChannel **out_channel);

/*
Reads up to byte_count bytes from channel. out_reached_eof is TRUE only when
the peer has closed its write direction.
*/
extern NError nexus_process_child_channel_read(NexusProcessChildChannel *channel, byte *buffer, uint_large byte_count, uint_large *out_bytes_read,
                                               boolean *out_reached_eof);

/* Writes the complete byte buffer to channel, blocking as necessary. */
extern NError nexus_process_child_channel_write(NexusProcessChildChannel *channel, const byte *bytes, uint_large byte_count);

/* Closes both directions of channel and releases its owned memory. Accepts NULL. */
extern void nexus_process_child_channel_destroy(NexusProcessChildChannel *channel);

/*
Starts executable_path without waiting for it to exit.

argv and environment have the same semantics as nexus_process_spawn_wait.
stdin/stdout are connected to the parent through the functions below; stderr is
inherited.

On success, *out_process owns all child/process and I/O handles.
*/
extern NError nexus_process_spawn_piped(NexusPath executable_path, char *const *argv, char *const *environment, NexusProcess **out_process);

/*
Writes the complete byte buffer to the child's stdin, blocking as necessary.
Returns NEXUS_ERROR_IO if the pipe is closed or the child can no longer accept
input.
*/
extern NError nexus_process_stdin_write(NexusProcess *process, const byte *bytes, uint_large byte_count);

/*
Closes the parent's write side of the child's stdin. Safe to call more than once.
*/
extern NError nexus_process_stdin_close(NexusProcess *process);

/*
Reads up to byte_count bytes from the child's stdout.

out_bytes_read receives the amount read. out_reached_eof is TRUE only when the
child stdout pipe has reached EOF. A successful read may return fewer bytes than
requested.
*/
extern NError nexus_process_stdout_read(NexusProcess *process, byte *buffer, uint_large byte_count, uint_large *out_bytes_read,
                                        boolean *out_reached_eof);

/*
Reads up to byte_count bytes from a captured child standard error stream.

out_reached_eof is TRUE only once the child has closed the stream. For a
process whose stderr was not captured, this immediately reports EOF.
*/
extern NError nexus_process_stderr_read(NexusProcess *process, byte *buffer, uint_large byte_count, uint_large *out_bytes_read,
                                        boolean *out_reached_eof);

/*
Returns TRUE while the child has not yet exited. This is a snapshot.
*/
extern boolean nexus_process_running_get(NexusProcess *process);

/*
Waits for the child to terminate and stores its exit code in result.
Safe after nexus_process_running_get has observed completion.
*/
extern NError nexus_process_wait(NexusProcess *process, NexusProcessSpawnResult *result);

/*
Requests immediate child termination. This is a fallback teardown operation;
cooperative protocols should ask the child to exit normally first.
*/
extern NError nexus_process_terminate(NexusProcess *process);

/*
Releases the process and pipe handles.

If the child is still running, Nexus terminates and reaps it first so destroying
this object cannot orphan a child process.
*/
extern void nexus_process_destroy(NexusProcess *process);

/*
nexus_process_replace replaces the current process image with executable_path.

argv must be a NULL-terminated array whose first element is the executable path.
When environment is NULL, the current process environment is inherited.
This function does not return on success.
*/
extern NError nexus_process_replace(NexusPath executable_path, char *const *argv, char *const *environment);

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

/*
nexus_process_executable_resolve resolves an executable specification to an
absolute filesystem path.

When executable contains a path component, it is treated as an explicit path.
Relative explicit paths are resolved against the current working directory.

When executable contains only a file name, the platform executable search path
is searched.

On success, out_resolved_executable receives an absolute path to an executable
file.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_INVALID_ARGUMENT when executable is empty or
  out_resolved_executable is NULL.
- NEXUS_ERROR_FILE_NOT_FOUND when no matching executable can be found.
- NEXUS_ERROR_PERMISSION_DENIED when a matching POSIX file exists but is not
  executable.
- NEXUS_ERROR_CAPACITY when the resolved path exceeds Nexus path capacity.
- another Nexus error when an underlying platform operation fails.

The search uses the current process environment.
*/
extern NError nexus_process_executable_resolve(NexusPath executable, NexusPath *out_resolved_executable);

/* ---------------------------------------------------------------------------- */
/* THREADS                                                                      */
/* ---------------------------------------------------------------------------- */

typedef struct NexusThread           NexusThread;
typedef struct NexusMutex            NexusMutex;
typedef struct NexusCond             NexusCond;
typedef struct NexusSemaphore        NexusSemaphore;
typedef struct NexusThreadsWaitGroup NexusThreadsWaitGroup;

typedef void (*NexusThreadFunc)(void *user_data);

/*
nexus_threads_sleep suspends the calling thread for at least duration.

A duration of zero or less returns immediately.

Contract: duration.nanoseconds must not be negative (asserted).
*/
extern void nexus_threads_sleep(NexusDuration duration);

/*
nexus_threads_spin_wait actively waits for at least duration without voluntarily
yielding the calling thread to the operating system scheduler.

A duration of zero or less returns immediately.

Contract: duration.nanoseconds must not be negative (asserted).
*/
extern void nexus_threads_spin_wait(NexusDuration duration);

/*
nexus_threads_create spawns a new thread executing entry_func with user_data.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_CAPACITY if memory allocation fails.
- NEXUS_ERROR_IO if OS thread creation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if out_thread or entry_func is NULL.

Contract: out_thread and entry_func must not be NULL (asserted).
*/
extern NError nexus_threads_create(NexusThread **out_thread, NexusThreadFunc entry_func, void *user_data);

/*
nexus_threads_join blocks the calling thread until the target thread finishes execution.
Joining a thread automatically frees its handle resources.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS join operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if thread is NULL.

Contract: thread must not be NULL (asserted).
*/
extern NError nexus_threads_join(NexusThread *thread);

/*
nexus_threads_mutex_create allocates and initializes a recursive-capable OS mutex.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_CAPACITY if memory allocation fails.
- NEXUS_ERROR_IO if OS mutex initialization fails.
- NEXUS_ERROR_INVALID_ARGUMENT if out_mutex is NULL.

Contract: out_mutex must not be NULL (asserted).
*/
extern NError nexus_threads_mutex_create(NexusMutex **out_mutex);

/*
nexus_threads_mutex_lock blocks until exclusive ownership of the mutex is acquired.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS lock operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if mutex is NULL.

Contract: mutex must not be NULL (asserted).
*/
extern NError nexus_threads_mutex_lock(NexusMutex *mutex);

/*
nexus_threads_mutex_try_lock attempts to acquire exclusive ownership without blocking.

Returns TRUE on success (lock acquired), FALSE on failure or if mutex is NULL.

Contract: mutex must not be NULL (asserted).
*/
extern boolean nexus_threads_mutex_try_lock(NexusMutex *mutex);

/*
nexus_threads_mutex_unlock releases exclusive ownership of the mutex.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS unlock operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if mutex is NULL.

Contract: mutex must not be NULL (asserted).
*/
extern NError nexus_threads_mutex_unlock(NexusMutex *mutex);

/*
nexus_threads_mutex_destroy frees all resources associated with the mutex.
The mutex must be unlocked before destruction.

Contract: mutex must not be NULL (asserted).
*/
extern void nexus_threads_mutex_destroy(NexusMutex *mutex);

/*
nexus_threads_cond_create allocates and initializes a condition variable.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_CAPACITY if memory allocation fails.
- NEXUS_ERROR_IO if OS condition variable initialization fails.
- NEXUS_ERROR_INVALID_ARGUMENT if out_cond is NULL.

Contract: out_cond must not be NULL (asserted).
*/
extern NError nexus_threads_cond_create(NexusCond **out_cond);

/*
nexus_threads_cond_wait atomically unlocks the provided mutex and blocks until signaled.
Re-acquires the mutex prior to returning.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS wait operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if cond or mutex is NULL.

Contract: cond and mutex must not be NULL (asserted).
*/
extern NError nexus_threads_cond_wait(NexusCond *cond, NexusMutex *mutex);

/*
nexus_threads_cond_wait_timeout waits for a signal or until the timeout duration elapses.

Returns TRUE if signaled, FALSE if timed out, failed, or if arguments are invalid.

Contract: cond and mutex must not be NULL, and duration must be strictly positive (asserted).
*/
extern boolean nexus_threads_cond_wait_timeout(NexusCond *cond, NexusMutex *mutex, NexusDuration duration);

/*
nexus_threads_cond_signal wakes up at least one thread waiting on the condition variable.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS signal operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if cond is NULL.

Contract: cond must not be NULL (asserted).
*/
extern NError nexus_threads_cond_signal(NexusCond *cond);

/*
nexus_threads_cond_broadcast wakes up all threads currently waiting on the condition variable.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS broadcast operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if cond is NULL.

Contract: cond must not be NULL (asserted).
*/
extern NError nexus_threads_cond_broadcast(NexusCond *cond);

/*
nexus_threads_cond_destroy frees resources associated with the condition variable.

Contract: cond must not be NULL (asserted).
*/
extern void nexus_threads_cond_destroy(NexusCond *cond);

/*
nexus_threads_semaphore_create allocates and initializes a counting semaphore.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_CAPACITY if memory allocation fails.
- NEXUS_ERROR_IO if OS semaphore initialization fails.
- NEXUS_ERROR_INVALID_ARGUMENT if out_sem is NULL.

Contract: out_sem must not be NULL (asserted).
*/
extern NError nexus_threads_semaphore_create(NexusSemaphore **out_sem, uint32 initial_count);

/*
nexus_threads_semaphore_wait decrements the semaphore counter, blocking if zero.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS wait operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if sem is NULL.

Contract: sem must not be NULL (asserted).
*/
extern NError nexus_threads_semaphore_wait(NexusSemaphore *sem);

/*
nexus_threads_semaphore_post increments the semaphore counter, unblocking a waiting thread.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS post operation fails.
- NEXUS_ERROR_INVALID_ARGUMENT if sem is NULL.

Contract: sem must not be NULL (asserted).
*/
extern NError nexus_threads_semaphore_post(NexusSemaphore *sem);

/*
nexus_threads_semaphore_destroy frees resources associated with the semaphore.

Contract: sem must not be NULL (asserted).
*/
extern void nexus_threads_semaphore_destroy(NexusSemaphore *sem);

/*
nexus_threads_waitgroup_create allocates and initializes a new waitgroup.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_CAPACITY if memory allocation fails.
- NEXUS_ERROR_IO if OS primitive initialization fails.
- NEXUS_ERROR_INVALID_ARGUMENT if out_waitgroup is NULL.

Contract: out_waitgroup must not be NULL (asserted).
*/
extern NError nexus_threads_waitgroup_create(NexusThreadsWaitGroup **out_waitgroup);

/*
nexus_threads_waitgroup_add increments or decrements the waitgroup counter by delta.
If the counter drops to zero, all waiting threads are unblocked.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if underlying OS primitive operations fail.
- NEXUS_ERROR_INVALID_ARGUMENT if waitgroup is NULL or delta is 0.

Contract: waitgroup must not be NULL, delta must not be 0, and the internal counter
must never drop below zero (asserted).
*/
extern NError nexus_threads_waitgroup_add(NexusThreadsWaitGroup *waitgroup, int32 delta);

/*
nexus_threads_waitgroup_done decrements the waitgroup counter by 1.
Convenience wrapper around nexus_threads_waitgroup_add(wg, -1).

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if underlying OS primitive operations fail.
- NEXUS_ERROR_INVALID_ARGUMENT if waitgroup is NULL.

Contract: waitgroup must not be NULL (asserted).
*/
extern NError nexus_threads_waitgroup_done(NexusThreadsWaitGroup *waitgroup);

/*
nexus_threads_waitgroup_wait blocks the calling thread until the waitgroup counter
reaches zero. If the counter is already zero or negative, it returns immediately.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if underlying OS primitive operations fail.
- NEXUS_ERROR_INVALID_ARGUMENT if waitgroup is NULL.

Contract: waitgroup must not be NULL (asserted).
*/
extern NError nexus_threads_waitgroup_wait(NexusThreadsWaitGroup *waitgroup);

/*
nexus_threads_waitgroup_try_wait returns TRUE when the waitgroup counter is currently zero.

Unlike nexus_threads_waitgroup_wait, this function never blocks. The result is only a
snapshot; another thread may add work immediately afterward.
*/
extern boolean nexus_threads_waitgroup_try_wait(NexusThreadsWaitGroup *waitgroup);

/*
nexus_threads_waitgroup_destroy frees all platform resources associated with the waitgroup.
The waitgroup must not have active waiting threads during destruction.

Contract: waitgroup must not be NULL (asserted).
*/
extern void nexus_threads_waitgroup_destroy(NexusThreadsWaitGroup *waitgroup);

/*
Atomic values are concrete, allocation-free synchronization primitives.

All atomic operations use sequentially consistent ordering.

Atomic values may be embedded directly inside structures and may be initialized
to zero using ordinary C aggregate initialization:

  NexusAtomicUint32 counter = { 0 };
  NexusAtomicBoolean ready  = { 0 };
  NexusAtomicPointer state  = { 0 };

Once an atomic value becomes accessible concurrently, its storage must only be
accessed through the atomic API.

Atomic values must be naturally aligned. Nexus enforces the alignment required
by the fixed-width atomic types.

The atomic API itself never allocates memory and never uses NexusMutex.
On platforms where the compiler cannot emit a lock-free operation for a given
width, the compiler/runtime may provide an implementation fallback.
*/

#if defined(_MSC_VER)
#  define NEXUS_THREADS_ATOMIC_ALIGN(bytes) __declspec(align(bytes))
#elif defined(__GNUC__) || defined(__clang__)
#  define NEXUS_THREADS_ATOMIC_ALIGN(bytes) __attribute__((aligned(bytes)))
#else
#  define NEXUS_THREADS_ATOMIC_ALIGN(bytes)
#endif

typedef struct NEXUS_THREADS_ATOMIC_ALIGN(4) NexusAtomicInt32 {
  volatile int32 storage;
} NexusAtomicInt32;

typedef struct NEXUS_THREADS_ATOMIC_ALIGN(4) NexusAtomicUint32 {
  volatile uint32 storage;
} NexusAtomicUint32;

typedef struct NEXUS_THREADS_ATOMIC_ALIGN(8) NexusAtomicInt64 {
  volatile int64 storage;
} NexusAtomicInt64;

typedef struct NEXUS_THREADS_ATOMIC_ALIGN(8) NexusAtomicUint64 {
  volatile uint64 storage;
} NexusAtomicUint64;

#if NEXUS_ARCHITECTURE_BITS == 64
typedef NexusAtomicInt64  NexusAtomicIntLarge;
typedef NexusAtomicUint64 NexusAtomicUintLarge;
#else
typedef NexusAtomicInt32  NexusAtomicIntLarge;
typedef NexusAtomicUint32 NexusAtomicUintLarge;
#endif

/*
NexusAtomicBoolean internally uses a 32-bit word rather than boolean's byte-sized
representation because 32-bit atomic operations are universally better supported
by the target platforms.
*/
typedef struct NEXUS_THREADS_ATOMIC_ALIGN(4) NexusAtomicBoolean {
  volatile uint32 storage;
} NexusAtomicBoolean;

typedef struct NexusAtomicPointer {
  void *volatile storage;
} NexusAtomicPointer;

#undef NEXUS_THREADS_ATOMIC_ALIGN

/*
nexus_threads_atomic_int32_load atomically returns the current value.
*/
extern int32 nexus_threads_atomic_int32_load(NexusAtomicInt32 *atomic);

/*
nexus_threads_atomic_int32_store atomically replaces the current value.
*/
extern void nexus_threads_atomic_int32_store(NexusAtomicInt32 *atomic, int32 value);

/*
nexus_threads_atomic_int32_swap atomically replaces the current value and returns
the previous value.
*/
extern int32 nexus_threads_atomic_int32_swap(NexusAtomicInt32 *atomic, int32 value);

/*
nexus_threads_atomic_int32_compare_exchange replaces old_value with new_value
only when the current value equals old_value.

Returns TRUE if the replacement occurred, otherwise FALSE.
*/
extern boolean nexus_threads_atomic_int32_compare_exchange(NexusAtomicInt32 *atomic, int32 old_value, int32 new_value);

/*
nexus_threads_atomic_int32_add atomically adds delta and returns the new value.

Arithmetic wraps modulo 2^32.
*/
extern int32 nexus_threads_atomic_int32_add(NexusAtomicInt32 *atomic, int32 delta);

/*
nexus_threads_atomic_uint32_load atomically returns the current value.
*/
extern uint32 nexus_threads_atomic_uint32_load(NexusAtomicUint32 *atomic);

/*
nexus_threads_atomic_uint32_store atomically replaces the current value.
*/
extern void nexus_threads_atomic_uint32_store(NexusAtomicUint32 *atomic, uint32 value);

/*
nexus_threads_atomic_uint32_swap atomically replaces the current value and returns
the previous value.
*/
extern uint32 nexus_threads_atomic_uint32_swap(NexusAtomicUint32 *atomic, uint32 value);

/*
nexus_threads_atomic_uint32_compare_exchange replaces old_value with new_value
only when the current value equals old_value.

Returns TRUE if the replacement occurred, otherwise FALSE.
*/
extern boolean nexus_threads_atomic_uint32_compare_exchange(NexusAtomicUint32 *atomic, uint32 old_value, uint32 new_value);

/*
nexus_threads_atomic_uint32_add atomically adds delta and returns the new value.

Arithmetic wraps modulo 2^32.
*/
extern uint32 nexus_threads_atomic_uint32_add(NexusAtomicUint32 *atomic, uint32 delta);

/*
nexus_threads_atomic_int64_load atomically returns the current value.
*/
extern int64 nexus_threads_atomic_int64_load(NexusAtomicInt64 *atomic);

/*
nexus_threads_atomic_int64_store atomically replaces the current value.
*/
extern void nexus_threads_atomic_int64_store(NexusAtomicInt64 *atomic, int64 value);

/*
nexus_threads_atomic_int64_swap atomically replaces the current value and returns
the previous value.
*/
extern int64 nexus_threads_atomic_int64_swap(NexusAtomicInt64 *atomic, int64 value);

/*
nexus_threads_atomic_int64_compare_exchange replaces old_value with new_value
only when the current value equals old_value.

Returns TRUE if the replacement occurred, otherwise FALSE.
*/
extern boolean nexus_threads_atomic_int64_compare_exchange(NexusAtomicInt64 *atomic, int64 old_value, int64 new_value);

/*
nexus_threads_atomic_int64_add atomically adds delta and returns the new value.

Arithmetic wraps modulo 2^64.
*/
extern int64 nexus_threads_atomic_int64_add(NexusAtomicInt64 *atomic, int64 delta);

/*
nexus_threads_atomic_uint64_load atomically returns the current value.
*/
extern uint64 nexus_threads_atomic_uint64_load(NexusAtomicUint64 *atomic);

/*
nexus_threads_atomic_uint64_store atomically replaces the current value.
*/
extern void nexus_threads_atomic_uint64_store(NexusAtomicUint64 *atomic, uint64 value);

/*
nexus_threads_atomic_uint64_swap atomically replaces the current value and returns
the previous value.
*/
extern uint64 nexus_threads_atomic_uint64_swap(NexusAtomicUint64 *atomic, uint64 value);

/*
nexus_threads_atomic_uint64_compare_exchange replaces old_value with new_value
only when the current value equals old_value.

Returns TRUE if the replacement occurred, otherwise FALSE.
*/
extern boolean nexus_threads_atomic_uint64_compare_exchange(NexusAtomicUint64 *atomic, uint64 old_value, uint64 new_value);

/*
nexus_threads_atomic_uint64_add atomically adds delta and returns the new value.

Arithmetic wraps modulo 2^64.
*/
extern uint64 nexus_threads_atomic_uint64_add(NexusAtomicUint64 *atomic, uint64 delta);

/*
Architecture-sized integer atomics.

These map directly to the corresponding fixed-width atomic implementation:
32 bits on 32-bit targets and 64 bits on 64-bit targets.
*/
extern int_large nexus_threads_atomic_int_large_load(NexusAtomicIntLarge *atomic);
extern void      nexus_threads_atomic_int_large_store(NexusAtomicIntLarge *atomic, int_large value);
extern int_large nexus_threads_atomic_int_large_swap(NexusAtomicIntLarge *atomic, int_large value);
extern boolean   nexus_threads_atomic_int_large_compare_exchange(NexusAtomicIntLarge *atomic, int_large old_value, int_large new_value);
extern int_large nexus_threads_atomic_int_large_add(NexusAtomicIntLarge *atomic, int_large delta);

extern uint_large nexus_threads_atomic_uint_large_load(NexusAtomicUintLarge *atomic);
extern void       nexus_threads_atomic_uint_large_store(NexusAtomicUintLarge *atomic, uint_large value);
extern uint_large nexus_threads_atomic_uint_large_swap(NexusAtomicUintLarge *atomic, uint_large value);
extern boolean    nexus_threads_atomic_uint_large_compare_exchange(NexusAtomicUintLarge *atomic, uint_large old_value, uint_large new_value);
extern uint_large nexus_threads_atomic_uint_large_add(NexusAtomicUintLarge *atomic, uint_large delta);

/*
Boolean atomics normalize stored values to TRUE or FALSE.
*/
extern boolean nexus_threads_atomic_boolean_load(NexusAtomicBoolean *atomic);
extern void    nexus_threads_atomic_boolean_store(NexusAtomicBoolean *atomic, boolean value);
extern boolean nexus_threads_atomic_boolean_swap(NexusAtomicBoolean *atomic, boolean value);
extern boolean nexus_threads_atomic_boolean_compare_exchange(NexusAtomicBoolean *atomic, boolean old_value, boolean new_value);

/*
Pointer atomics operate on the pointer value itself.

They do not provide synchronization for mutation of the pointed-to object and do
not provide object lifetime management.
*/
extern void   *nexus_threads_atomic_pointer_load(NexusAtomicPointer *atomic);
extern void    nexus_threads_atomic_pointer_store(NexusAtomicPointer *atomic, void *value);
extern void   *nexus_threads_atomic_pointer_swap(NexusAtomicPointer *atomic, void *value);
extern boolean nexus_threads_atomic_pointer_compare_exchange(NexusAtomicPointer *atomic, void *old_value, void *new_value);

/* ---------------------------------------------------------------------------- */
/* ASYNC EVENT MULTIPLEXER                                                      */
/* ---------------------------------------------------------------------------- */

typedef struct NexusAsyncPoller NexusAsyncPoller;

#if defined(NEXUS_PLATFORM_WINDOWS)
typedef UINT_PTR NexusNativeHandle;
#else
typedef int NexusNativeHandle;
#endif

typedef enum NexusAsyncInterest {
  NEXUS_ASYNC_INTEREST_READ  = 1U << 0,
  NEXUS_ASYNC_INTEREST_WRITE = 1U << 1,
  NEXUS_ASYNC_INTEREST_ERROR = 1U << 2,
  NEXUS_ASYNC_INTEREST_EDGE  = 1U << 3
} NexusAsyncInterest;

typedef struct NexusAsyncEvent {
  NexusNativeHandle handle;
  uint32            events; /* Bitmask of NexusAsyncInterest */
  void             *user_data;
} NexusAsyncEvent;

/*
nexus_async_set_nonblocking toggles non-blocking I/O mode on an OS socket descriptor.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the underlying OS control operation fails.
*/
extern NError nexus_async_set_nonblocking(NexusNativeHandle handle, boolean non_blocking);

/*
nexus_async_poller_create allocates and initializes an OS event demultiplexer
(epoll on Linux, kqueue on macOS/BSD, WSAPoll on Windows).

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_CAPACITY if memory allocation fails.
- NEXUS_ERROR_IO if OS poller initialization fails.
- NEXUS_ERROR_INVALID_ARGUMENT if out_poller is NULL.

Contract: out_poller must not be NULL (asserted).
*/
extern NError nexus_async_poller_create(NexusAsyncPoller **out_poller);

/*
nexus_async_poller_add registers a file descriptor with the poller for readiness monitoring.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the registration fails or handle is already registered.
- NEXUS_ERROR_INVALID_ARGUMENT if poller is NULL or interests is 0.

Contract: poller must not be NULL and interests must not be 0 (asserted).
*/
extern NError nexus_async_poller_add(NexusAsyncPoller *poller, NexusNativeHandle handle, uint32 interests, void *user_data);

/*
nexus_async_poller_modify updates the monitored event interests and user context for a handle.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if the handle is not registered with the poller.
- NEXUS_ERROR_INVALID_ARGUMENT if poller is NULL or interests is 0.

Contract: poller must not be NULL and interests must not be 0 (asserted).
*/
extern NError nexus_async_poller_modify(NexusAsyncPoller *poller, NexusNativeHandle handle, uint32 interests, void *user_data);

/*
nexus_async_poller_remove unregisters a file descriptor from the poller.

Returns:
- NEXUS_ERROR_NONE on success.
- NEXUS_ERROR_IO if unregistering fails or handle is not registered.
- NEXUS_ERROR_INVALID_ARGUMENT if poller is NULL.

Contract: poller must not be NULL (asserted).
*/
extern NError nexus_async_poller_remove(NexusAsyncPoller *poller, NexusNativeHandle handle);

/*
nexus_async_poller_wait blocks up to duration waiting for active readiness events.

Passing a duration of zero or less performs a non-blocking poll.
Passing duration with nanoseconds set to -1 blocks indefinitely.

Returns:
- NEXUS_ERROR_NONE on success (out_event_count contains the number of fired events).
- NEXUS_ERROR_IO if polling fails at OS level.
- NEXUS_ERROR_INVALID_ARGUMENT if poller, out_events, or out_event_count is NULL.

Contract: poller, out_events, and out_event_count must not be NULL (asserted).
*/
extern NError nexus_async_poller_wait(NexusAsyncPoller *poller, NexusAsyncEvent *out_events, uint32 max_events, NexusDuration duration,
                                      uint32 *out_event_count);

/*
nexus_async_poller_destroy releases all OS handles and memory associated with the poller.

Contract: poller must not be NULL (asserted).
*/
extern void nexus_async_poller_destroy(NexusAsyncPoller *poller);

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
NexusFilesystemDirectoryVisitCallback is called once for each direct child of a
directory. entry_path is the complete child path and is valid only for the
duration of the callback.

Returning an error stops iteration and makes nexus_filesystem_directory_visit
return that error.
*/
typedef NError NexusFilesystemDirectoryVisitCallback(NexusPath entry_path, void *user_data);

/*
nexus_filesystem_directory_visit visits each direct child of directory_path.

The visit order is filesystem-defined. The function does not recurse and does
not report the synthetic . or .. entries.

callback must not be NULL.
*/
extern NError nexus_filesystem_directory_visit(NexusPath directory_path, NexusFilesystemDirectoryVisitCallback *callback, void *user_data);

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
/* ENTROPY                                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusEntropyFillFunc writes byte_count bytes of entropy into out_bytes.

user_data is provider-defined opaque state.

The function must write exactly byte_count bytes when returning
NEXUS_ERROR_NONE.

The quality and security properties of the produced bytes are determined by the
provider. Callers which require cryptographically secure entropy must use a
provider documented to provide cryptographically secure random bytes.

Returns NEXUS_ERROR_NONE on success or an appropriate error when entropy could
not be produced.
*/
typedef NError NexusEntropyFillFunc(void *user_data, byte *out_bytes, uint64 byte_count);

/*
nexus_entropy_system_fill writes cryptographically secure random bytes supplied
by the operating system.

This is the default entropy source for Nexus functionality which requires
unpredictable random data, including UUIDv4 and the random portions of UUIDv7.

user_data is ignored and may be NULL.

Platform implementations use the operating system's native cryptographically
secure random facility.

Returns NEXUS_ERROR_NONE on success.
Returns NEXUS_ERROR_IO when the operating system entropy source fails.
*/
extern NError nexus_entropy_system_fill(void *user_data, byte *out_bytes, uint64 byte_count);

/* ---------------------------------------------------------------------------- */
/* HASHING                                                                      */
/* ---------------------------------------------------------------------------- */

/*
NexusHash is reserved for future general-purpose hash containers.
*/
typedef struct NexusHash NexusHash;
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
nexus_hash_zobrist_table_fill fills each key in table with an independent
provider-generated bitstring.

fill determines the entropy/randomness source. Zobrist hashing itself does not
require cryptographically secure randomness; deterministic PRNG providers are
normally preferable when reproducibility is required.

Returns NEXUS_ERROR_NONE on success or propagates an error returned by fill.
*/
static NError nexus_hash_zobrist_table_fill(NexusHashZobristTable *table, NexusEntropyFillFunc *fill, void *user_data) /* NOLINT */
{
  NError error;

  uint64 i;

  byte key_bytes[8];

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(fill != NULL);
  NEXUS_ASSERT_DEBUG(table->key_count == 0 || table->keys != NULL);

  if (table == NULL || fill == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (table->key_count > 0 && table->keys == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  for (i = 0; i < table->key_count; i++) {
    error = fill(user_data, key_bytes, NEXUS_SIZEOF(key_bytes));

    if (error != NEXUS_ERROR_NONE) {
      return error;
    }

    table->keys[i] = nexus_bits_uint64_from_bytes_lsb(key_bytes);
  }

  return NEXUS_ERROR_NONE;
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

#define NEXUS_HASH_SHA1_DIGEST_SIZE 20
#define NEXUS_HASH_SHA1_BLOCK_SIZE  64

/*
NexusHashSHA1Context stores the state of an incremental SHA-1 computation.

SHA-1 is provided primarily for compatibility with algorithms and protocols
which specifically require SHA-1, including UUID version 5.

SHA-1 should not be selected for new cryptographic integrity or signature
schemes where collision resistance is required.
*/
typedef struct NexusHashSHA1Context {
  uint32 state[5];

  uint64 total_byte_count;

  byte   block[NEXUS_HASH_SHA1_BLOCK_SIZE];
  uint32 block_byte_count;
} NexusHashSHA1Context;

/*
nexus_hash_sha1_begin initializes context for a new SHA-1 computation.
*/
extern void nexus_hash_sha1_begin(NexusHashSHA1Context *context);

/*
nexus_hash_sha1_bytes appends byte_count bytes from data to an incremental SHA-1
computation.

data may be NULL only when byte_count is zero.
*/
extern void nexus_hash_sha1_bytes(NexusHashSHA1Context *context, const void *data, uint64 byte_count);

/*
nexus_hash_sha1_end finalizes context and writes the 20-byte SHA-1 digest in
network/big-endian byte order.

The context should be considered finalized after this call and must be passed
to nexus_hash_sha1_begin before reuse.
*/
extern void nexus_hash_sha1_end(NexusHashSHA1Context *context, byte out_digest[NEXUS_HASH_SHA1_DIGEST_SIZE]);

/*
nexus_hash_sha1 computes SHA-1 over byte_count bytes in one operation.

Equivalent to begin + bytes + end.
*/
extern void nexus_hash_sha1(const void *data, uint64 byte_count, byte out_digest[NEXUS_HASH_SHA1_DIGEST_SIZE]);

/* ---------------------------------------------------------------------------- */
/* IDENTITY                                                                     */
/* ---------------------------------------------------------------------------- */

#ifndef NEXUS_UUID_STRING_LENGTH
#  define NEXUS_UUID_STRING_LENGTH      36
#  define NEXUS_UUID_STRING_BUFFER_SIZE 37
#endif

/*
NexusUUID is a 128-bit universally unique identifier.

The numeric representation follows normal uint128 semantics:
hi contains the most significant 64 bits and lo contains the least significant
64 bits.

UUID serialization is performed in network / big-endian byte order.
*/
typedef uint128 NexusUUID;

/*
nexus_identity_uuid_string_write writes uuid in canonical UUID form:

xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx

Hexadecimal characters are lowercase. out_string must provide at least
NEXUS_UUID_STRING_BUFFER_SIZE bytes.
*/
extern void nexus_identity_uuid_string_write(NexusUUID uuid, char out_string[NEXUS_UUID_STRING_BUFFER_SIZE]);

/*
nexus_identity_uuid_string_parse parses a canonical UUID string.

Leading and trailing ASCII whitespace is ignored. Both lowercase and uppercase
hexadecimal characters are accepted.

Returns NEXUS_ERROR_NONE on success or NEXUS_ERROR_INVALID_ARGUMENT when the
input is malformed.
*/
extern NError nexus_identity_uuid_string_parse(const char *string, NexusUUID *out_uuid);

/*
nexus_identity_uuid_v4_generate generates an RFC 9562 UUID version 4 using the
operating system entropy source.
*/
extern NError nexus_identity_uuid_v4_generate(NexusUUID *out_uuid);

/*
nexus_identity_uuid_v4_generate_with_entropy generates an RFC 9562 UUID version
4 using entropy_fill.

The UUID format remains RFC-compliant with any provider supplying appropriate
random or pseudorandom data; uniqueness and unpredictability depend on the
quality and state management of that provider.
*/
extern NError nexus_identity_uuid_v4_generate_with_entropy(NexusEntropyFillFunc *entropy_fill, void *entropy_user_data, NexusUUID *out_uuid);

/*
nexus_identity_uuid_v5_generate generates an RFC 9562 UUID version 5 from
namespace_uuid and the bytes of name.

name is interpreted as a null-terminated byte string. UTF-8 text therefore has
the same semantics as the corresponding UTF-8 byte sequence.
*/
extern NError nexus_identity_uuid_v5_generate(NexusUUID namespace_uuid, const char *name, NexusUUID *out_uuid);

/*
nexus_identity_uuid_v5_generate_bytes is the binary-safe version of
nexus_identity_uuid_v5_generate.

This form should be used when the name can contain embedded zero bytes.
*/
extern NError nexus_identity_uuid_v5_generate_bytes(NexusUUID namespace_uuid, const void *name, uint_large name_size, NexusUUID *out_uuid);

/*
nexus_identity_uuid_v7_generate_random generates a random-field UUIDv7 using
the operating system entropy source.
*/
extern NError nexus_identity_uuid_v7_generate_random(NexusUUID *out_uuid);

/*
nexus_identity_uuid_v7_generate_random_with_entropy is the injectable-entropy
form of nexus_identity_uuid_v7_generate_random.
*/
extern NError nexus_identity_uuid_v7_generate_random_with_entropy(NexusEntropyFillFunc *entropy_fill, void *entropy_user_data, NexusUUID *out_uuid);

/*
nexus_identity_uuid_v7_generate_monotonic generates a process-monotonic UUIDv7
using the operating system entropy source for its random suffix.
*/
extern NError nexus_identity_uuid_v7_generate_monotonic(NexusUUID *out_uuid);

/*
nexus_identity_uuid_v7_generate_monotonic_with_entropy is the injectable
entropy form of nexus_identity_uuid_v7_generate_monotonic.
*/
extern NError nexus_identity_uuid_v7_generate_monotonic_with_entropy(NexusEntropyFillFunc *entropy_fill, void *entropy_user_data,
                                                                     NexusUUID *out_uuid);

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
nexus_hardware_cpu_retired_instructions_get retrieves the cumulative retired
instruction count for the current thread when supported. On Linux this uses
perf_event_open to program the CPU PMU. User access may require permissive
perf_event_paranoid sysctl settings.

Returns NEXUS_ERROR_INVALID_ARGUMENT when count is NULL.
Returns NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE when the platform does not expose this counter.
Returns NEXUS_ERROR_PERMISSION_DENIED when the kernel denies PMU access.
Returns NEXUS_ERROR_IO when the platform query fails.
*/
extern NError nexus_hardware_cpu_retired_instructions_get(uint64 *count);

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

typedef void NexusHashMap;

/*
HashMap implementation is Swiss-Table based: https://pratikpandey.substack.com/p/swisstables-high-performance-hashmaps
*/

extern NexusHashMap *nexus_data_hashmap_create(uint_large key_size_bytes, uint_large value_size_bytes, uint_large initial_capacity_groups,
                                               uint64 hash_seed);
extern void          nexus_data_hashmap_destroy(NexusHashMap *hashmap);

extern void nexus_data_hashmap_put(NexusHashMap *hashmap_handle, const void *key, const void *value);
/* Returned value storage is invalidated by a resize or destruction. */
extern void   *nexus_data_hashmap_get(NexusHashMap *hashmap_handle, const void *key);
extern boolean nexus_data_hashmap_get_copy(NexusHashMap *hashmap_handle, const void *key, void *out_value);
/* Returns existing or newly claimed value storage; initialize new storage before further map use. */
extern void   *nexus_data_hashmap_insert(NexusHashMap *hashmap_handle, const void *key);
extern boolean nexus_data_hashmap_get_keys_allocated(NexusHashMap *hashmap_handle, void **out_keys_buffer, uint_large *out_count);
extern boolean nexus_data_hashmap_get_values_allocated(NexusHashMap *hashmap_handle, void **out_values_buffer, uint_large *out_count);
extern boolean nexus_data_hashmap_get_entries_allocated(NexusHashMap *hashmap_handle, void **out_keys_buffer, void **out_values_buffer,
                                                        uint_large *out_count);
extern boolean nexus_data_hashmap_delete(NexusHashMap *hashmap_handle, const void *key);

typedef struct {
  NexusHashMap *hashmap;
  uint_large    group_idx;
  uint8         slot_idx;
} NexusHashMapEnumerator;

extern void    nexus_data_hashmap_enumerator_init(NexusHashMapEnumerator *enumerator);
extern boolean nexus_data_hashmap_enumerator_next(NexusHashMapEnumerator *enumerator, void **out_key, void **out_value);

/*
nexus_data_array_reserve ensures array has storage for required_count elements.

array points to the caller's allocation pointer.
capacity points to the caller's current element capacity.
element_size is the size of one element.

The initial allocation capacity is 4 elements and subsequent growth doubles.
When *array is NULL, malloc is used. realloc is only used for an existing
allocation.

Returns TRUE when sufficient capacity exists after the call.
*/
extern boolean nexus_data_array_reserve(void **array, uint32 *capacity, uint32 required_count, uint_large element_size);
