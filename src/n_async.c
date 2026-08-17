#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <sys/event.h>
#  include <sys/time.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/epoll.h>
#endif

/* ---------------------------------------------------------------------------- */
/* PLATFORM INTERNAL STRUCTURE DEFINITIONS                                      */
/* ---------------------------------------------------------------------------- */

#if defined(NEXUS_PLATFORM_WINDOWS)

typedef struct Win32MonitoredHandle {
  SOCKET socket_handle;
  uint32 interests;
  void  *user_data;
} Win32MonitoredHandle;

struct NexusAsyncPoller {
  Win32MonitoredHandle *items;
  WSAPOLLFD            *poll_fds;
  uint32                count;
  uint32                capacity;
};

#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)

struct NexusAsyncPoller {
  int kq_fd;
};

#elif defined(NEXUS_PLATFORM_POSIX)

struct NexusAsyncPoller {
  int epoll_fd;
};

#endif

/* ---------------------------------------------------------------------------- */
/* NON-BLOCKING UTILITIES                                                       */
/* ---------------------------------------------------------------------------- */

NError nexus_async_set_nonblocking(NexusNativeHandle handle, boolean non_blocking) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  u_long mode;

  mode = non_blocking ? 1UL : 0UL;
  if (ioctlsocket((SOCKET)handle, FIONBIO, &mode) != 0) {
    return NEXUS_ERROR_IO;
  }
  return NEXUS_ERROR_NONE;

#elif defined(NEXUS_PLATFORM_POSIX) || defined(NEXUS_PLATFORM_BSD)
  int flags;

  flags = fcntl((int)handle, F_GETFL, 0);
  if (flags < 0) {
    return NEXUS_ERROR_IO;
  }

  if (non_blocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  if (fcntl((int)handle, F_SETFL, flags) < 0) {
    return NEXUS_ERROR_IO;
  }
  return NEXUS_ERROR_NONE;
#else
  (void)handle;
  (void)non_blocking;
  return NEXUS_ERROR_IO;
#endif
}

/* ---------------------------------------------------------------------------- */
/* POLLER LIFECYCLE & REGISTRATION                                              */
/* ---------------------------------------------------------------------------- */

NError nexus_async_poller_create(NexusAsyncPoller **out_poller) {
  NexusAsyncPoller *poller;

  NEXUS_ASSERT_MESSAGE(out_poller != NULL, "Destination poller pointer cannot be NULL");

  if (out_poller == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_poller = NULL;

  poller = (NexusAsyncPoller *)malloc(NEXUS_SIZEOF(NexusAsyncPoller));
  if (poller == NULL) {
    return NEXUS_ERROR_CAPACITY;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  poller->count    = 0;
  poller->capacity = 16;
  poller->items    = (Win32MonitoredHandle *)malloc(NEXUS_SIZEOF(Win32MonitoredHandle) * poller->capacity);
  poller->poll_fds = (WSAPOLLFD *)malloc(NEXUS_SIZEOF(WSAPOLLFD) * poller->capacity);

  if (poller->items == NULL || poller->poll_fds == NULL) {
    if (poller->items != NULL)
      free(poller->items);
    if (poller->poll_fds != NULL)
      free(poller->poll_fds);
    free(poller);
    return NEXUS_ERROR_CAPACITY;
  }

#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
  poller->kq_fd = kqueue();
  if (poller->kq_fd < 0) {
    free(poller);
    return NEXUS_ERROR_IO;
  }

#elif defined(NEXUS_PLATFORM_POSIX)
  poller->epoll_fd = epoll_create1(0);
  if (poller->epoll_fd < 0) {
    free(poller);
    return NEXUS_ERROR_IO;
  }
#endif

  *out_poller = poller;
  return NEXUS_ERROR_NONE;
}

NError nexus_async_poller_add(NexusAsyncPoller *poller, NexusNativeHandle handle, uint32 interests, void *user_data) {
  NEXUS_ASSERT_MESSAGE(poller != NULL, "Attempted to register handle with NULL poller");
  NEXUS_ASSERT_MESSAGE(interests != 0, "Poller interest bitmask cannot be zero");

  if (poller == NULL || interests == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    uint32 i;
    short  events;

    for (i = 0; i < poller->count; ++i) {
      if (poller->items[i].socket_handle == (SOCKET)handle) {
        return NEXUS_ERROR_IO; /* Already exists */
      }
    }

    if (poller->count >= poller->capacity) {
      uint32                new_capacity;
      Win32MonitoredHandle *new_items;
      WSAPOLLFD            *new_poll_fds;

      new_capacity = poller->capacity * 2;
      new_items    = (Win32MonitoredHandle *)realloc(poller->items, NEXUS_SIZEOF(Win32MonitoredHandle) * new_capacity);
      new_poll_fds = (WSAPOLLFD *)realloc(poller->poll_fds, NEXUS_SIZEOF(WSAPOLLFD) * new_capacity);

      if (new_items == NULL || new_poll_fds == NULL) {
        if (new_items != NULL)
          poller->items = new_items;
        if (new_poll_fds != NULL)
          poller->poll_fds = new_poll_fds;
        return NEXUS_ERROR_CAPACITY;
      }

      poller->items    = new_items;
      poller->poll_fds = new_poll_fds;
      poller->capacity = new_capacity;
    }

    events = 0;
    if (interests & NEXUS_ASYNC_INTEREST_READ)
      events |= POLLRDNORM | POLLRDBAND;
    if (interests & NEXUS_ASYNC_INTEREST_WRITE)
      events |= POLLWRNORM;

    poller->items[poller->count].socket_handle = (SOCKET)handle;
    poller->items[poller->count].interests     = interests;
    poller->items[poller->count].user_data     = user_data;

    poller->poll_fds[poller->count].fd      = (SOCKET)handle;
    poller->poll_fds[poller->count].events  = events;
    poller->poll_fds[poller->count].revents = 0;

    poller->count++;
    return NEXUS_ERROR_NONE;
  }

#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
  {
    struct kevent kev[2];
    int           nchanges;

    nchanges = 0;

    if (interests & NEXUS_ASYNC_INTEREST_READ) {
      EV_SET(&kev[nchanges], (uintptr_t)handle, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, user_data);
      nchanges++;
    }
    if (interests & NEXUS_ASYNC_INTEREST_WRITE) {
      EV_SET(&kev[nchanges], (uintptr_t)handle, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, user_data);
      nchanges++;
    }

    if (kevent(poller->kq_fd, kev, nchanges, NULL, 0, NULL) < 0) {
      return NEXUS_ERROR_IO;
    }
    return NEXUS_ERROR_NONE;
  }

#elif defined(NEXUS_PLATFORM_POSIX)
  {
    struct epoll_event event;

    event.events   = 0;
    event.data.ptr = user_data;

    if (interests & NEXUS_ASYNC_INTEREST_READ)
      event.events |= EPOLLIN;
    if (interests & NEXUS_ASYNC_INTEREST_WRITE)
      event.events |= EPOLLOUT;
    if (interests & NEXUS_ASYNC_INTEREST_EDGE)
      event.events |= EPOLLET;

    if (epoll_ctl(poller->epoll_fd, EPOLL_CTL_ADD, (int)handle, &event) < 0) {
      return NEXUS_ERROR_IO;
    }
    return NEXUS_ERROR_NONE;
  }
#endif
}

NError nexus_async_poller_modify(NexusAsyncPoller *poller, NexusNativeHandle handle, uint32 interests, void *user_data) {
  NEXUS_ASSERT_MESSAGE(poller != NULL, "Attempted to modify handle with NULL poller");
  NEXUS_ASSERT_MESSAGE(interests != 0, "Poller interest bitmask cannot be zero");

  if (poller == NULL || interests == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    uint32 i;
    short  events;

    for (i = 0; i < poller->count; ++i) {
      if (poller->items[i].socket_handle == (SOCKET)handle) {
        events = 0;
        if (interests & NEXUS_ASYNC_INTEREST_READ)
          events |= POLLRDNORM | POLLRDBAND;
        if (interests & NEXUS_ASYNC_INTEREST_WRITE)
          events |= POLLWRNORM;

        poller->items[i].interests = interests;
        poller->items[i].user_data = user_data;

        poller->poll_fds[i].events  = events;
        poller->poll_fds[i].revents = 0;
        return NEXUS_ERROR_NONE;
      }
    }
    return NEXUS_ERROR_IO;
  }

#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
  {
    NError err;

    err = nexus_async_poller_remove(poller, handle);
    if (err != NEXUS_ERROR_NONE) {
      return err;
    }
    return nexus_async_poller_add(poller, handle, interests, user_data);
  }

#elif defined(NEXUS_PLATFORM_POSIX)
  {
    struct epoll_event event;

    event.events   = 0;
    event.data.ptr = user_data;

    if (interests & NEXUS_ASYNC_INTEREST_READ)
      event.events |= EPOLLIN;
    if (interests & NEXUS_ASYNC_INTEREST_WRITE)
      event.events |= EPOLLOUT;
    if (interests & NEXUS_ASYNC_INTEREST_EDGE)
      event.events |= EPOLLET;

    if (epoll_ctl(poller->epoll_fd, EPOLL_CTL_MOD, (int)handle, &event) < 0) {
      return NEXUS_ERROR_IO;
    }
    return NEXUS_ERROR_NONE;
  }
#endif
}

NError nexus_async_poller_remove(NexusAsyncPoller *poller, NexusNativeHandle handle) {
  NEXUS_ASSERT_MESSAGE(poller != NULL, "Attempted to remove handle from NULL poller");

  if (poller == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    uint32 i;

    for (i = 0; i < poller->count; ++i) {
      if (poller->items[i].socket_handle == (SOCKET)handle) {
        if (i < poller->count - 1) {
          poller->items[i]    = poller->items[poller->count - 1];
          poller->poll_fds[i] = poller->poll_fds[poller->count - 1];
        }
        poller->count--;
        return NEXUS_ERROR_NONE;
      }
    }
    return NEXUS_ERROR_IO;
  }

#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
  {
    struct kevent kev[2];

    EV_SET(&kev[0], (uintptr_t)handle, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&kev[1], (uintptr_t)handle, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

    /* Ignore errors if one filter was not actively registered */
    kevent(poller->kq_fd, kev, 2, NULL, 0, NULL);
    return NEXUS_ERROR_NONE;
  }

#elif defined(NEXUS_PLATFORM_POSIX)
  {
    if (epoll_ctl(poller->epoll_fd, EPOLL_CTL_DEL, (int)handle, NULL) < 0) {
      return NEXUS_ERROR_IO;
    }
    return NEXUS_ERROR_NONE;
  }
#endif
}

/* ---------------------------------------------------------------------------- */
/* EVENT DEMULTIPLEXING WAIT                                                    */
/* ---------------------------------------------------------------------------- */

NError nexus_async_poller_wait(NexusAsyncPoller *poller, NexusAsyncEvent *out_events, uint32 max_events, NexusDuration duration,
                               uint32 *out_event_count) {
  NEXUS_ASSERT_MESSAGE(poller != NULL, "Attempted poll on NULL poller");
  NEXUS_ASSERT_MESSAGE(out_events != NULL, "Destination event array cannot be NULL");
  NEXUS_ASSERT_MESSAGE(out_event_count != NULL, "Destination event count pointer cannot be NULL");

  if (poller == NULL || out_events == NULL || out_event_count == NULL || max_events == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_event_count = 0;

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    int    timeout_ms;
    int    poll_res;
    uint32 i;
    uint32 fired_count;

    if (duration.nanoseconds < 0) {
      timeout_ms = -1;
    } else {
      timeout_ms = (int)((duration.nanoseconds + NEXUS_NANOSECONDS_PER_MILLISECOND - 1ULL) / NEXUS_NANOSECONDS_PER_MILLISECOND);
    }

    if (poller->count == 0) {
      return NEXUS_ERROR_NONE;
    }

    poll_res = WSAPoll(poller->poll_fds, (ULONG)poller->count, timeout_ms);
    if (poll_res < 0) {
      return NEXUS_ERROR_IO;
    }

    fired_count = 0;
    for (i = 0; i < poller->count && fired_count < max_events; ++i) {
      if (poller->poll_fds[i].revents != 0) {
        uint32 flags;

        flags = 0;
        if (poller->poll_fds[i].revents & (POLLRDNORM | POLLRDBAND | POLLPRI)) {
          flags |= NEXUS_ASYNC_INTEREST_READ;
        }
        if (poller->poll_fds[i].revents & POLLWRNORM) {
          flags |= NEXUS_ASYNC_INTEREST_WRITE;
        }
        if (poller->poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
          flags |= NEXUS_ASYNC_INTEREST_ERROR;
        }

        out_events[fired_count].handle    = (NexusNativeHandle)poller->items[i].socket_handle;
        out_events[fired_count].events    = flags;
        out_events[fired_count].user_data = poller->items[i].user_data;
        fired_count++;
      }
    }

    *out_event_count = fired_count;
    return NEXUS_ERROR_NONE;
  }

#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
  {
    struct timespec  timeout_ts;
    struct timespec *timeout_ptr;
    struct kevent    events_buf[64];
    int              n_events;
    int              fetch_max;
    int              i;
    uint32           fired_count;

    if (duration.nanoseconds < 0) {
      timeout_ptr = NULL;
    } else {
      timeout_ts.tv_sec  = (time_t)((uint64)duration.nanoseconds / NEXUS_NANOSECONDS_PER_SECOND);
      timeout_ts.tv_nsec = (long)((uint64)duration.nanoseconds % NEXUS_NANOSECONDS_PER_SECOND);
      timeout_ptr        = &timeout_ts;
    }

    fetch_max = (int)max_events < 64 ? (int)max_events : 64;
    n_events  = kevent(poller->kq_fd, NULL, 0, events_buf, fetch_max, timeout_ptr);

    if (n_events < 0) {
      return NEXUS_ERROR_IO;
    }

    fired_count = 0;
    for (i = 0; i < n_events; ++i) {
      uint32 flags;

      flags = 0;
      if (events_buf[i].filter == EVFILT_READ)
        flags |= NEXUS_ASYNC_INTEREST_READ;
      if (events_buf[i].filter == EVFILT_WRITE)
        flags |= NEXUS_ASYNC_INTEREST_WRITE;
      if (events_buf[i].flags & EV_ERROR)
        flags |= NEXUS_ASYNC_INTEREST_ERROR;

      out_events[fired_count].handle    = (NexusNativeHandle)events_buf[i].ident;
      out_events[fired_count].events    = flags;
      out_events[fired_count].user_data = events_buf[i].udata;
      fired_count++;
    }

    *out_event_count = fired_count;
    return NEXUS_ERROR_NONE;
  }

#elif defined(NEXUS_PLATFORM_POSIX)
  {
    int                timeout_ms;
    struct epoll_event events_buf[64];
    int                n_events;
    int                fetch_max;
    int                i;
    uint32             fired_count;

    if (duration.nanoseconds < 0) {
      timeout_ms = -1;
    } else {
      timeout_ms = (int)((duration.nanoseconds + NEXUS_NANOSECONDS_PER_MILLISECOND - 1ULL) / NEXUS_NANOSECONDS_PER_MILLISECOND);
    }

    fetch_max = (int)max_events < 64 ? (int)max_events : 64;
    n_events  = epoll_wait(poller->epoll_fd, events_buf, fetch_max, timeout_ms);

    if (n_events < 0) {
      return NEXUS_ERROR_IO;
    }

    fired_count = 0;
    for (i = 0; i < n_events; ++i) {
      uint32 flags;

      flags = 0;
      if (events_buf[i].events & EPOLLIN)
        flags |= NEXUS_ASYNC_INTEREST_READ;
      if (events_buf[i].events & EPOLLOUT)
        flags |= NEXUS_ASYNC_INTEREST_WRITE;
      if (events_buf[i].events & (EPOLLERR | EPOLLHUP))
        flags |= NEXUS_ASYNC_INTEREST_ERROR;

      out_events[fired_count].handle    = 0; /* Identifiers are tracked through user_data pointer */
      out_events[fired_count].events    = flags;
      out_events[fired_count].user_data = events_buf[i].data.ptr;
      fired_count++;
    }

    *out_event_count = fired_count;
    return NEXUS_ERROR_NONE;
  }
#endif
}

void nexus_async_poller_destroy(NexusAsyncPoller *poller) {
  NEXUS_ASSERT_MESSAGE(poller != NULL, "Attempted to destroy NULL poller");

  if (poller == NULL) {
    return;
  }

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (poller->items != NULL) {
    free(poller->items);
  }
  if (poller->poll_fds != NULL) {
    free(poller->poll_fds);
  }
#elif defined(__APPLE__) || defined(NEXUS_PLATFORM_BSD)
  if (poller->kq_fd >= 0) {
    close(poller->kq_fd);
  }
#elif defined(NEXUS_PLATFORM_POSIX)
  if (poller->epoll_fd >= 0) {
    close(poller->epoll_fd);
  }
#endif

  free(poller);
}