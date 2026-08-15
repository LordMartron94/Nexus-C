#if !defined(_WIN32)
#  ifndef _POSIX_C_SOURCE
#    define _POSIX_C_SOURCE 200809L
#  endif
#endif

#include <signal.h>

#include "../nexus.h"

static volatile sig_atomic_t n_signals_received[NEXUS_SIGNAL_COUNT];

static void n_signals_handler(int signal_number) {
  switch (signal_number) {
  case SIGINT:
    n_signals_received[NEXUS_SIGNAL_INTERRUPT] = 1;
    break;

  case SIGTERM:
    n_signals_received[NEXUS_SIGNAL_TERMINATE] = 1;
    break;

  default:
    break;
  }
}

static int n_signals_native_number_get(NexusSignal signal) {
  switch (signal) {
  case NEXUS_SIGNAL_INTERRUPT:
    return SIGINT;

  case NEXUS_SIGNAL_TERMINATE:
    return SIGTERM;

  default:
    return 0;
  }
}

NError nexus_signals_capture(NexusSignal signal) {
#if defined(NEXUS_PLATFORM_POSIX)
  struct sigaction action;
#endif
  int signal_number;

  if (signal >= NEXUS_SIGNAL_COUNT) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  signal_number = n_signals_native_number_get(signal);

  if (signal_number == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  n_signals_received[signal] = 0;

#if defined(NEXUS_PLATFORM_POSIX)

  action.sa_handler = n_signals_handler;
  action.sa_flags   = 0;

  if (sigemptyset(&action.sa_mask) != 0) {
    return NEXUS_ERROR_IO;
  }

  if (sigaction(signal_number, &action, NULL) != 0) {
    return NEXUS_ERROR_IO;
  }

#else

  if (signal(signal_number, n_signals_handler) == SIG_ERR) {
    return NEXUS_ERROR_IO;
  }

#endif

  return NEXUS_ERROR_NONE;
}

NError nexus_signals_release(NexusSignal signal) {
#if defined(NEXUS_PLATFORM_POSIX)
  struct sigaction action;
#endif
  int signal_number;

  if (signal >= NEXUS_SIGNAL_COUNT) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  signal_number = n_signals_native_number_get(signal);

  if (signal_number == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_POSIX)

  action.sa_handler = SIG_DFL;
  action.sa_flags   = 0;

  if (sigemptyset(&action.sa_mask) != 0) {
    return NEXUS_ERROR_IO;
  }

  if (sigaction(signal_number, &action, NULL) != 0) {
    return NEXUS_ERROR_IO;
  }

#else

  if (signal(signal_number, SIG_DFL) == SIG_ERR) {
    return NEXUS_ERROR_IO;
  }

#endif

  n_signals_received[signal] = 0;

  return NEXUS_ERROR_NONE;
}

boolean nexus_signals_received_get(NexusSignal signal) {
  NEXUS_ASSERT_DEBUG(signal < NEXUS_SIGNAL_COUNT);

  return n_signals_received[signal] != 0 ? TRUE : FALSE;
}

boolean nexus_signals_received_exchange(NexusSignal signal) {
  boolean received;

  NEXUS_ASSERT_DEBUG(signal < NEXUS_SIGNAL_COUNT);

  received = n_signals_received[signal] != 0 ? TRUE : FALSE;

  n_signals_received[signal] = 0;

  return received;
}

void nexus_signals_received_clear(NexusSignal signal) {
  NEXUS_ASSERT_DEBUG(signal < NEXUS_SIGNAL_COUNT);

  n_signals_received[signal] = 0;
}