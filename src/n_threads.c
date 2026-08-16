#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <process.h>
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
#  include <errno.h>
#  include <time.h>
#  include <pthread.h>
#  if defined(__APPLE__)
#    include <dispatch/dispatch.h>
#  else
#    include <semaphore.h>
#  endif
#endif

#if NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32
#  if defined(_MSC_VER)
#    include <intrin.h>
#  elif defined(__GNUC__) || defined(__clang__)
#    include <immintrin.h>
#  endif
#endif

/* ---------------------------------------------------------------------------- */
/* PLATFORM INTERNAL STRUCTURE DEFINITIONS                                      */
/* ---------------------------------------------------------------------------- */

struct NexusThread {
  NexusThreadFunc entry_func;
  void           *user_data;
#if defined(NEXUS_PLATFORM_WINDOWS)
  HANDLE       handle;
  unsigned int thread_id;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_t handle;
#endif
};

struct NexusMutex {
#if defined(NEXUS_PLATFORM_WINDOWS)
  CRITICAL_SECTION cs;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_mutex_t handle;
#endif
};

struct NexusCond {
#if defined(NEXUS_PLATFORM_WINDOWS)
  CONDITION_VARIABLE cv;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_cond_t handle;
#endif
};

struct NexusSemaphore {
#if defined(NEXUS_PLATFORM_WINDOWS)
  HANDLE handle;
#elif defined(__APPLE__)
  dispatch_semaphore_t sem;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  sem_t handle;
#endif
};

struct NexusThreadsWaitGroup {
  int32 counter;
#if defined(NEXUS_PLATFORM_WINDOWS)
  CRITICAL_SECTION   cs;
  CONDITION_VARIABLE cv;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
#endif
};

/* ---------------------------------------------------------------------------- */
/* MEMORY DEBUGGER THREAD-SAFETY BOOTSTRAP                                      */
/* ---------------------------------------------------------------------------- */

/*
 * Nexus' debug allocator is process-global. Before the first secondary thread is
 * created, install an allocation-free, process-lifetime native mutex into the
 * memory debugger. User callbacks run outside this lock, so the allocator does
 * not impose lock ordering on Echo or other subsystems.
 *
 * This lock MUST NOT be a heap-backed NexusMutex:
 *   - NexusMutex itself is allocated through malloc.
 *   - malloc may be redirected to the memory debugger.
 *   - destroying/freeing the allocator's own mutex would require taking that
 *     same mutex and would make shutdown ordering fragile.
 *
 * The native mutex intentionally remains initialized until process termination.
 * There is no deinitialization path.
 */
#if NEXUS_MEMORY_DEBUG_ENABLED

#  if defined(NEXUS_PLATFORM_WINDOWS)

static CRITICAL_SECTION n_threads_memory_debug_mutex;
static INIT_ONCE        n_threads_memory_debug_once = INIT_ONCE_STATIC_INIT;

static int n_threads_memory_debug_lock(void *mutex) {
  EnterCriticalSection((CRITICAL_SECTION *)mutex);
  return 0;
}

static int n_threads_memory_debug_unlock(void *mutex) {
  LeaveCriticalSection((CRITICAL_SECTION *)mutex);
  return 0;
}

static BOOL CALLBACK n_threads_memory_debug_initialize_once(PINIT_ONCE once, PVOID parameter, PVOID *context) {
  (void)once;
  (void)parameter;
  (void)context;

  InitializeCriticalSection(&n_threads_memory_debug_mutex);

  nexus_debug_mem_thread_safe_init(n_threads_memory_debug_lock, n_threads_memory_debug_unlock, &n_threads_memory_debug_mutex);

  return TRUE;
}

static NError n_threads_memory_debug_thread_safety_ensure(void) {
  if (!InitOnceExecuteOnce(&n_threads_memory_debug_once, n_threads_memory_debug_initialize_once, NULL, NULL)) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
}

#  elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)

static pthread_mutex_t n_threads_memory_debug_mutex;
static pthread_once_t  n_threads_memory_debug_once = PTHREAD_ONCE_INIT;
static int             n_threads_memory_debug_initialize_error;

static int n_threads_memory_debug_lock(void *mutex) {
  return pthread_mutex_lock((pthread_mutex_t *)mutex);
}

static int n_threads_memory_debug_unlock(void *mutex) {
  return pthread_mutex_unlock((pthread_mutex_t *)mutex);
}

static void n_threads_memory_debug_initialize_once(void) {
  int result;

  n_threads_memory_debug_initialize_error = 0;

  result = pthread_mutex_init(&n_threads_memory_debug_mutex, NULL);
  if (result != 0) {
    n_threads_memory_debug_initialize_error = result;
    return;
  }

  nexus_debug_mem_thread_safe_init(n_threads_memory_debug_lock, n_threads_memory_debug_unlock, &n_threads_memory_debug_mutex);
}

static NError n_threads_memory_debug_thread_safety_ensure(void) {
  if (pthread_once(&n_threads_memory_debug_once, n_threads_memory_debug_initialize_once) != 0) {
    return NEXUS_ERROR_IO;
  }

  if (n_threads_memory_debug_initialize_error != 0) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
}

#  else

static NError n_threads_memory_debug_thread_safety_ensure(void) {
  return NEXUS_ERROR_IO;
}

#  endif

#else

static NError n_threads_memory_debug_thread_safety_ensure(void) {
  return NEXUS_ERROR_NONE;
}

#endif

/* ---------------------------------------------------------------------------- */
/* THREAD ENTRY TRAMPOLINE                                                      */
/* ---------------------------------------------------------------------------- */

#if defined(NEXUS_PLATFORM_WINDOWS)
static unsigned __stdcall nexus_thread_entry_win32(void *arg) {
  NexusThread *thread;
  thread = (NexusThread *)arg;
  NEXUS_ASSERT_MESSAGE(thread != NULL, "Thread context passed to entry trampoline is NULL");
  if (thread != NULL && thread->entry_func != NULL) {
    thread->entry_func(thread->user_data);
  }
  return 0;
}
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
static void *nexus_thread_entry_posix(void *arg) {
  NexusThread *thread;
  thread = (NexusThread *)arg;
  NEXUS_ASSERT_MESSAGE(thread != NULL, "Thread context passed to entry trampoline is NULL");
  if (thread != NULL && thread->entry_func != NULL) {
    thread->entry_func(thread->user_data);
  }
  return NULL;
}
#endif

/* ---------------------------------------------------------------------------- */
/* THREAD LIFECYCLE                                                             */
/* ---------------------------------------------------------------------------- */

NError nexus_threads_create(NexusThread **out_thread, NexusThreadFunc entry_func, void *user_data) {
  NexusThread *thread;
  NError       error;

  NEXUS_ASSERT_MESSAGE(out_thread != NULL, "Destination thread pointer cannot be NULL");
  NEXUS_ASSERT_MESSAGE(entry_func != NULL, "Thread entry function cannot be NULL");

  if (out_thread == NULL || entry_func == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_thread = NULL;

  /*
   * This must happen before allocating the NexusThread and, critically, before
   * the native thread is started. Once this succeeds, every tracked allocation
   * made by any Nexus-created thread is serialized by the memory debugger.
   */
  error = n_threads_memory_debug_thread_safety_ensure();
  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  thread = (NexusThread *)malloc(NEXUS_SIZEOF(NexusThread));
  if (thread == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

  thread->entry_func = entry_func;
  thread->user_data  = user_data;

#if defined(NEXUS_PLATFORM_WINDOWS)
  thread->handle = (HANDLE)_beginthreadex(NULL, 0, nexus_thread_entry_win32, thread, 0, &thread->thread_id);
  if (thread->handle == NULL || thread->handle == (HANDLE)-1L) {
    free(thread);
    return NEXUS_ERROR_IO;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_create(&thread->handle, NULL, nexus_thread_entry_posix, thread) != 0) {
    free(thread);
    return NEXUS_ERROR_IO;
  }
#endif

  *out_thread = thread;
  return NEXUS_ERROR_NONE;
}

NError nexus_threads_join(NexusThread *thread) {
  NEXUS_ASSERT_MESSAGE(thread != NULL, "Attempted to join NULL thread");

  if (thread == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (thread->handle != NULL) {
    if (WaitForSingleObject(thread->handle, INFINITE) != WAIT_OBJECT_0) {
      return NEXUS_ERROR_IO;
    }
    CloseHandle(thread->handle);
    thread->handle = NULL;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_join(thread->handle, NULL) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  free(thread);
  return NEXUS_ERROR_NONE;
}

/* ---------------------------------------------------------------------------- */
/* TIMING & SLEEP                                                               */
/* ---------------------------------------------------------------------------- */

void nexus_threads_sleep(NexusDuration duration) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  HANDLE        timer;
  LARGE_INTEGER due_time;
  DWORD         milliseconds;
  uint64        nanoseconds;
  uint64        timer_ticks;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  struct timespec requested;
  struct timespec remaining;
  int             result;
#endif

  NEXUS_ASSERT_MESSAGE(duration.nanoseconds >= 0, "Sleep duration cannot be negative");

  if (duration.nanoseconds <= 0) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  nanoseconds = (uint64)duration.nanoseconds;

  if (nanoseconds >= NEXUS_NANOSECONDS_PER_MILLISECOND) {
    milliseconds = (DWORD)((nanoseconds + NEXUS_NANOSECONDS_PER_MILLISECOND - 1ULL) / NEXUS_NANOSECONDS_PER_MILLISECOND);
    Sleep(milliseconds);
    return;
  }

  timer_ticks = (nanoseconds + 99ULL) / 100ULL;
  timer       = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);

  if (timer == NULL) {
    Sleep(1);
    return;
  }

  due_time.QuadPart = -(LONGLONG)timer_ticks;

  if (!SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE)) {
    CloseHandle(timer);
    Sleep(1);
    return;
  }

  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  requested.tv_sec  = (time_t)((uint64)duration.nanoseconds / NEXUS_NANOSECONDS_PER_SECOND);
  requested.tv_nsec = (long)((uint64)duration.nanoseconds % NEXUS_NANOSECONDS_PER_SECOND);

  for (;;) {
    result = nanosleep(&requested, &remaining);
    if (result == 0) {
      return;
    }
    if (errno != EINTR) {
      return;
    }
    requested = remaining;
  }
#else
#  error "nexus_threads_sleep is not implemented for this platform"
#endif
}

static void nexus_threads_spin_hint(void) {
#if NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32
  _mm_pause();
#elif NEXUS_ARCH == NEXUS_ARCH_ARM64 || NEXUS_ARCH == NEXUS_ARCH_ARM7 || NEXUS_ARCH == NEXUS_ARCH_ARM7A || NEXUS_ARCH == NEXUS_ARCH_ARM7R ||         \
    NEXUS_ARCH == NEXUS_ARCH_ARM7M || NEXUS_ARCH == NEXUS_ARCH_ARM7S
#  if defined(_MSC_VER)
  __yield();
#  elif defined(__GNUC__) || defined(__clang__)
  __asm__ __volatile__("yield");
#  endif
#endif
}

void nexus_threads_spin_wait(NexusDuration duration) {
  NexusTime target;

  NEXUS_ASSERT_MESSAGE(duration.nanoseconds >= 0, "Spin duration cannot be negative");

  if (duration.nanoseconds <= 0) {
    return;
  }

  target = nexus_time_add_duration(nexus_time_get_monotonic(), duration);

  while (nexus_time_get_monotonic().time < target.time) {
    nexus_threads_spin_hint();
  }
}

/* ---------------------------------------------------------------------------- */
/* ATOMICS                                                                      */
/* ---------------------------------------------------------------------------- */

/*
Atomic operations deliberately use sequentially consistent ordering.

This gives the API Go-like semantics and avoids exposing memory ordering concerns
through the general-purpose Nexus threading API.
*/

#if defined(NEXUS_PLATFORM_WINDOWS)

typedef char n_threads_atomic_long_size_check[(NEXUS_SIZEOF(LONG) == NEXUS_SIZEOF(uint32)) ? 1 : -1];
typedef char n_threads_atomic_long_long_size_check[(NEXUS_SIZEOF(LONGLONG) == NEXUS_SIZEOF(uint64)) ? 1 : -1];

static LONG n_threads_atomic_uint32_to_windows_long(uint32 value) {
  LONG result;

  nexus_memory_bytes_copy(&result, &value, NEXUS_SIZEOF(result));

  return result;
}

static uint32 n_threads_atomic_uint32_from_windows_long(LONG value) {
  uint32 result;

  nexus_memory_bytes_copy(&result, &value, NEXUS_SIZEOF(result));

  return result;
}

static LONGLONG n_threads_atomic_uint64_to_windows_long_long(uint64 value) {
  LONGLONG result;

  nexus_memory_bytes_copy(&result, &value, NEXUS_SIZEOF(result));

  return result;
}

static uint64 n_threads_atomic_uint64_from_windows_long_long(LONGLONG value) {
  uint64 result;

  nexus_memory_bytes_copy(&result, &value, NEXUS_SIZEOF(result));

  return result;
}

#endif

static uint32 n_threads_atomic_uint32_load_raw(volatile uint32 *value) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  return n_threads_atomic_uint32_from_windows_long(InterlockedCompareExchange((volatile LONG *)value, 0, 0));

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_load_n(value, __ATOMIC_SEQ_CST);

#else
#  error "32-bit atomic operations are not implemented for this compiler"
#endif
}

static void n_threads_atomic_uint32_store_raw(volatile uint32 *value, uint32 new_value) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  InterlockedExchange((volatile LONG *)value, n_threads_atomic_uint32_to_windows_long(new_value));

#elif defined(__GNUC__) || defined(__clang__)
  __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST);

#else
#  error "32-bit atomic operations are not implemented for this compiler"
#endif
}

static uint32 n_threads_atomic_uint32_swap_raw(volatile uint32 *value, uint32 new_value) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  return n_threads_atomic_uint32_from_windows_long(InterlockedExchange((volatile LONG *)value, n_threads_atomic_uint32_to_windows_long(new_value)));

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST);

#else
#  error "32-bit atomic operations are not implemented for this compiler"
#endif
}

static boolean n_threads_atomic_uint32_compare_exchange_raw(volatile uint32 *value, uint32 old_value, /* NOLINT(readability-non-const-parameter) */
                                                            uint32 new_value) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  LONG result;

  result = InterlockedCompareExchange((volatile LONG *)value, n_threads_atomic_uint32_to_windows_long(new_value),
                                      n_threads_atomic_uint32_to_windows_long(old_value));

  return n_threads_atomic_uint32_from_windows_long(result) == old_value ? TRUE : FALSE;

#elif defined(__GNUC__) || defined(__clang__)
  uint32 expected;

  expected = old_value;

  return __atomic_compare_exchange_n(value, &expected, new_value, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE;

#else
#  error "32-bit atomic operations are not implemented for this compiler"
#endif
}

static uint32 n_threads_atomic_uint32_add_raw(volatile uint32 *value, uint32 delta) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  uint32 old_value;

  old_value =
      n_threads_atomic_uint32_from_windows_long(InterlockedExchangeAdd((volatile LONG *)value, n_threads_atomic_uint32_to_windows_long(delta)));

  return old_value + delta;

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_add_fetch(value, delta, __ATOMIC_SEQ_CST);

#else
#  error "32-bit atomic operations are not implemented for this compiler"
#endif
}

static uint64 n_threads_atomic_uint64_load_raw(volatile uint64 *value) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  return n_threads_atomic_uint64_from_windows_long_long(InterlockedCompareExchange64((volatile LONGLONG *)value, 0, 0));

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_load_n(value, __ATOMIC_SEQ_CST);

#else
#  error "64-bit atomic operations are not implemented for this compiler"
#endif
}

static void n_threads_atomic_uint64_store_raw(volatile uint64 *value, uint64 new_value) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  InterlockedExchange64((volatile LONGLONG *)value, n_threads_atomic_uint64_to_windows_long_long(new_value));

#elif defined(__GNUC__) || defined(__clang__)
  __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST);

#else
#  error "64-bit atomic operations are not implemented for this compiler"
#endif
}

static uint64 n_threads_atomic_uint64_swap_raw(volatile uint64 *value, uint64 new_value) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  return n_threads_atomic_uint64_from_windows_long_long(
      InterlockedExchange64((volatile LONGLONG *)value, n_threads_atomic_uint64_to_windows_long_long(new_value)));

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST);

#else
#  error "64-bit atomic operations are not implemented for this compiler"
#endif
}

static boolean n_threads_atomic_uint64_compare_exchange_raw(volatile uint64 *value, uint64 old_value, /* NOLINT(readability-non-const-parameter) */
                                                            uint64 new_value) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  LONGLONG result;

  result = InterlockedCompareExchange64((volatile LONGLONG *)value, n_threads_atomic_uint64_to_windows_long_long(new_value),
                                        n_threads_atomic_uint64_to_windows_long_long(old_value));

  return n_threads_atomic_uint64_from_windows_long_long(result) == old_value ? TRUE : FALSE;

#elif defined(__GNUC__) || defined(__clang__)
  uint64 expected;

  expected = old_value;

  return __atomic_compare_exchange_n(value, &expected, new_value, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE;

#else
#  error "64-bit atomic operations are not implemented for this compiler"
#endif
}

static uint64 n_threads_atomic_uint64_add_raw(volatile uint64 *value, uint64 delta) { /* NOLINT(readability-non-const-parameter) */
#if defined(NEXUS_PLATFORM_WINDOWS)
  uint64 old_value;

  old_value = n_threads_atomic_uint64_from_windows_long_long(
      InterlockedExchangeAdd64((volatile LONGLONG *)value, n_threads_atomic_uint64_to_windows_long_long(delta)));

  return old_value + delta;

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_add_fetch(value, delta, __ATOMIC_SEQ_CST);

#else
#  error "64-bit atomic operations are not implemented for this compiler"
#endif
}

static uint32 n_threads_atomic_int32_to_bits(int32 value) {
  uint32 bits;

  nexus_memory_bytes_copy(&bits, &value, NEXUS_SIZEOF(bits));

  return bits;
}

static int32 n_threads_atomic_int32_from_bits(uint32 bits) {
  int32 value;

  nexus_memory_bytes_copy(&value, &bits, NEXUS_SIZEOF(value));

  return value;
}

static uint64 n_threads_atomic_int64_to_bits(int64 value) {
  uint64 bits;

  nexus_memory_bytes_copy(&bits, &value, NEXUS_SIZEOF(bits));

  return bits;
}

static int64 n_threads_atomic_int64_from_bits(uint64 bits) {
  int64 value;

  nexus_memory_bytes_copy(&value, &bits, NEXUS_SIZEOF(value));

  return value;
}

/* ---------------------------------------------------------------------------- */
/* INT32 ATOMIC                                                                 */
/* ---------------------------------------------------------------------------- */

int32 nexus_threads_atomic_int32_load(NexusAtomicInt32 *atomic) {
  uint32 bits;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int32 load on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  bits = n_threads_atomic_uint32_load_raw((volatile uint32 *)&atomic->storage);

  return n_threads_atomic_int32_from_bits(bits);
}

void nexus_threads_atomic_int32_store(NexusAtomicInt32 *atomic, int32 value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int32 store on NULL atomic");

  if (atomic == NULL) {
    return;
  }

  n_threads_atomic_uint32_store_raw((volatile uint32 *)&atomic->storage, n_threads_atomic_int32_to_bits(value));
}

int32 nexus_threads_atomic_int32_swap(NexusAtomicInt32 *atomic, int32 value) {
  uint32 bits;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int32 swap on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  bits = n_threads_atomic_uint32_swap_raw((volatile uint32 *)&atomic->storage, n_threads_atomic_int32_to_bits(value));

  return n_threads_atomic_int32_from_bits(bits);
}

boolean nexus_threads_atomic_int32_compare_exchange(NexusAtomicInt32 *atomic, int32 old_value, int32 new_value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int32 compare_exchange on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  return n_threads_atomic_uint32_compare_exchange_raw((volatile uint32 *)&atomic->storage, n_threads_atomic_int32_to_bits(old_value),
                                                      n_threads_atomic_int32_to_bits(new_value));
}

int32 nexus_threads_atomic_int32_add(NexusAtomicInt32 *atomic, int32 delta) {
  uint32 bits;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int32 add on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  bits = n_threads_atomic_uint32_add_raw((volatile uint32 *)&atomic->storage, n_threads_atomic_int32_to_bits(delta));

  return n_threads_atomic_int32_from_bits(bits);
}

/* ---------------------------------------------------------------------------- */
/* UINT32 ATOMIC                                                                */
/* ---------------------------------------------------------------------------- */

uint32 nexus_threads_atomic_uint32_load(NexusAtomicUint32 *atomic) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint32 load on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  return n_threads_atomic_uint32_load_raw(&atomic->storage);
}

void nexus_threads_atomic_uint32_store(NexusAtomicUint32 *atomic, uint32 value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint32 store on NULL atomic");

  if (atomic == NULL) {
    return;
  }

  n_threads_atomic_uint32_store_raw(&atomic->storage, value);
}

uint32 nexus_threads_atomic_uint32_swap(NexusAtomicUint32 *atomic, uint32 value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint32 swap on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  return n_threads_atomic_uint32_swap_raw(&atomic->storage, value);
}

boolean nexus_threads_atomic_uint32_compare_exchange(NexusAtomicUint32 *atomic, uint32 old_value, uint32 new_value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint32 compare_exchange on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  return n_threads_atomic_uint32_compare_exchange_raw(&atomic->storage, old_value, new_value);
}

uint32 nexus_threads_atomic_uint32_add(NexusAtomicUint32 *atomic, uint32 delta) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint32 add on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  return n_threads_atomic_uint32_add_raw(&atomic->storage, delta);
}

/* ---------------------------------------------------------------------------- */
/* INT64 ATOMIC                                                                 */
/* ---------------------------------------------------------------------------- */

int64 nexus_threads_atomic_int64_load(NexusAtomicInt64 *atomic) {
  uint64 bits;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int64 load on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  bits = n_threads_atomic_uint64_load_raw((volatile uint64 *)&atomic->storage);

  return n_threads_atomic_int64_from_bits(bits);
}

void nexus_threads_atomic_int64_store(NexusAtomicInt64 *atomic, int64 value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int64 store on NULL atomic");

  if (atomic == NULL) {
    return;
  }

  n_threads_atomic_uint64_store_raw((volatile uint64 *)&atomic->storage, n_threads_atomic_int64_to_bits(value));
}

int64 nexus_threads_atomic_int64_swap(NexusAtomicInt64 *atomic, int64 value) {
  uint64 bits;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int64 swap on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  bits = n_threads_atomic_uint64_swap_raw((volatile uint64 *)&atomic->storage, n_threads_atomic_int64_to_bits(value));

  return n_threads_atomic_int64_from_bits(bits);
}

boolean nexus_threads_atomic_int64_compare_exchange(NexusAtomicInt64 *atomic, int64 old_value, int64 new_value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int64 compare_exchange on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  return n_threads_atomic_uint64_compare_exchange_raw((volatile uint64 *)&atomic->storage, n_threads_atomic_int64_to_bits(old_value),
                                                      n_threads_atomic_int64_to_bits(new_value));
}

int64 nexus_threads_atomic_int64_add(NexusAtomicInt64 *atomic, int64 delta) {
  uint64 bits;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic int64 add on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  bits = n_threads_atomic_uint64_add_raw((volatile uint64 *)&atomic->storage, n_threads_atomic_int64_to_bits(delta));

  return n_threads_atomic_int64_from_bits(bits);
}

/* ---------------------------------------------------------------------------- */
/* UINT64 ATOMIC                                                                */
/* ---------------------------------------------------------------------------- */

uint64 nexus_threads_atomic_uint64_load(NexusAtomicUint64 *atomic) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint64 load on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  return n_threads_atomic_uint64_load_raw(&atomic->storage);
}

void nexus_threads_atomic_uint64_store(NexusAtomicUint64 *atomic, uint64 value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint64 store on NULL atomic");

  if (atomic == NULL) {
    return;
  }

  n_threads_atomic_uint64_store_raw(&atomic->storage, value);
}

uint64 nexus_threads_atomic_uint64_swap(NexusAtomicUint64 *atomic, uint64 value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint64 swap on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  return n_threads_atomic_uint64_swap_raw(&atomic->storage, value);
}

boolean nexus_threads_atomic_uint64_compare_exchange(NexusAtomicUint64 *atomic, uint64 old_value, uint64 new_value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint64 compare_exchange on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  return n_threads_atomic_uint64_compare_exchange_raw(&atomic->storage, old_value, new_value);
}

uint64 nexus_threads_atomic_uint64_add(NexusAtomicUint64 *atomic, uint64 delta) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic uint64 add on NULL atomic");

  if (atomic == NULL) {
    return 0;
  }

  return n_threads_atomic_uint64_add_raw(&atomic->storage, delta);
}

/* ---------------------------------------------------------------------------- */
/* ARCHITECTURE-WIDTH ATOMICS                                                   */
/* ---------------------------------------------------------------------------- */

int_large nexus_threads_atomic_int_large_load(NexusAtomicIntLarge *atomic) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_int64_load(atomic);
#else
  return nexus_threads_atomic_int32_load(atomic);
#endif
}

void nexus_threads_atomic_int_large_store(NexusAtomicIntLarge *atomic, int_large value) {
#if NEXUS_ARCHITECTURE_BITS == 64
  nexus_threads_atomic_int64_store(atomic, value);
#else
  nexus_threads_atomic_int32_store(atomic, value);
#endif
}

int_large nexus_threads_atomic_int_large_swap(NexusAtomicIntLarge *atomic, int_large value) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_int64_swap(atomic, value);
#else
  return nexus_threads_atomic_int32_swap(atomic, value);
#endif
}

boolean nexus_threads_atomic_int_large_compare_exchange(NexusAtomicIntLarge *atomic, int_large old_value, int_large new_value) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_int64_compare_exchange(atomic, old_value, new_value);
#else
  return nexus_threads_atomic_int32_compare_exchange(atomic, old_value, new_value);
#endif
}

int_large nexus_threads_atomic_int_large_add(NexusAtomicIntLarge *atomic, int_large delta) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_int64_add(atomic, delta);
#else
  return nexus_threads_atomic_int32_add(atomic, delta);
#endif
}

uint_large nexus_threads_atomic_uint_large_load(NexusAtomicUintLarge *atomic) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_uint64_load(atomic);
#else
  return nexus_threads_atomic_uint32_load(atomic);
#endif
}

void nexus_threads_atomic_uint_large_store(NexusAtomicUintLarge *atomic, uint_large value) {
#if NEXUS_ARCHITECTURE_BITS == 64
  nexus_threads_atomic_uint64_store(atomic, value);
#else
  nexus_threads_atomic_uint32_store(atomic, value);
#endif
}

uint_large nexus_threads_atomic_uint_large_swap(NexusAtomicUintLarge *atomic, uint_large value) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_uint64_swap(atomic, value);
#else
  return nexus_threads_atomic_uint32_swap(atomic, value);
#endif
}

boolean nexus_threads_atomic_uint_large_compare_exchange(NexusAtomicUintLarge *atomic, uint_large old_value, uint_large new_value) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_uint64_compare_exchange(atomic, old_value, new_value);
#else
  return nexus_threads_atomic_uint32_compare_exchange(atomic, old_value, new_value);
#endif
}

uint_large nexus_threads_atomic_uint_large_add(NexusAtomicUintLarge *atomic, uint_large delta) {
#if NEXUS_ARCHITECTURE_BITS == 64
  return nexus_threads_atomic_uint64_add(atomic, delta);
#else
  return nexus_threads_atomic_uint32_add(atomic, delta);
#endif
}

/* ---------------------------------------------------------------------------- */
/* BOOLEAN ATOMIC                                                               */
/* ---------------------------------------------------------------------------- */

boolean nexus_threads_atomic_boolean_load(NexusAtomicBoolean *atomic) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic boolean load on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  return n_threads_atomic_uint32_load_raw(&atomic->storage) != 0 ? TRUE : FALSE;
}

void nexus_threads_atomic_boolean_store(NexusAtomicBoolean *atomic, boolean value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic boolean store on NULL atomic");

  if (atomic == NULL) {
    return;
  }

  n_threads_atomic_uint32_store_raw(&atomic->storage, value ? 1U : 0U);
}

boolean nexus_threads_atomic_boolean_swap(NexusAtomicBoolean *atomic, boolean value) {
  uint32 old_value;

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic boolean swap on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  old_value = n_threads_atomic_uint32_swap_raw(&atomic->storage, value ? 1U : 0U);

  return old_value != 0 ? TRUE : FALSE;
}

boolean nexus_threads_atomic_boolean_compare_exchange(NexusAtomicBoolean *atomic, boolean old_value, boolean new_value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic boolean compare_exchange on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

  return n_threads_atomic_uint32_compare_exchange_raw(&atomic->storage, old_value ? 1U : 0U, new_value ? 1U : 0U);
}

/* ---------------------------------------------------------------------------- */
/* POINTER ATOMIC                                                               */
/* ---------------------------------------------------------------------------- */

void *nexus_threads_atomic_pointer_load(NexusAtomicPointer *atomic) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic pointer load on NULL atomic");

  if (atomic == NULL) {
    return NULL;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  return InterlockedCompareExchangePointer((PVOID volatile *)&atomic->storage, NULL, NULL);

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_load_n(&atomic->storage, __ATOMIC_SEQ_CST);

#else
#  error "Pointer atomic operations are not implemented for this compiler"
#endif
}

void nexus_threads_atomic_pointer_store(NexusAtomicPointer *atomic, void *value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic pointer store on NULL atomic");

  if (atomic == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  InterlockedExchangePointer((PVOID volatile *)&atomic->storage, value);

#elif defined(__GNUC__) || defined(__clang__)
  __atomic_store_n(&atomic->storage, value, __ATOMIC_SEQ_CST);

#else
#  error "Pointer atomic operations are not implemented for this compiler"
#endif
}

void *nexus_threads_atomic_pointer_swap(NexusAtomicPointer *atomic, void *value) {
  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic pointer swap on NULL atomic");

  if (atomic == NULL) {
    return NULL;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  return InterlockedExchangePointer((PVOID volatile *)&atomic->storage, value);

#elif defined(__GNUC__) || defined(__clang__)
  return __atomic_exchange_n(&atomic->storage, value, __ATOMIC_SEQ_CST);

#else
#  error "Pointer atomic operations are not implemented for this compiler"
#endif
}

boolean nexus_threads_atomic_pointer_compare_exchange(NexusAtomicPointer *atomic, void *old_value, void *new_value) {
#if defined(__GNUC__) || defined(__clang__)
  void *expected;
#endif

  NEXUS_ASSERT_MESSAGE(atomic != NULL, "Attempted atomic pointer compare_exchange on NULL atomic");

  if (atomic == NULL) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  return InterlockedCompareExchangePointer((PVOID volatile *)&atomic->storage, new_value, old_value) == old_value ? TRUE : FALSE;

#elif defined(__GNUC__) || defined(__clang__)
  expected = old_value;

  return __atomic_compare_exchange_n(&atomic->storage, &expected, new_value, FALSE, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? TRUE : FALSE;

#else
#  error "Pointer atomic operations are not implemented for this compiler"
#endif
}

/* ---------------------------------------------------------------------------- */
/* MUTEX                                                                        */
/* ---------------------------------------------------------------------------- */

NError nexus_threads_mutex_create(NexusMutex **out_mutex) {
  NexusMutex *mutex;

  NEXUS_ASSERT_MESSAGE(out_mutex != NULL, "Destination mutex pointer cannot be NULL");

  if (out_mutex == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_mutex = NULL;

  mutex = (NexusMutex *)malloc(NEXUS_SIZEOF(NexusMutex));
  if (mutex == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  InitializeCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  {
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
      free(mutex);
      return NEXUS_ERROR_IO;
    }
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&mutex->handle, &attr) != 0) {
      pthread_mutexattr_destroy(&attr);
      free(mutex);
      return NEXUS_ERROR_IO;
    }
    pthread_mutexattr_destroy(&attr);
  }
#endif

  *out_mutex = mutex;
  return NEXUS_ERROR_NONE;
}

NError nexus_threads_mutex_lock(NexusMutex *mutex) {
  NEXUS_ASSERT_MESSAGE(mutex != NULL, "Attempted to lock NULL mutex");

  if (mutex == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  EnterCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_mutex_lock(&mutex->handle) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

boolean nexus_threads_mutex_try_lock(NexusMutex *mutex) {
  NEXUS_ASSERT_MESSAGE(mutex != NULL, "Attempted to try_lock NULL mutex");

  if (mutex == NULL) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  return TryEnterCriticalSection(&mutex->cs) != 0 ? TRUE : FALSE;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  return pthread_mutex_trylock(&mutex->handle) == 0 ? TRUE : FALSE;
#endif
}

NError nexus_threads_mutex_unlock(NexusMutex *mutex) {
  NEXUS_ASSERT_MESSAGE(mutex != NULL, "Attempted to unlock NULL mutex");

  if (mutex == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  LeaveCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_mutex_unlock(&mutex->handle) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

void nexus_threads_mutex_destroy(NexusMutex *mutex) {
  NEXUS_ASSERT_MESSAGE(mutex != NULL, "Attempted to destroy NULL mutex");

  if (mutex == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  DeleteCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_mutex_destroy(&mutex->handle);
#endif

  free(mutex);
}

/* ---------------------------------------------------------------------------- */
/* CONDITION VARIABLE                                                           */
/* ---------------------------------------------------------------------------- */

NError nexus_threads_cond_create(NexusCond **out_cond) {
  NexusCond *cond;

  NEXUS_ASSERT_MESSAGE(out_cond != NULL, "Destination condition variable pointer cannot be NULL");

  if (out_cond == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_cond = NULL;

  cond = (NexusCond *)malloc(NEXUS_SIZEOF(NexusCond));
  if (cond == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  InitializeConditionVariable(&cond->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_cond_init(&cond->handle, NULL) != 0) {
    free(cond);
    return NEXUS_ERROR_IO;
  }
#endif

  *out_cond = cond;
  return NEXUS_ERROR_NONE;
}

NError nexus_threads_cond_wait(NexusCond *cond, NexusMutex *mutex) {
  NEXUS_ASSERT_MESSAGE(cond != NULL, "Attempted cond_wait on NULL cond variable");
  NEXUS_ASSERT_MESSAGE(mutex != NULL, "Attempted cond_wait on NULL mutex");

  if (cond == NULL || mutex == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (!SleepConditionVariableCS(&cond->cv, &mutex->cs, INFINITE)) {
    return NEXUS_ERROR_IO;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_cond_wait(&cond->handle, &mutex->handle) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

boolean nexus_threads_cond_wait_timeout(NexusCond *cond, NexusMutex *mutex, NexusDuration duration) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  DWORD ms;
  BOOL  res;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  struct timespec time_spec;
  uint64          total_ns;
  int             res;
#endif

  NEXUS_ASSERT_MESSAGE(cond != NULL, "Attempted cond_wait_timeout on NULL cond variable");
  NEXUS_ASSERT_MESSAGE(mutex != NULL, "Attempted cond_wait_timeout on NULL mutex");
  NEXUS_ASSERT_MESSAGE(duration.nanoseconds > 0, "Wait timeout duration must be strictly positive");

  if (cond == NULL || mutex == NULL || duration.nanoseconds <= 0) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  ms  = (DWORD)((duration.nanoseconds + NEXUS_NANOSECONDS_PER_MILLISECOND - 1) / NEXUS_NANOSECONDS_PER_MILLISECOND);
  res = SleepConditionVariableCS(&cond->cv, &mutex->cs, ms);
  return res != 0 ? TRUE : FALSE;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  clock_gettime(CLOCK_REALTIME, &time_spec);
  total_ns = (uint64)time_spec.tv_nsec + (uint64)duration.nanoseconds;
  time_spec.tv_sec += (time_t)(total_ns / NEXUS_NANOSECONDS_PER_SECOND);
  time_spec.tv_nsec = (long)(total_ns % NEXUS_NANOSECONDS_PER_SECOND);

  res = pthread_cond_timedwait(&cond->handle, &mutex->handle, &time_spec);
  return res == 0 ? TRUE : FALSE;
#endif
}

NError nexus_threads_cond_signal(NexusCond *cond) {
  NEXUS_ASSERT_MESSAGE(cond != NULL, "Attempted cond_signal on NULL cond variable");

  if (cond == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  WakeConditionVariable(&cond->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_cond_signal(&cond->handle) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_threads_cond_broadcast(NexusCond *cond) {
  NEXUS_ASSERT_MESSAGE(cond != NULL, "Attempted cond_broadcast on NULL cond variable");

  if (cond == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  WakeAllConditionVariable(&cond->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_cond_broadcast(&cond->handle) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

void nexus_threads_cond_destroy(NexusCond *cond) {
  NEXUS_ASSERT_MESSAGE(cond != NULL, "Attempted to destroy NULL cond variable");

  if (cond == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_cond_destroy(&cond->handle);
#endif

  free(cond);
}

/* ---------------------------------------------------------------------------- */
/* SEMAPHORE                                                                    */
/* ---------------------------------------------------------------------------- */

NError nexus_threads_semaphore_create(NexusSemaphore **out_sem, uint32 initial_count) {
  NexusSemaphore *sem;

  NEXUS_ASSERT_MESSAGE(out_sem != NULL, "Destination semaphore pointer cannot be NULL");

  if (out_sem == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_sem = NULL;

  sem = (NexusSemaphore *)malloc(NEXUS_SIZEOF(NexusSemaphore));
  if (sem == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  sem->handle = CreateSemaphoreW(NULL, (LONG)initial_count, LONG_MAX, NULL);
  if (sem->handle == NULL) {
    free(sem);
    return NEXUS_ERROR_IO;
  }
#elif defined(__APPLE__)
  sem->sem = dispatch_semaphore_create((long)initial_count);
  if (sem->sem == NULL) {
    free(sem);
    return NEXUS_ERROR_IO;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (sem_init(&sem->handle, 0, initial_count) != 0) {
    free(sem);
    return NEXUS_ERROR_IO;
  }
#endif

  *out_sem = sem;
  return NEXUS_ERROR_NONE;
}

NError nexus_threads_semaphore_wait(NexusSemaphore *sem) {
  NEXUS_ASSERT_MESSAGE(sem != NULL, "Attempted semaphore_wait on NULL semaphore");

  if (sem == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (WaitForSingleObject(sem->handle, INFINITE) != WAIT_OBJECT_0) {
    return NEXUS_ERROR_IO;
  }
#elif defined(__APPLE__)
  dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  while (sem_wait(&sem->handle) != 0) {
    if (errno != EINTR) {
      return NEXUS_ERROR_IO;
    }
  }
#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_threads_semaphore_post(NexusSemaphore *sem) {
  NEXUS_ASSERT_MESSAGE(sem != NULL, "Attempted semaphore_post on NULL semaphore");

  if (sem == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (!ReleaseSemaphore(sem->handle, 1, NULL)) {
    return NEXUS_ERROR_IO;
  }
#elif defined(__APPLE__)
  dispatch_semaphore_signal(sem->sem);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (sem_post(&sem->handle) != 0) {
    return NEXUS_ERROR_IO;
  }
#endif

  return NEXUS_ERROR_NONE;
}

void nexus_threads_semaphore_destroy(NexusSemaphore *sem) {
  NEXUS_ASSERT_MESSAGE(sem != NULL, "Attempted to destroy NULL semaphore");

  if (sem == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  CloseHandle(sem->handle);
#elif defined(__APPLE__)
  dispatch_release(sem->sem);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  sem_destroy(&sem->handle);
#endif

  free(sem);
}

/* ---------------------------------------------------------------------------- */
/* WAIT GROUP                                                                   */
/* ---------------------------------------------------------------------------- */

NError nexus_threads_waitgroup_create(NexusThreadsWaitGroup **out_waitgroup) {
  NexusThreadsWaitGroup *waitgroup;

  NEXUS_ASSERT_MESSAGE(out_waitgroup != NULL, "Destination waitgroup pointer cannot be NULL");

  if (out_waitgroup == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_waitgroup = NULL;

  waitgroup = (NexusThreadsWaitGroup *)malloc(NEXUS_SIZEOF(NexusThreadsWaitGroup));
  if (waitgroup == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

  waitgroup->counter = 0;

#if defined(NEXUS_PLATFORM_WINDOWS)
  InitializeCriticalSection(&waitgroup->cs);
  InitializeConditionVariable(&waitgroup->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_mutex_init(&waitgroup->mutex, NULL) != 0) {
    free(waitgroup);
    return NEXUS_ERROR_IO;
  }

  if (pthread_cond_init(&waitgroup->cond, NULL) != 0) {
    pthread_mutex_destroy(&waitgroup->mutex);
    free(waitgroup);
    return NEXUS_ERROR_IO;
  }
#endif

  *out_waitgroup = waitgroup;
  return NEXUS_ERROR_NONE;
}

NError nexus_threads_waitgroup_add(NexusThreadsWaitGroup *waitgroup, int32 delta) {
  NEXUS_ASSERT_MESSAGE(waitgroup != NULL, "Attempted waitgroup_add on NULL waitgroup");
  NEXUS_ASSERT_MESSAGE(delta != 0, "Waitgroup delta cannot be zero");

  if (waitgroup == NULL || delta == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  EnterCriticalSection(&waitgroup->cs);

  waitgroup->counter += delta;
  NEXUS_ASSERT_MESSAGE(waitgroup->counter >= 0, "Waitgroup counter went negative");

  if (waitgroup->counter == 0) {
    WakeAllConditionVariable(&waitgroup->cv);
  }

  LeaveCriticalSection(&waitgroup->cs);

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_mutex_lock(&waitgroup->mutex) != 0) {
    return NEXUS_ERROR_IO;
  }

  waitgroup->counter += delta;
  NEXUS_ASSERT_MESSAGE(waitgroup->counter >= 0, "Waitgroup counter went negative");

  if (waitgroup->counter == 0) {
    pthread_cond_broadcast(&waitgroup->cond);
  }

  pthread_mutex_unlock(&waitgroup->mutex);
#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_threads_waitgroup_done(NexusThreadsWaitGroup *waitgroup) {
  NEXUS_ASSERT_MESSAGE(waitgroup != NULL, "Attempted waitgroup_done on NULL waitgroup");
  return nexus_threads_waitgroup_add(waitgroup, -1);
}

NError nexus_threads_waitgroup_wait(NexusThreadsWaitGroup *waitgroup) {
  NEXUS_ASSERT_MESSAGE(waitgroup != NULL, "Attempted waitgroup_wait on NULL waitgroup");

  if (waitgroup == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  EnterCriticalSection(&waitgroup->cs);

  while (waitgroup->counter > 0) {
    SleepConditionVariableCS(&waitgroup->cv, &waitgroup->cs, INFINITE);
  }

  LeaveCriticalSection(&waitgroup->cs);

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_mutex_lock(&waitgroup->mutex) != 0) {
    return NEXUS_ERROR_IO;
  }

  while (waitgroup->counter > 0) {
    pthread_cond_wait(&waitgroup->cond, &waitgroup->mutex);
  }

  pthread_mutex_unlock(&waitgroup->mutex);
#endif

  return NEXUS_ERROR_NONE;
}

boolean nexus_threads_waitgroup_try_wait(NexusThreadsWaitGroup *waitgroup) {
  boolean ready;

  NEXUS_ASSERT_MESSAGE(waitgroup != NULL, "Attempted waitgroup_try_wait on NULL waitgroup");

  if (waitgroup == NULL) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  EnterCriticalSection(&waitgroup->cs);

  ready = waitgroup->counter == 0;

  LeaveCriticalSection(&waitgroup->cs);

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_mutex_lock(&waitgroup->mutex) != 0) {
    return FALSE;
  }

  ready = waitgroup->counter == 0;

  pthread_mutex_unlock(&waitgroup->mutex);
#else
  ready = FALSE;
#endif

  return ready;
}

void nexus_threads_waitgroup_destroy(NexusThreadsWaitGroup *waitgroup) {
  NEXUS_ASSERT_MESSAGE(waitgroup != NULL, "Attempted to destroy NULL waitgroup");

  if (waitgroup == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  DeleteCriticalSection(&waitgroup->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_cond_destroy(&waitgroup->cond);
  pthread_mutex_destroy(&waitgroup->mutex);
#endif

  free(waitgroup);
}
