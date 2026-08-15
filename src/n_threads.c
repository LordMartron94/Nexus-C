#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <windows.h>
#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
#  include <errno.h>
#  include <time.h>
#endif

#if NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32

#  if defined(_MSC_VER)
#    include <intrin.h>
#  elif defined(__GNUC__) || defined(__clang__)
#    include <immintrin.h>
#  endif

#endif

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

  /*
  Sleep() is appropriate once the requested interval is representable in
  milliseconds. Round upward so nexus_threads_sleep never deliberately
  undersleeps the requested duration.
  */
  if (nanoseconds >= NEXUS_NANOSECONDS_PER_MILLISECOND) {
    milliseconds = (DWORD)((nanoseconds + NEXUS_NANOSECONDS_PER_MILLISECOND - 1ULL) / NEXUS_NANOSECONDS_PER_MILLISECOND);

    Sleep(milliseconds);
    return;
  }

  /*
  Windows waitable timers use 100-nanosecond units.

  Negative due times represent relative intervals. Round upward to the nearest
  100 ns so the requested sleep is never shortened by conversion.
  */
  timer_ticks = (nanoseconds + 99ULL) / 100ULL;

  timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE);

  if (timer == NULL) {
    /*
    High-resolution timer creation may be unavailable on older Windows
    versions. Fall back to Sleep(1), preserving the minimum-sleep semantic.
    */
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

#else

  /*
  No portable CPU spin-loop hint is known for this architecture.
  The monotonic-clock polling itself prevents the loop from being empty.
  */

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