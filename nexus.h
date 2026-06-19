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
#    define NEXUS_PLATFORM_BSD 1
#  elif defined(__NetBSD__)
#    define NEXUS_PLATFORM_NETBSD 1
#    define NEXUS_PLATFORM_BSD 1
#  elif defined(__OpenBSD__)
#    define NEXUS_PLATFORM_OPENBSD 1
#    define NEXUS_PLATFORM_BSD 1
#  elif defined(__DragonFly__)
#    define NEXUS_PLATFORM_DRAGONFLY 1
#    define NEXUS_PLATFORM_BSD 1
#  elif defined(__bsdi__) || defined(__bsdi)
#    define NEXUS_PLATFORM_BSDI 1
#    define NEXUS_PLATFORM_BSD 1
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
#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__) || defined(__amd64__) || \
    defined(__riscv64) || (defined(__riscv_xlen) && __riscv_xlen == 64)
#  define NEXUS_ARCHITECTURE_BITS_NATIVE 64
#elif defined(_WIN32) || defined(__i386__) || defined(_M_IX86) || defined(__arm__) || defined(__ARMEL__) || defined(__ARMEB__) || \
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
#elif defined(__GNUC__) || defined(__clang__) || defined(__LP64__) || defined(_LP64)
#  define INT64_MAX_VAL  LLONG_MAX
#  define INT64_MIN_VAL  LLONG_MIN
#  define UINT64_MAX_VAL ULLONG_MAX
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

#define REAL64_MAX_VAL DBL_MAX
#define REAL64_MIN_VAL DBL_MIN

#if NEXUS_DOUBLE_PRECISION
#  define F_REAL_MAX REAL64_MAX_VAL
#  define F_REAL_MIN REAL64_MIN_VAL
#else
#  define F_REAL_MAX REAL32_MAX_VAL
#  define F_REAL_MIN REAL32_MIN_VAL
#endif

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
nexus_debug_mem_active enables or disables recording of new allocation statistics.
Existing tracked blocks remain in the table; only subsequent malloc, calloc, and realloc calls
respect the active flag for byte totals and allocation counts.
*/
extern void nexus_debug_mem_active(boolean active);

/*
NexusDebugMemLogCallback is invoked for each tracked malloc, calloc, realloc, and free when logging
is enabled. message is fully formatted; file and line identify the allocation call site.
*/
typedef void NexusDebugMemLogCallback(void *user_data, const char *message, const char *file, uint32 line);

/*
nexus_debug_mem_log_callback_set registers a callback for allocation tracing.
Pass NULL callback to disable. The callback is not invoked re-entrantly (nested allocations during
logging are not logged). Thread-safe when initialized via nexus_debug_mem_thread_safe_init.
*/
extern void nexus_debug_mem_log_callback_set(NexusDebugMemLogCallback *callback, void *user_data);

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
Counters respect nexus_debug_mem_active and are cleared by nexus_debug_mem_reset.
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
Lists each call site whose net allocation count (allocated minus freed) exceeds min_allocs,
including byte totals, live allocation counts, and any comments registered on live blocks.
*/
extern void nexus_debug_mem_print(uint32 min_allocs);

/*
nexus_debug_mem_reset clears per-site byte totals and allocation counters without freeing live
memory or discarding tracked blocks. Use to ignore allocations made before a known baseline.
*/
extern void nexus_debug_mem_reset(void);

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
otherwise a warning is printed first.
*/
extern boolean nexus_debug_mem_query_is_allocated(void *pointer, size_t size, boolean ignore_not_found);

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
#    define nexus_debug_mem_log_callback_set(n, m)
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

#if NEXUS_EXIT_CRASH_ENABLED

#  if !NEXUS_MEMORY_DEBUG_ENABLED
#    include <stdlib.h>
#  endif

#  if !defined(NEXUS_EXIT_CRASH_IMPLEMENTATION)
#    define exit(n) exit_crash(n)
#  endif

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

/* ---------------------------------------------------------------------------- */
/* MEMORY                                                                       */
/* ---------------------------------------------------------------------------- */

/*
nexus_memory_bytes_copy copies byte_count bytes from src into dest.

dest and src must not be NULL when byte_count is greater than zero.
The regions must not overlap.
*/
extern void nexus_memory_bytes_copy(void *dest, const void *src, uint_large byte_count);

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
nexus_strings_bytes_format writes byte_count as a human-readable binary size (B, KiB, MiB, GiB, TiB).
Uses IEC binary prefixes (1024). string must not be NULL and max_string_length must be greater than zero.
*/
extern NexusStringFormatResult nexus_strings_bytes_format(char *string, uint_large max_string_length, uint_large byte_count);

/*
nexus_strings_string_length gets the current length of a string.

string must not be NULL.
*/
extern uint_large nexus_strings_string_length(const char *string);

/*
Checks if a string exactly starts with the provided prefix.

string and prefix must not be NULL.
*/
extern boolean nexus_strings_string_starts_with(const char *string, const char *prefix);

/*
Performs a safe, bounded copy of a string. Guarantees null-termination.

Truncates when src does not fit and discards the outcome. Prefer
nexus_strings_string_copy_with_truncation when truncation must be detected.

dest, src must not be NULL and dest_max_len must be greater than zero.
*/
extern void nexus_strings_string_copy(char *dest, uint_large dest_max_len, const char *src);

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
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
extern int32 nexus_strings_string_compare(const char *str1, const char *str2);

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
extern int32 nexus_strings_string_compare_unsigned(const unsigned char *str1, const unsigned char *str2);

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
extern int32 nexus_strings_string_compare_mixed(const unsigned char *str1, const char *str2);

/*
Performs a lexicographical ASCII comparison. Returns <0 if str1 < str2, 0 if equal, >0 if str1 > str2.

str1 and str2 must not be NULL.
*/
extern int32 nexus_strings_string_compare_mixed_alt(const char *str1, const unsigned char *str2);

/*
Checks if two strings are equal.

Convenience wrapper around `nexus_strings_string_compare`
*/
extern boolean nexus_strings_string_equals(const char *str1, const char *str2);

/*
Checks if two unsigned strings are equal.

Convenience wrapper around `nexus_strings_string_compare_unsigned`
*/
extern boolean nexus_strings_string_equals_unsigned(const unsigned char *str1, const unsigned char *str2);

/*
Checks if two mixed strings are equal.

Convenience wrapper around `nexus_strings_string_compare_mixed`
*/
extern boolean nexus_strings_string_equals_mixed(const unsigned char *str1, const char *str2);

/*
Checks if two mixed strings are equal.

Convenience wrapper around `nexus_strings_string_compare_mixed_alt`
*/
extern boolean nexus_strings_string_equals_mixed_alt(const char *str1, const unsigned char *str2);

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

/* ---------------------------------------------------------------------------- */
/* ERRORS                                                                       */
/* ---------------------------------------------------------------------------- */

/*
NError is the universal 32-bit error format shared across all libraries.

The high 16 bits encode a two-character ASCII facility tag. The low 16 bits
encode a facility-specific error code. NEXUS_ERROR_NONE (0) is success.

Use nexus_errors_message_write to obtain a human-readable description of a
NError value. Nexus only formats messages for its own 'N' 'X' facility errors.
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

#define NEXUS_ERROR_FILE_NOT_FOUND    NEXUS_ERROR_MAKE('N', 'X', 1)
#define NEXUS_ERROR_PERMISSION_DENIED NEXUS_ERROR_MAKE('N', 'X', 2)
#define NEXUS_ERROR_ALREADY_EXISTS    NEXUS_ERROR_MAKE('N', 'X', 3)
#define NEXUS_ERROR_DIR_NOT_EMPTY     NEXUS_ERROR_MAKE('N', 'X', 4)
#define NEXUS_ERROR_DISK_FULL         NEXUS_ERROR_MAKE('N', 'X', 5)
#define NEXUS_ERROR_INVALID_ARGUMENT  NEXUS_ERROR_MAKE('N', 'X', 6)
#define NEXUS_ERROR_IO                NEXUS_ERROR_MAKE('N', 'X', 7)

/*
nexus_errors_message_write copies a human-readable description of error into buffer,
optionally prefixed with prefix.

When prefix is NULL or an empty string, the message is written without a prefix.
Writes an empty string when error is NEXUS_ERROR_NONE. buffer must not be NULL and
buffer_max_length must be greater than zero.
*/
extern uint_large nexus_errors_message_write(NError error, char *buffer, uint_large buffer_max_length, const char *prefix);

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
nexus_filesystem_file_write writes bytes to an opened file.

Writes the number of bytes written to out_bytes_written. A short write without a
stream error is not treated as failure.
*/
extern NError nexus_filesystem_file_write(NexusFileHandle *file_handle, byte *bytes, uint_large length, uint_large *out_bytes_written);

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

/* ---------------------------------------------------------------------------- */
/* ASSERTIONS                                                                   */
/* ---------------------------------------------------------------------------- */

/* TODO: refactor into a global runtime?? */

#ifndef NEXUS_ASSERTIONS_ENABLED
#  define NEXUS_ASSERTIONS_ENABLED 1
#endif

#ifndef NEXUS_DEBUG_ENABLED
#  define NEXUS_DEBUG_ENABLED 1
#endif

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
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
        __asm__ __volatile__("brk #0\n\tnop");                                                                                                          \
        abort();
#    elif defined(__arm__) || defined(__ARM_ARCH) || defined(_M_ARM)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
        __asm__ __volatile__("bkpt #0\n\tnop");                                                                                                        \
        abort();
#    elif defined(__riscv) || defined(__riscv__)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
        __asm__ __volatile__("ebreak\n\tnop");                                                                                                         \
        abort();
#    elif defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
        __asm__ __volatile__("int3\n\tnop");                                                                                                           \
        abort();
#    else
#      define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
        __builtin_trap();
#    endif
#  else /* Generic fallback */
#    include <signal.h>
#    define NEXUS_ASSERTIONS_DEBUG_TRAP()                                                                                                            \
      (void)raise(SIGTRAP);                                                                                                                          \
      abort();
#  endif /* NEXUS_ASSERTIONS_DEBUG_TRAP implementation selection */

/*
ErrorMessageReportCallback is the callback that gets invoked when an assertion fails.

This is usually set to either an "ERROR" or "CRITICAL" report in a logger, depending on client interpretation.
*/
typedef void ErrorMessageReportCallback(void *user_data, const char *message, const char *file, uint32 line);

/*
nexus_assertions_error_callback_set sets the callback used for reporting assertion failures.
*/
extern void nexus_assertions_error_callback_set(ErrorMessageReportCallback *callback, void *user_data);

/*
nexus_assertion_failure_report reports an assertion failure.
*/
extern void nexus_assertions_failure_report(const char *expression, const char *message, const char *file, uint32 line);

#  define NEXUS_ASSERT(expr)                                                                                                                         \
    do {                                                                                                                                             \
      if (expr) {                                                                                                                                    \
      } else {                                                                                                                                       \
        nexus_assertions_failure_report(#expr, "", __FILE__, __LINE__);                                                                              \
        NEXUS_ASSERTIONS_DEBUG_TRAP();                                                                                                               \
      }                                                                                                                                              \
    } while (0)

#  define NEXUS_ASSERT_MESSAGE(expr, message)                                                                                                        \
    do {                                                                                                                                             \
      if (expr) {                                                                                                                                    \
      } else {                                                                                                                                       \
        nexus_assertions_failure_report(#expr, message, __FILE__, __LINE__);                                                                         \
        NEXUS_ASSERTIONS_DEBUG_TRAP();                                                                                                               \
      }                                                                                                                                              \
    } while (0)

#  if NEXUS_DEBUG_ENABLED

#    define NEXUS_ASSERT_DEBUG(expr)                                                                                                                 \
      do {                                                                                                                                           \
        if (expr) {                                                                                                                                  \
        } else {                                                                                                                                     \
          nexus_assertions_failure_report(#expr, "", __FILE__, __LINE__);                                                                            \
          NEXUS_ASSERTIONS_DEBUG_TRAP();                                                                                                             \
        }                                                                                                                                            \
      } while (0)

#    define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)                                                                                                \
      do {                                                                                                                                           \
        if (expr) {                                                                                                                                  \
        } else {                                                                                                                                     \
          nexus_assertions_failure_report(#expr, message, __FILE__, __LINE__);                                                                       \
          NEXUS_ASSERTIONS_DEBUG_TRAP();                                                                                                             \
        }                                                                                                                                            \
      } while (0)

#  else

#    define NEXUS_ASSERT_DEBUG(expr)
#    define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)

#  endif /* NEXUS_DEBUG_ENABLED */

#else

#  define NEXUS_ASSERT(expr)
#  define NEXUS_ASSERT_MESSAGE(expr, message)
#  define NEXUS_ASSERT_DEBUG(expr)
#  define NEXUS_ASSERT_MESSAGE_DEBUG(expr, message)

#endif /* NEXUS_ASSERTIONS_ENABLED */

/* ---------------------------------------------------------------------------- */
/* BITS                                                                         */
/* ---------------------------------------------------------------------------- */

/*
Endian-safe decoders for packed binary data.

LSB functions treat bytes[0] as the least significant byte (little-endian wire order).
MSB functions treat bytes[0] as the most significant byte (big-endian wire order).

Integer signed variants reinterpret the assembled bit pattern as two's complement.
Floating-point variants assemble an IEEE-754 bit pattern, then reinterpret it.

bytes must not be NULL. Each function reads exactly sizeof(return type) bytes.
*/

/*
nexus_bits_uint16_from_bytes_lsb decodes a 16-bit unsigned integer from 2 bytes, little-endian.
*/
extern uint16 nexus_bits_uint16_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_uint32_from_bytes_lsb decodes a 32-bit unsigned integer from 4 bytes, little-endian.
*/
extern uint32 nexus_bits_uint32_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_uint64_from_bytes_lsb decodes a 64-bit unsigned integer from 8 bytes, little-endian.
*/
extern uint64 nexus_bits_uint64_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_int16_from_bytes_lsb decodes a 16-bit signed integer from 2 bytes, little-endian.
*/
extern int16 nexus_bits_int16_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_int32_from_bytes_lsb decodes a 32-bit signed integer from 4 bytes, little-endian.
*/
extern int32 nexus_bits_int32_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_int64_from_bytes_lsb decodes a 64-bit signed integer from 8 bytes, little-endian.
*/
extern int64 nexus_bits_int64_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_real32_from_bytes_lsb decodes a 32-bit IEEE-754 float from 4 bytes, little-endian.
*/
extern real32 nexus_bits_real32_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_real64_from_bytes_lsb decodes a 64-bit IEEE-754 double from 8 bytes, little-endian.
*/
extern real64 nexus_bits_real64_from_bytes_lsb(const byte *bytes);

/*
nexus_bits_uint16_from_bytes_msb decodes a 16-bit unsigned integer from 2 bytes, big-endian.
*/
extern uint16 nexus_bits_uint16_from_bytes_msb(const byte *bytes);

/*
nexus_bits_uint32_from_bytes_msb decodes a 32-bit unsigned integer from 4 bytes, big-endian.
*/
extern uint32 nexus_bits_uint32_from_bytes_msb(const byte *bytes);

/*
nexus_bits_uint64_from_bytes_msb decodes a 64-bit unsigned integer from 8 bytes, big-endian.
*/
extern uint64 nexus_bits_uint64_from_bytes_msb(const byte *bytes);

/*
nexus_bits_int16_from_bytes_msb decodes a 16-bit signed integer from 2 bytes, big-endian.
*/
extern int16 nexus_bits_int16_from_bytes_msb(const byte *bytes);

/*
nexus_bits_int32_from_bytes_msb decodes a 32-bit signed integer from 4 bytes, big-endian.
*/
extern int32 nexus_bits_int32_from_bytes_msb(const byte *bytes);

/*
nexus_bits_int64_from_bytes_msb decodes a 64-bit signed integer from 8 bytes, big-endian.
*/
extern int64 nexus_bits_int64_from_bytes_msb(const byte *bytes);

/*
nexus_bits_real32_from_bytes_msb decodes a 32-bit IEEE-754 float from 4 bytes, big-endian.
*/
extern real32 nexus_bits_real32_from_bytes_msb(const byte *bytes);

/*
nexus_bits_real64_from_bytes_msb decodes a 64-bit IEEE-754 double from 8 bytes, big-endian.
*/
extern real64 nexus_bits_real64_from_bytes_msb(const byte *bytes);

/* ---------------------------------------------------------------------------- */
/* HASHING                                                                      */
/* ---------------------------------------------------------------------------- */

typedef struct NexusHash NexusHash; /* Note, for now this is a stub, later it will be a hash. */
