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

#include <stdlib.h>

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

/* ---------------------------------------------------------------------------- */
/* THREAD ENTRY TRAMPOLINE                                                      */
/* ---------------------------------------------------------------------------- */

#if defined(NEXUS_PLATFORM_WINDOWS)
static unsigned __stdcall nexus_thread_entry_win32(void *arg) {
  NexusThread *thread;
  thread = (NexusThread *)arg;
  if (thread != NULL && thread->entry_func != NULL) {
    thread->entry_func(thread->user_data);
  }
  return 0;
}
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
static void *nexus_thread_entry_posix(void *arg) {
  NexusThread *thread;
  thread = (NexusThread *)arg;
  if (thread != NULL && thread->entry_func != NULL) {
    thread->entry_func(thread->user_data);
  }
  return NULL;
}
#endif

/* ---------------------------------------------------------------------------- */
/* THREAD LIFECYCLE                                                             */
/* ---------------------------------------------------------------------------- */

NexusThread *nexus_thread_create(NexusThreadFunc entry_func, void *user_data) {
  NexusThread *thread;

  if (entry_func == NULL) {
    return NULL;
  }

  thread = (NexusThread *)malloc(sizeof(NexusThread));
  if (thread == NULL) {
    return NULL;
  }

  thread->entry_func = entry_func;
  thread->user_data  = user_data;

#if defined(NEXUS_PLATFORM_WINDOWS)
  thread->handle = (HANDLE)_beginthreadex(NULL, 0, nexus_thread_entry_win32, thread, 0, &thread->thread_id);

  if (thread->handle == NULL || thread->handle == (HANDLE)-1L) {
    free(thread);
    return NULL;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_create(&thread->handle, NULL, nexus_thread_entry_posix, thread) != 0) {
    free(thread);
    return NULL;
  }
#endif

  return thread;
}

void nexus_thread_join(NexusThread *thread) {
  if (thread == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (thread->handle != NULL) {
    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    thread->handle = NULL;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_join(thread->handle, NULL);
#endif

  free(thread);
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

  if (duration.nanoseconds <= 0) {
    return;
  }

  target = nexus_time_add_duration(nexus_time_get_monotonic(), duration);

  while (nexus_time_get_monotonic().time < target.time) {
    nexus_threads_spin_hint();
  }
}

/* ---------------------------------------------------------------------------- */
/* MUTEX                                                                        */
/* ---------------------------------------------------------------------------- */

NexusMutex *nexus_mutex_create(void) {
  NexusMutex *mutex;

  mutex = (NexusMutex *)malloc(sizeof(NexusMutex));
  if (mutex == NULL) {
    return NULL;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  InitializeCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&mutex->handle, &attr) != 0) {
      pthread_mutexattr_destroy(&attr);
      free(mutex);
      return NULL;
    }
    pthread_mutexattr_destroy(&attr);
  }
#endif

  return mutex;
}

void nexus_mutex_lock(NexusMutex *mutex) {
  if (mutex == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  EnterCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_mutex_lock(&mutex->handle);
#endif
}

boolean nexus_mutex_try_lock(NexusMutex *mutex) {
  if (mutex == NULL) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  return TryEnterCriticalSection(&mutex->cs) != 0 ? 1 : 0;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  return pthread_mutex_trylock(&mutex->handle) == 0 ? TRUE : FALSE;
#endif
}

void nexus_mutex_unlock(NexusMutex *mutex) {
  if (mutex == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  LeaveCriticalSection(&mutex->cs);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_mutex_unlock(&mutex->handle);
#endif
}

void nexus_mutex_destroy(NexusMutex *mutex) {
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

NexusCond *nexus_cond_create(void) {
  NexusCond *cond;

  cond = (NexusCond *)malloc(sizeof(NexusCond));
  if (cond == NULL) {
    return NULL;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  InitializeConditionVariable(&cond->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (pthread_cond_init(&cond->handle, NULL) != 0) {
    free(cond);
    return NULL;
  }
#endif

  return cond;
}

void nexus_cond_wait(NexusCond *cond, NexusMutex *mutex) {
  if (cond == NULL || mutex == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  SleepConditionVariableCS(&cond->cv, &mutex->cs, INFINITE);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_cond_wait(&cond->handle, &mutex->handle);
#endif
}

boolean nexus_cond_wait_timeout(NexusCond *cond, NexusMutex *mutex, NexusDuration duration) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  DWORD ms;
  BOOL  res;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  struct timespec time_spec;
  uint64          total_ns;
  int             res;
#endif

  if (cond == NULL || mutex == NULL) {
    return FALSE;
  }

  if (duration.nanoseconds <= 0) {
    return FALSE;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  ms  = (DWORD)((duration.nanoseconds + NEXUS_NANOSECONDS_PER_MILLISECOND - 1) / NEXUS_NANOSECONDS_PER_MILLISECOND);
  res = SleepConditionVariableCS(&cond->cv, &mutex->cs, ms);
  return res != 0 ? 1 : 0;
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  clock_gettime(CLOCK_REALTIME, &time_spec);
  total_ns = (uint64)time_spec.tv_nsec + (uint64)duration.nanoseconds;
  time_spec.tv_sec += (time_t)(total_ns / NEXUS_NANOSECONDS_PER_SECOND);
  time_spec.tv_nsec = (long)(total_ns % NEXUS_NANOSECONDS_PER_SECOND);

  res = pthread_cond_timedwait(&cond->handle, &mutex->handle, &time_spec);
  return res == 0 ? TRUE : FALSE;
#endif
}

void nexus_cond_signal(NexusCond *cond) {
  if (cond == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  WakeConditionVariable(&cond->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_cond_signal(&cond->handle);
#endif
}

void nexus_cond_broadcast(NexusCond *cond) {
  if (cond == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  WakeAllConditionVariable(&cond->cv);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  pthread_cond_broadcast(&cond->handle);
#endif
}

void nexus_cond_destroy(NexusCond *cond) {
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

NexusSemaphore *nexus_semaphore_create(uint32 initial_count) {
  NexusSemaphore *sem;

  sem = (NexusSemaphore *)malloc(sizeof(NexusSemaphore));
  if (sem == NULL) {
    return NULL;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  sem->handle = CreateSemaphoreW(NULL, (LONG)initial_count, LONG_MAX, NULL);
  if (sem->handle == NULL) {
    free(sem);
    return NULL;
  }
#elif defined(__APPLE__)
  sem->sem = dispatch_semaphore_create((long)initial_count);
  if (sem->sem == NULL) {
    free(sem);
    return NULL;
  }
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  if (sem_init(&sem->handle, 0, initial_count) != 0) {
    free(sem);
    return NULL;
  }
#endif

  return sem;
}

void nexus_semaphore_wait(NexusSemaphore *sem) {
  if (sem == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  WaitForSingleObject(sem->handle, INFINITE);
#elif defined(__APPLE__)
  dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  while (sem_wait(&sem->handle) != 0) {
    if (errno != EINTR) {
      break;
    }
  }
#endif
}

void nexus_semaphore_post(NexusSemaphore *sem) {
  if (sem == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  ReleaseSemaphore(sem->handle, 1, NULL);
#elif defined(__APPLE__)
  dispatch_semaphore_signal(sem->sem);
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  sem_post(&sem->handle);
#endif
}

void nexus_semaphore_destroy(NexusSemaphore *sem) {
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