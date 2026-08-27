#if defined(__linux__)
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#  include <malloc.h>
#endif

#include "../nexus.h"

#if defined(_MSC_VER)
#  include <intrin.h>
#endif

#if (NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32)
#  if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#    include <xmmintrin.h>
#  endif
#endif

#if NEXUS_PLATFORM_POSIX
#  include <sys/resource.h>
#endif

#if defined(NEXUS_PLATFORM_LINUX)
#  include <errno.h>
#  include <linux/perf_event.h>
#  include <string.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#  include <sys/ioctl.h>
#endif

#if NEXUS_PLATFORM_WINDOWS
#  include <stdlib.h>
#  include <windows.h>
#  include <psapi.h>
#endif

#define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 0

#if (NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if defined(_MSC_VER)
  *cycle_count = (uint64)__rdtsc();
  return NEXUS_ERROR_NONE;
#  elif (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#    if defined(__has_builtin)
#      if __has_builtin(__builtin_ia32_rdtsc)
  *cycle_count = (uint64)__builtin_ia32_rdtsc();
  return NEXUS_ERROR_NONE;
#      endif
#    elif defined(__GNUC__)
  *cycle_count = (uint64)__builtin_ia32_rdtsc();
  return NEXUS_ERROR_NONE;
#    endif

  {
    unsigned int low;
    unsigned int high;

    __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
    *cycle_count = ((uint64)high << 32) | (uint64)low;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#elif (NEXUS_ARCH == NEXUS_ARCH_ARM64)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if defined(_MSC_VER)
  *cycle_count = (uint64)__rdpmccntr64();
  return NEXUS_ERROR_NONE;
#  elif (defined(__GNUC__) || defined(__clang__)) && defined(__aarch64__)
#    if defined(__has_builtin)
#      if __has_builtin(__builtin_arm_rsr64)
  *cycle_count = (uint64)__builtin_arm_rsr64("cntvct_el0");
  return NEXUS_ERROR_NONE;
#      endif
#    else
  *cycle_count = (uint64)__builtin_arm_rsr64("cntvct_el0");
  return NEXUS_ERROR_NONE;
#    endif

  {
    uint64 counter;

    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(counter));
    *cycle_count = counter;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#elif (NEXUS_ARCH == NEXUS_ARCH_ARM7 || NEXUS_ARCH == NEXUS_ARCH_ARM7A || NEXUS_ARCH == NEXUS_ARCH_ARM7R || NEXUS_ARCH == NEXUS_ARCH_ARM7S)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if defined(__GNUC__) || defined(__clang__)
  {
    uint32 counter;

    __asm__ __volatile__("mrc p15, 0, %0, c14, c0, 0" : "=r"(counter));
    *cycle_count = (uint64)counter;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#elif (NEXUS_ARCH == NEXUS_ARCH_RISCV32 || NEXUS_ARCH == NEXUS_ARCH_RISCV64)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if (defined(__GNUC__) || defined(__clang__)) && defined(__riscv)
#    if defined(__has_builtin)
#      if __has_builtin(__builtin_riscv_rdcycle)
  *cycle_count = (uint64)__builtin_riscv_rdcycle();
  return NEXUS_ERROR_NONE;
#      endif
#    else
  *cycle_count = (uint64)__builtin_riscv_rdcycle();
  return NEXUS_ERROR_NONE;
#    endif
#  endif

#  if defined(__GNUC__) || defined(__clang__)
  {
    uint_large counter;

    __asm__ __volatile__("rdcycle %0" : "=r"(counter));
    *cycle_count = (uint64)counter;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#elif (NEXUS_ARCH == NEXUS_ARCH_POWERPC || NEXUS_ARCH == NEXUS_ARCH_POWERPC64)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if defined(__GNUC__) || defined(__clang__)
  {
    uint32 timebase_upper_first;
    uint32 timebase_lower;
    uint32 timebase_upper_second;

    do {
      __asm__ __volatile__("mfspr %0, 269" : "=r"(timebase_upper_first));
      __asm__ __volatile__("mfspr %0, 268" : "=r"(timebase_lower));
      __asm__ __volatile__("mfspr %0, 269" : "=r"(timebase_upper_second));
    } while (timebase_upper_first != timebase_upper_second);

    *cycle_count = ((uint64)timebase_upper_first << 32) | (uint64)timebase_lower;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#elif (NEXUS_ARCH == NEXUS_ARCH_MIPS)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if defined(__GNUC__) || defined(__clang__)
  {
    uint32 counter;

    __asm__ __volatile__("rdhwr %0, $2" : "=r"(counter));
    *cycle_count = (uint64)counter;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#elif (NEXUS_ARCH == NEXUS_ARCH_SPARC)

#  undef NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED
#  define NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED 1

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
#  if defined(__GNUC__) || defined(__clang__)
  {
    uint64 counter;

    __asm__ __volatile__("rd %%tick, %0" : "=r"(counter));
    *cycle_count = counter;
    return NEXUS_ERROR_NONE;
  }
#  else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#  endif
}

#endif

#if !NEXUS_HARDWARE_CPU_CLOCK_CYCLES_IMPLEMENTED

static NError n_hardware_cpu_clock_cycles_read(uint64 *cycle_count) {
  (void)cycle_count;
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
}

#endif

NError nexus_hardware_cpu_clock_cycles_get(uint64 *cycle_count) {
  if (cycle_count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  return n_hardware_cpu_clock_cycles_read(cycle_count);
}

#if NEXUS_PLATFORM_POSIX

static NError n_hardware_rusage_get(struct rusage *resource_usage) {
  int getrusage_result;

  if (resource_usage == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#  if defined(RUSAGE_THREAD)
  getrusage_result = getrusage(RUSAGE_THREAD, resource_usage);
  if (getrusage_result != 0) {
    getrusage_result = getrusage(RUSAGE_SELF, resource_usage);
  }
#  else
  getrusage_result = getrusage(RUSAGE_SELF, resource_usage);
#  endif

  if (getrusage_result != 0) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
}

#endif

#if NEXUS_PLATFORM_WINDOWS

typedef LONG NTSTATUS;

#  ifndef STATUS_SUCCESS
#    define STATUS_SUCCESS ((NTSTATUS)0)
#  endif

#  ifndef STATUS_INFO_LENGTH_MISMATCH
#    define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#  endif

#  define NEXUS_HARDWARE_WINDOWS_SYSTEM_PROCESS_INFORMATION 5

typedef NTSTATUS(NTAPI *NexusHardwareWindowsNtQuerySystemInformationFunc)(ULONG system_information_class, void *system_information,
                                                                          ULONG system_information_length, ULONG *return_length);

typedef struct NexusHardwareWindowsClientId {
  HANDLE unique_process;
  HANDLE unique_thread;
} NexusHardwareWindowsClientId;

typedef struct NexusHardwareWindowsSystemThreadInformation {
  LARGE_INTEGER                kernel_time;
  LARGE_INTEGER                user_time;
  LARGE_INTEGER                create_time;
  ULONG                        wait_time;
  void                        *start_address;
  NexusHardwareWindowsClientId client_id;
  LONG                         priority;
  LONG                         base_priority;
  ULONG                        context_switch_count;
  ULONG                        thread_state;
  ULONG                        wait_reason;
} NexusHardwareWindowsSystemThreadInformation;

typedef struct NexusHardwareWindowsSystemProcessInformation {
  ULONG                                       next_entry_offset;
  ULONG                                       number_of_threads;
  LARGE_INTEGER                               working_set_private_size;
  LARGE_INTEGER                               hard_fault_count;
  LARGE_INTEGER                               number_of_threads_high_watermark;
  LARGE_INTEGER                               cycle_time;
  LARGE_INTEGER                               create_time;
  LARGE_INTEGER                               user_time;
  LARGE_INTEGER                               kernel_time;
  UNICODE_STRING                              image_name;
  LONG                                        base_priority;
  HANDLE                                      unique_process_id;
  HANDLE                                      inherited_from_unique_process_id;
  ULONG                                       handle_count;
  ULONG                                       session_id;
  ULONG_PTR                                   unique_process_key;
  SIZE_T                                      peak_virtual_size;
  SIZE_T                                      virtual_size;
  ULONG                                       page_fault_count;
  SIZE_T                                      peak_working_set_size;
  SIZE_T                                      working_set_size;
  SIZE_T                                      quota_peak_paged_pool_usage;
  SIZE_T                                      quota_paged_pool_usage;
  SIZE_T                                      quota_peak_non_paged_pool_usage;
  SIZE_T                                      quota_non_paged_pool_usage;
  SIZE_T                                      pagefile_usage;
  SIZE_T                                      peak_pagefile_usage;
  SIZE_T                                      private_usage;
  LARGE_INTEGER                               read_operation_count;
  LARGE_INTEGER                               write_operation_count;
  LARGE_INTEGER                               other_operation_count;
  LARGE_INTEGER                               read_transfer_count;
  LARGE_INTEGER                               write_transfer_count;
  LARGE_INTEGER                               other_transfer_count;
  NexusHardwareWindowsSystemThreadInformation threads[1];
} NexusHardwareWindowsSystemProcessInformation;

static NexusHardwareWindowsNtQuerySystemInformationFunc n_hardware_windows_nt_query_system_information_get(void) {
  static NexusHardwareWindowsNtQuerySystemInformationFunc function_pointer  = NULL;
  static boolean                                          function_resolved = FALSE;

  if (function_resolved == FALSE) {
    HMODULE ntdll_module;

    ntdll_module = GetModuleHandleW(L"ntdll.dll");
    if (ntdll_module != NULL) {
      function_pointer = (NexusHardwareWindowsNtQuerySystemInformationFunc)(void *)GetProcAddress(ntdll_module, "NtQuerySystemInformation");
    }

    function_resolved = TRUE;
  }

  return function_pointer;
}

static NError n_hardware_windows_system_process_information_snapshot(void **snapshot_buffer, ULONG *snapshot_buffer_length) {
  NexusHardwareWindowsNtQuerySystemInformationFunc nt_query_system_information;
  void                                            *buffer;
  ULONG                                            buffer_length;
  ULONG                                            return_length;
  NTSTATUS                                         status;

  if (snapshot_buffer == NULL || snapshot_buffer_length == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  nt_query_system_information = n_hardware_windows_nt_query_system_information_get();
  if (nt_query_system_information == NULL) {
    return NEXUS_ERROR_IO;
  }

  buffer_length = 0x10000;
  buffer        = malloc((size_t)buffer_length);
  if (buffer == NULL) {
    return NEXUS_ERROR_IO;
  }

  for (;;) {
    return_length = 0;
    status        = nt_query_system_information(NEXUS_HARDWARE_WINDOWS_SYSTEM_PROCESS_INFORMATION, buffer, buffer_length, &return_length);

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
      void *resized_buffer;

      if (return_length > buffer_length) {
        buffer_length = return_length;
      } else {
        buffer_length *= 2;
      }

      resized_buffer = realloc(buffer, (size_t)buffer_length);
      if (resized_buffer == NULL) {
        free(buffer);
        return NEXUS_ERROR_IO;
      }

      buffer = resized_buffer;
      continue;
    }

    if (status != STATUS_SUCCESS) {
      free(buffer);
      return NEXUS_ERROR_IO;
    }

    break;
  }

  *snapshot_buffer        = buffer;
  *snapshot_buffer_length = buffer_length;
  return NEXUS_ERROR_NONE;
}

static NError n_hardware_windows_current_process_information_get(NexusHardwareWindowsSystemProcessInformation **process_information,
                                                                 void *snapshot_buffer, ULONG snapshot_buffer_length) {
  DWORD current_process_id;
  byte *cursor;
  byte *snapshot_end;

  if (process_information == NULL || snapshot_buffer == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  current_process_id = GetCurrentProcessId();
  cursor             = (byte *)snapshot_buffer;
  snapshot_end       = cursor + snapshot_buffer_length;

  for (;;) {
    NexusHardwareWindowsSystemProcessInformation *process;

    if ((cursor + NEXUS_SIZEOF(NexusHardwareWindowsSystemProcessInformation)) > snapshot_end) {
      return NEXUS_ERROR_IO;
    }

    process = (NexusHardwareWindowsSystemProcessInformation *)cursor;
    if ((DWORD)(ULONG_PTR)process->unique_process_id == current_process_id) {
      *process_information = process;
      return NEXUS_ERROR_NONE;
    }

    if (process->next_entry_offset == 0) {
      break;
    }

    cursor += process->next_entry_offset;
  }

  return NEXUS_ERROR_IO;
}

static NError n_hardware_windows_current_thread_context_switches_get(uint64 *count) {
  void                                         *snapshot_buffer;
  ULONG                                         snapshot_buffer_length;
  NexusHardwareWindowsSystemProcessInformation *process_information;
  DWORD                                         current_thread_id;
  ULONG                                         thread_index;
  NError                                        snapshot_error;
  NError                                        process_error;

  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  snapshot_error = n_hardware_windows_system_process_information_snapshot(&snapshot_buffer, &snapshot_buffer_length);
  if (snapshot_error != NEXUS_ERROR_NONE) {
    return snapshot_error;
  }

  process_error = n_hardware_windows_current_process_information_get(&process_information, snapshot_buffer, snapshot_buffer_length);
  if (process_error != NEXUS_ERROR_NONE) {
    free(snapshot_buffer);
    return process_error;
  }

  current_thread_id = GetCurrentThreadId();
  for (thread_index = 0; thread_index < process_information->number_of_threads; thread_index++) {
    NexusHardwareWindowsSystemThreadInformation *thread_information;

    thread_information = &process_information->threads[thread_index];
    if ((DWORD)(ULONG_PTR)thread_information->client_id.unique_thread == current_thread_id) {
      *count = (uint64)thread_information->context_switch_count;
      free(snapshot_buffer);
      return NEXUS_ERROR_NONE;
    }
  }

  free(snapshot_buffer);
  return NEXUS_ERROR_IO;
}

static NError n_hardware_windows_current_process_hard_fault_count_get(uint64 *count) {
  void                                         *snapshot_buffer;
  ULONG                                         snapshot_buffer_length;
  NexusHardwareWindowsSystemProcessInformation *process_information;
  NError                                        snapshot_error;
  NError                                        process_error;

  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  snapshot_error = n_hardware_windows_system_process_information_snapshot(&snapshot_buffer, &snapshot_buffer_length);
  if (snapshot_error != NEXUS_ERROR_NONE) {
    return snapshot_error;
  }

  process_error = n_hardware_windows_current_process_information_get(&process_information, snapshot_buffer, snapshot_buffer_length);
  if (process_error != NEXUS_ERROR_NONE) {
    free(snapshot_buffer);
    return process_error;
  }

  *count = (uint64)process_information->hard_fault_count.QuadPart;
  free(snapshot_buffer);
  return NEXUS_ERROR_NONE;
}

static NError n_hardware_windows_process_page_fault_count_get(uint64 *count) {
  PROCESS_MEMORY_COUNTERS memory_counters;

  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (GetProcessMemoryInfo(GetCurrentProcess(), &memory_counters, (DWORD)NEXUS_SIZEOF(memory_counters)) == FALSE) {
    return NEXUS_ERROR_IO;
  }

  *count = (uint64)memory_counters.PageFaultCount;
  return NEXUS_ERROR_NONE;
}

#endif

NError nexus_hardware_voluntary_context_switches_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if NEXUS_PLATFORM_POSIX
  {
    struct rusage resource_usage;
    NError        read_error;

    read_error = n_hardware_rusage_get(&resource_usage);
    if (read_error != NEXUS_ERROR_NONE) {
      return read_error;
    }

    *count = (uint64)resource_usage.ru_nvcsw;
    return NEXUS_ERROR_NONE;
  }
#elif NEXUS_PLATFORM_WINDOWS
  /*
  GetThreadContext reads CPU register state, not switch counts. Windows does not expose a
  documented voluntary/involuntary split without ETW; NtQuerySystemInformation provides the
  current thread's cumulative context switch count instead.
  */
  return n_hardware_windows_current_thread_context_switches_get(count);
#else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_involuntary_context_switches_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if NEXUS_PLATFORM_POSIX
  {
    struct rusage resource_usage;
    NError        read_error;

    read_error = n_hardware_rusage_get(&resource_usage);
    if (read_error != NEXUS_ERROR_NONE) {
      return read_error;
    }

    *count = (uint64)resource_usage.ru_nivcsw;
    return NEXUS_ERROR_NONE;
  }
#elif NEXUS_PLATFORM_WINDOWS
  (void)count;
  /*
  Windows does not expose voluntary and involuntary context switch counters separately outside
  of ETW. Use nexus_hardware_voluntary_context_switches_get for the cumulative thread count.
  */
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_major_page_faults_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if NEXUS_PLATFORM_POSIX
  {
    struct rusage resource_usage;
    NError        read_error;

    read_error = n_hardware_rusage_get(&resource_usage);
    if (read_error != NEXUS_ERROR_NONE) {
      return read_error;
    }

    *count = (uint64)resource_usage.ru_majflt;
    return NEXUS_ERROR_NONE;
  }
#elif NEXUS_PLATFORM_WINDOWS
  return n_hardware_windows_current_process_hard_fault_count_get(count);
#else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_minor_page_faults_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if NEXUS_PLATFORM_POSIX
  {
    struct rusage resource_usage;
    NError        read_error;

    read_error = n_hardware_rusage_get(&resource_usage);
    if (read_error != NEXUS_ERROR_NONE) {
      return read_error;
    }

    *count = (uint64)resource_usage.ru_minflt;
    return NEXUS_ERROR_NONE;
  }
#elif NEXUS_PLATFORM_WINDOWS
  return n_hardware_windows_process_page_fault_count_get(count);
#else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

#if defined(NEXUS_PLATFORM_LINUX)

static NEXUS_THREAD_LOCAL int     s_hardware_linux_cycle_perf_fd           = -1;
static NEXUS_THREAD_LOCAL boolean s_hardware_linux_cycle_perf_failed       = FALSE;
static NEXUS_THREAD_LOCAL int     s_hardware_linux_cache_miss_perf_fd      = -1;
static NEXUS_THREAD_LOCAL boolean s_hardware_linux_cache_miss_perf_failed  = FALSE;
static NEXUS_THREAD_LOCAL int     s_hardware_linux_instruction_perf_fd     = -1;
static NEXUS_THREAD_LOCAL boolean s_hardware_linux_instruction_perf_failed = FALSE;
static NEXUS_THREAD_LOCAL uint32  s_hardware_linux_perf_counter_suspend_depth;

static NError n_hardware_linux_perf_event_open(uint32 type, uint64 config, int *descriptor, boolean *failed) {
  struct perf_event_attr event_attributes;
  int                    perf_event_fd;

  if (descriptor == NULL || failed == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (*descriptor >= 0) {
    return NEXUS_ERROR_NONE;
  }

  if (*failed != FALSE) {
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }

  nexus_memory_bytes_set(&event_attributes, 0, NEXUS_SIZEOF(event_attributes));

  event_attributes.type           = type;
  event_attributes.size           = (uint32)NEXUS_SIZEOF(event_attributes);
  event_attributes.config         = config;
  event_attributes.disabled       = s_hardware_linux_perf_counter_suspend_depth != 0 ? 1U : 0U;
  event_attributes.exclude_kernel = 1;
  event_attributes.exclude_hv     = 1;
  event_attributes.inherit        = 0;

  perf_event_fd = (int)syscall(SYS_perf_event_open, &event_attributes, (pid_t)0, -1, -1, 0UL);
  if (perf_event_fd < 0) {
    *failed = TRUE;

    if (errno == EPERM || errno == EACCES) {
      return NEXUS_ERROR_PERMISSION_DENIED;
    }

    return NEXUS_ERROR_IO;
  }

  *descriptor = perf_event_fd;

  return NEXUS_ERROR_NONE;
}

static NError n_hardware_linux_perf_event_read(int descriptor, uint64 *count) {
  ssize_t bytes_read;

  if (descriptor < 0 || count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  do {
    bytes_read = read(descriptor, count, NEXUS_SIZEOF(*count));
  } while (bytes_read < 0 && errno == EINTR);

  if (bytes_read != (ssize_t)NEXUS_SIZEOF(*count)) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
}

static NError n_hardware_linux_perf_events_enabled_set(boolean enabled) {
  unsigned long request;
  NError        error;

  request = enabled != FALSE ? PERF_EVENT_IOC_ENABLE : PERF_EVENT_IOC_DISABLE;
  error   = NEXUS_ERROR_NONE;

  if (s_hardware_linux_cycle_perf_fd >= 0 && ioctl(s_hardware_linux_cycle_perf_fd, request, 0) != 0) {
    error = NEXUS_ERROR_IO;
  }

  if (s_hardware_linux_instruction_perf_fd >= 0 && ioctl(s_hardware_linux_instruction_perf_fd, request, 0) != 0) {
    error = NEXUS_ERROR_IO;
  }

  if (s_hardware_linux_cache_miss_perf_fd >= 0 && ioctl(s_hardware_linux_cache_miss_perf_fd, request, 0) != 0) {
    error = NEXUS_ERROR_IO;
  }

  return error;
}

static NError n_hardware_linux_cache_miss_perf_event_try_open(uint64 config, uint32 type) {
  struct perf_event_attr event_attributes;
  int                    perf_event_fd;

  nexus_memory_bytes_set(&event_attributes, 0, sizeof(event_attributes));

  event_attributes.type           = type;
  event_attributes.size           = (uint32)sizeof(event_attributes);
  event_attributes.config         = config;
  event_attributes.disabled       = s_hardware_linux_perf_counter_suspend_depth != 0 ? 1U : 0U;
  event_attributes.exclude_kernel = 1;
  event_attributes.exclude_hv     = 1;
  event_attributes.inherit        = 0;

  perf_event_fd = (int)syscall(SYS_perf_event_open, &event_attributes, (pid_t)0, -1, -1, 0UL);
  if (perf_event_fd < 0) {
    if (errno == EPERM || errno == EACCES) {
      return NEXUS_ERROR_PERMISSION_DENIED;
    }

    return NEXUS_ERROR_IO;
  }

  s_hardware_linux_cache_miss_perf_fd = perf_event_fd;

  return NEXUS_ERROR_NONE;
}

static NError n_hardware_linux_cache_miss_perf_event_open(void) {
  NError open_error;

  if (s_hardware_linux_cache_miss_perf_fd >= 0) {
    return NEXUS_ERROR_NONE;
  }

  if (s_hardware_linux_cache_miss_perf_failed == TRUE) {
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }

  open_error = n_hardware_linux_cache_miss_perf_event_try_open(PERF_COUNT_HW_CACHE_MISSES, PERF_TYPE_HARDWARE);
  if (open_error == NEXUS_ERROR_NONE) {
    return NEXUS_ERROR_NONE;
  }

  if (open_error == NEXUS_ERROR_PERMISSION_DENIED) {
    s_hardware_linux_cache_miss_perf_failed = TRUE;
    return open_error;
  }

  open_error = n_hardware_linux_cache_miss_perf_event_try_open(
      PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), PERF_TYPE_HW_CACHE);
  if (open_error == NEXUS_ERROR_NONE) {
    return NEXUS_ERROR_NONE;
  }

  if (open_error == NEXUS_ERROR_PERMISSION_DENIED) {
    s_hardware_linux_cache_miss_perf_failed = TRUE;
    return open_error;
  }

  open_error = n_hardware_linux_cache_miss_perf_event_try_open(
      PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), PERF_TYPE_HW_CACHE);
  if (open_error == NEXUS_ERROR_NONE) {
    return NEXUS_ERROR_NONE;
  }

  s_hardware_linux_cache_miss_perf_failed = TRUE;
  if (open_error == NEXUS_ERROR_PERMISSION_DENIED) {
    return open_error;
  }

  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
}

static NError n_hardware_linux_cache_miss_perf_event_read(uint64 *count) {
  NError error;

  error = n_hardware_linux_cache_miss_perf_event_open();
  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  return n_hardware_linux_perf_event_read(s_hardware_linux_cache_miss_perf_fd, count);
}

static NError n_hardware_linux_instruction_perf_event_open(void) {
  return n_hardware_linux_perf_event_open(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, &s_hardware_linux_instruction_perf_fd,
                                          &s_hardware_linux_instruction_perf_failed);
}

static NError n_hardware_linux_instruction_perf_event_read(uint64 *count) {
  NError error;

  error = n_hardware_linux_instruction_perf_event_open();
  if (error != NEXUS_ERROR_NONE) {
    return error;
  }

  return n_hardware_linux_perf_event_read(s_hardware_linux_instruction_perf_fd, count);
}

#endif

NError nexus_hardware_cache_misses_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_LINUX)
  return n_hardware_linux_cache_miss_perf_event_read(count);
#else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_cpu_retired_instructions_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_LINUX)
  return n_hardware_linux_instruction_perf_event_read(count);
#else
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

uint32 nexus_hardware_floating_point_denormal_flush_push(void) {
#if (NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32)
  /*
  MXCSR bit 15 = FTZ (flush denormal results to zero).
  MXCSR bit 6  = DAZ (treat denormal operands as zero).
  Encapsulated here so callers never include xmmintrin/pmmintrin directly.
  */
#  if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  {
    unsigned int previous;

    previous = _mm_getcsr();
    _mm_setcsr(previous | 0x8040u);
    return (uint32)previous;
  }
#  else
  return 0u;
#  endif
#else
  return 0u;
#endif
}

void nexus_hardware_floating_point_denormal_flush_pop(uint32 previous_control) {
#if (NEXUS_ARCH == NEXUS_ARCH_X86_64 || NEXUS_ARCH == NEXUS_ARCH_X86_32)
#  if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  _mm_setcsr((unsigned int)previous_control);
#  else
  (void)previous_control;
#  endif
#else
  (void)previous_control;
#endif
}

NError nexus_hardware_cpu_cycles_get(uint64 *count) {
  if (count == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

#if defined(NEXUS_PLATFORM_LINUX)
  {
    NError error;

    error = n_hardware_linux_perf_event_open(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, &s_hardware_linux_cycle_perf_fd,
                                             &s_hardware_linux_cycle_perf_failed);
    if (error != NEXUS_ERROR_NONE) {
      return error;
    }

    return n_hardware_linux_perf_event_read(s_hardware_linux_cycle_perf_fd, count);
  }
#else
  return nexus_hardware_cpu_clock_cycles_get(count);
#endif
}

NError nexus_hardware_performance_counters_suspend(void) {
#if defined(NEXUS_PLATFORM_LINUX)
  NError error;

  if (s_hardware_linux_perf_counter_suspend_depth == UINT32_MAX_VAL) {
    return NEXUS_ERROR_CAPACITY;
  }

  s_hardware_linux_perf_counter_suspend_depth++;

  if (s_hardware_linux_perf_counter_suspend_depth != 1U) {
    return NEXUS_ERROR_NONE;
  }

  error = n_hardware_linux_perf_events_enabled_set(FALSE);
  if (error != NEXUS_ERROR_NONE) {
    s_hardware_linux_perf_counter_suspend_depth = 0;
    (void)n_hardware_linux_perf_events_enabled_set(TRUE);
    return error;
  }

  return NEXUS_ERROR_NONE;
#else
  return NEXUS_ERROR_NONE;
#endif
}

NError nexus_hardware_performance_counters_resume(void) {
#if defined(NEXUS_PLATFORM_LINUX)
  if (s_hardware_linux_perf_counter_suspend_depth == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  s_hardware_linux_perf_counter_suspend_depth--;

  if (s_hardware_linux_perf_counter_suspend_depth != 0) {
    return NEXUS_ERROR_NONE;
  }

  return n_hardware_linux_perf_events_enabled_set(TRUE);
#else
  return NEXUS_ERROR_NONE;
#endif
}
/* ---------------------------------------------------------------------------- */
/* EXTENSIBLE PERFORMANCE METRIC READERS                                        */
/* ---------------------------------------------------------------------------- */

struct NexusPerformanceMetricReader {
  NexusPerformanceMetricDescriptor descriptor;
  int                              perf_fd;
  boolean                          enabled;
};

#define N_PERF_INTERVAL_MS(ms) {((int64)(ms) * (int64)NEXUS_NANOSECONDS_PER_MILLISECOND), NTP_MILLISECOND}

static const NexusPerformanceMetricDescriptor n_hardware_performance_metrics[] = {
    {NPMK_CPU_CYCLES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "CPU Cycles", "PMU", N_PERF_INTERVAL_MS(10)},
    {NPMK_REFERENCE_CPU_CYCLES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Reference CPU Cycles", "PMU", N_PERF_INTERVAL_MS(10)},
    {NPMK_RETIRED_INSTRUCTIONS, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Instructions Retired", "PMU", N_PERF_INTERVAL_MS(10)},
    {NPMK_CACHE_REFERENCES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Cache References", "PMU", N_PERF_INTERVAL_MS(20)},
    {NPMK_CACHE_MISSES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Cache Misses", "PMU", N_PERF_INTERVAL_MS(20)},
    {NPMK_BRANCH_INSTRUCTIONS, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Branch Instructions", "PMU", N_PERF_INTERVAL_MS(20)},
    {NPMK_BRANCH_MISSES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Branch Mispredictions", "PMU", N_PERF_INTERVAL_MS(20)},
    {NPMK_BUS_CYCLES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Bus Cycles", "PMU", N_PERF_INTERVAL_MS(50)},
    {NPMK_STALLED_FRONTEND_CYCLES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Frontend Stalled Cycles", "PMU", N_PERF_INTERVAL_MS(25)},
    {NPMK_STALLED_BACKEND_CYCLES, NPMSK_PMU_HARDWARE, NPMVK_COUNTER, NPMUK_COUNT, "Backend Stalled Cycles", "PMU", N_PERF_INTERVAL_MS(25)},

    {NPMK_L1D_READS, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "L1D Reads", "PMU Cache", N_PERF_INTERVAL_MS(25)},
    {NPMK_L1D_READ_MISSES, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "L1D Read Misses", "PMU Cache", N_PERF_INTERVAL_MS(25)},
    {NPMK_L1I_READS, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "L1I Reads", "PMU Cache", N_PERF_INTERVAL_MS(50)},
    {NPMK_L1I_READ_MISSES, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "L1I Read Misses", "PMU Cache", N_PERF_INTERVAL_MS(50)},
    {NPMK_LLC_READS, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "LLC Reads", "PMU Cache", N_PERF_INTERVAL_MS(25)},
    {NPMK_LLC_READ_MISSES, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "LLC Read Misses", "PMU Cache", N_PERF_INTERVAL_MS(25)},
    {NPMK_DTLB_READS, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "dTLB Reads", "PMU Cache", N_PERF_INTERVAL_MS(50)},
    {NPMK_DTLB_READ_MISSES, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "dTLB Read Misses", "PMU Cache", N_PERF_INTERVAL_MS(50)},
    {NPMK_ITLB_READS, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "iTLB Reads", "PMU Cache", N_PERF_INTERVAL_MS(75)},
    {NPMK_ITLB_READ_MISSES, NPMSK_PMU_CACHE, NPMVK_COUNTER, NPMUK_COUNT, "iTLB Read Misses", "PMU Cache", N_PERF_INTERVAL_MS(75)},

    {NPMK_CPU_CLOCK_NANOSECONDS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_NANOSECONDS, "CPU Clock", "Kernel", N_PERF_INTERVAL_MS(20)},
    {NPMK_TASK_CLOCK_NANOSECONDS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_NANOSECONDS, "Task Clock", "Kernel", N_PERF_INTERVAL_MS(20)},
    {NPMK_PAGE_FAULTS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "Page Faults", "Kernel", N_PERF_INTERVAL_MS(50)},
    {NPMK_MINOR_PAGE_FAULTS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "Minor Page Faults", "Kernel", N_PERF_INTERVAL_MS(50)},
    {NPMK_MAJOR_PAGE_FAULTS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "Major Page Faults", "Kernel", N_PERF_INTERVAL_MS(50)},
    {NPMK_CONTEXT_SWITCHES, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "Context Switches", "Kernel", N_PERF_INTERVAL_MS(50)},
    {NPMK_CPU_MIGRATIONS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "CPU Migrations", "Kernel", N_PERF_INTERVAL_MS(50)},
    {NPMK_ALIGNMENT_FAULTS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "Alignment Faults", "Kernel", N_PERF_INTERVAL_MS(100)},
    {NPMK_EMULATION_FAULTS, NPMSK_KERNEL_SOFTWARE, NPMVK_COUNTER, NPMUK_COUNT, "Emulation Faults", "Kernel", N_PERF_INTERVAL_MS(100)},

    {NPMK_ARCHITECTURE_CLOCK_TICKS, NPMSK_ARCHITECTURE_CLOCK, NPMVK_COUNTER, NPMUK_TICKS, "Architecture Clock Ticks", "Architecture Clock",
     N_PERF_INTERVAL_MS(10)},
    {NPMK_PROCESS_USER_CPU_NANOSECONDS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_NANOSECONDS, "User CPU Time", "Resource Usage",
     N_PERF_INTERVAL_MS(50)},
    {NPMK_PROCESS_SYSTEM_CPU_NANOSECONDS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_NANOSECONDS, "System CPU Time", "Resource Usage",
     N_PERF_INTERVAL_MS(50)},
    {NPMK_PAGE_FAULTS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_COUNT, "Page Faults", "Resource Usage", N_PERF_INTERVAL_MS(50)},
    {NPMK_MINOR_PAGE_FAULTS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_COUNT, "Minor Page Faults", "Resource Usage", N_PERF_INTERVAL_MS(50)},
    {NPMK_MAJOR_PAGE_FAULTS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_COUNT, "Major Page Faults", "Resource Usage", N_PERF_INTERVAL_MS(50)},
    {NPMK_CONTEXT_SWITCHES, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_COUNT, "Context Switches", "Resource Usage", N_PERF_INTERVAL_MS(50)},
    {NPMK_PROCESS_MAX_RESIDENT_BYTES, NPMSK_RESOURCE_USAGE, NPMVK_GAUGE, NPMUK_BYTES, "Peak Resident Memory", "Resource Usage",
     N_PERF_INTERVAL_MS(100)},
    {NPMK_BLOCK_INPUT_OPERATIONS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_COUNT, "Block Input Operations", "Resource Usage",
     N_PERF_INTERVAL_MS(100)},
    {NPMK_BLOCK_OUTPUT_OPERATIONS, NPMSK_RESOURCE_USAGE, NPMVK_COUNTER, NPMUK_COUNT, "Block Output Operations", "Resource Usage",
     N_PERF_INTERVAL_MS(100)},

    {NPMK_DEBUG_ALLOCATIONS, NPMSK_MEMORY_DEBUGGER, NPMVK_COUNTER, NPMUK_COUNT, "Allocations", "Nexus Memory Debugger", N_PERF_INTERVAL_MS(25)},
    {NPMK_DEBUG_BYTES_ALLOCATED, NPMSK_MEMORY_DEBUGGER, NPMVK_COUNTER, NPMUK_BYTES, "Bytes Allocated", "Nexus Memory Debugger",
     N_PERF_INTERVAL_MS(25)},
    {NPMK_DEBUG_FREES, NPMSK_MEMORY_DEBUGGER, NPMVK_COUNTER, NPMUK_COUNT, "Frees", "Nexus Memory Debugger", N_PERF_INTERVAL_MS(25)},
    {NPMK_DEBUG_BYTES_FREED, NPMSK_MEMORY_DEBUGGER, NPMVK_COUNTER, NPMUK_BYTES, "Bytes Freed", "Nexus Memory Debugger", N_PERF_INTERVAL_MS(25)},
    {NPMK_DEBUG_LIVE_BYTES, NPMSK_MEMORY_DEBUGGER, NPMVK_GAUGE, NPMUK_BYTES, "Live Bytes", "Nexus Memory Debugger", N_PERF_INTERVAL_MS(50)},
    {NPMK_DEBUG_PEAK_LIVE_BYTES, NPMSK_MEMORY_DEBUGGER, NPMVK_GAUGE, NPMUK_BYTES, "Peak Live Bytes", "Nexus Memory Debugger", N_PERF_INTERVAL_MS(50)},
    {NPMK_DEBUG_LIVE_BLOCKS, NPMSK_MEMORY_DEBUGGER, NPMVK_GAUGE, NPMUK_COUNT, "Live Allocations", "Nexus Memory Debugger", N_PERF_INTERVAL_MS(50)},

    {NPMK_ALLOCATOR_IN_USE_BYTES, NPMSK_SYSTEM_ALLOCATOR, NPMVK_GAUGE, NPMUK_BYTES, "Allocator In-use Bytes", "System Allocator",
     N_PERF_INTERVAL_MS(100)},
    {NPMK_ALLOCATOR_ARENA_BYTES, NPMSK_SYSTEM_ALLOCATOR, NPMVK_GAUGE, NPMUK_BYTES, "Allocator Arena Bytes", "System Allocator",
     N_PERF_INTERVAL_MS(100)},
    {NPMK_ALLOCATOR_MMAP_BYTES, NPMSK_SYSTEM_ALLOCATOR, NPMVK_GAUGE, NPMUK_BYTES, "Allocator mmap Bytes", "System Allocator",
     N_PERF_INTERVAL_MS(100)}};

#undef N_PERF_INTERVAL_MS

const NexusPerformanceMetricDescriptor *nexus_hardware_performance_metrics_get(uint32 *out_metric_count) {
  if (out_metric_count == NULL) {
    return NULL;
  }

  *out_metric_count = (uint32)NEXUS_ARRAY_SIZE_ELEMENTS(n_hardware_performance_metrics);
  return n_hardware_performance_metrics;
}

#if defined(NEXUS_PLATFORM_LINUX)
static boolean n_hardware_performance_perf_config_get(NexusPerformanceMetricKind kind, uint32 *out_type, uint64 *out_config) {
  uint64 cache_config;

  switch (kind) {
  case NPMK_CPU_CYCLES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_CPU_CYCLES;
    return TRUE;
  case NPMK_REFERENCE_CPU_CYCLES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_REF_CPU_CYCLES;
    return TRUE;
  case NPMK_RETIRED_INSTRUCTIONS:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_INSTRUCTIONS;
    return TRUE;
  case NPMK_CACHE_REFERENCES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_CACHE_REFERENCES;
    return TRUE;
  case NPMK_CACHE_MISSES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_CACHE_MISSES;
    return TRUE;
  case NPMK_BRANCH_INSTRUCTIONS:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
    return TRUE;
  case NPMK_BRANCH_MISSES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_BRANCH_MISSES;
    return TRUE;
  case NPMK_BUS_CYCLES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_BUS_CYCLES;
    return TRUE;
  case NPMK_STALLED_FRONTEND_CYCLES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
    return TRUE;
  case NPMK_STALLED_BACKEND_CYCLES:
    *out_type   = PERF_TYPE_HARDWARE;
    *out_config = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
    return TRUE;

  case NPMK_CPU_CLOCK_NANOSECONDS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_CPU_CLOCK;
    return TRUE;
  case NPMK_TASK_CLOCK_NANOSECONDS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_TASK_CLOCK;
    return TRUE;
  case NPMK_PAGE_FAULTS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_PAGE_FAULTS;
    return TRUE;
  case NPMK_MINOR_PAGE_FAULTS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_PAGE_FAULTS_MIN;
    return TRUE;
  case NPMK_MAJOR_PAGE_FAULTS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_PAGE_FAULTS_MAJ;
    return TRUE;
  case NPMK_CONTEXT_SWITCHES:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_CONTEXT_SWITCHES;
    return TRUE;
  case NPMK_CPU_MIGRATIONS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_CPU_MIGRATIONS;
    return TRUE;
  case NPMK_ALIGNMENT_FAULTS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_ALIGNMENT_FAULTS;
    return TRUE;
  case NPMK_EMULATION_FAULTS:
    *out_type   = PERF_TYPE_SOFTWARE;
    *out_config = PERF_COUNT_SW_EMULATION_FAULTS;
    return TRUE;

  default:
    break;
  }

  cache_config = 0;
  switch (kind) {
  case NPMK_L1D_READS:
    cache_config = PERF_COUNT_HW_CACHE_L1D | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    break;
  case NPMK_L1D_READ_MISSES:
    cache_config = PERF_COUNT_HW_CACHE_L1D | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    break;
  case NPMK_L1I_READS:
    cache_config = PERF_COUNT_HW_CACHE_L1I | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    break;
  case NPMK_L1I_READ_MISSES:
    cache_config = PERF_COUNT_HW_CACHE_L1I | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    break;
  case NPMK_LLC_READS:
    cache_config = PERF_COUNT_HW_CACHE_LL | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    break;
  case NPMK_LLC_READ_MISSES:
    cache_config = PERF_COUNT_HW_CACHE_LL | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    break;
  case NPMK_DTLB_READS:
    cache_config = PERF_COUNT_HW_CACHE_DTLB | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    break;
  case NPMK_DTLB_READ_MISSES:
    cache_config = PERF_COUNT_HW_CACHE_DTLB | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    break;
  case NPMK_ITLB_READS:
    cache_config = PERF_COUNT_HW_CACHE_ITLB | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
    break;
  case NPMK_ITLB_READ_MISSES:
    cache_config = PERF_COUNT_HW_CACHE_ITLB | ((uint64)PERF_COUNT_HW_CACHE_OP_READ << 8) | ((uint64)PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
    break;
  default:
    return FALSE;
  }

  *out_type   = PERF_TYPE_HW_CACHE;
  *out_config = cache_config;
  return TRUE;
}

static NError n_hardware_performance_perf_open(NexusPerformanceMetricReader *reader) {
  struct perf_event_attr attr;
  uint32                 type;
  uint64                 config;
  int                    file_descriptor;

  if (n_hardware_performance_perf_config_get(reader->descriptor.kind, &type, &config) == FALSE) {
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }

  nexus_memory_bytes_clear(&attr, NEXUS_SIZEOF(attr));
  attr.type           = type;
  attr.size           = (uint32)NEXUS_SIZEOF(attr);
  attr.config         = config;
  attr.disabled       = 0;
  attr.exclude_kernel = 1;
  attr.exclude_hv     = 1;
  attr.inherit        = 0;
  attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

  file_descriptor = (int)syscall(SYS_perf_event_open, &attr, (pid_t)0, -1, -1, 0UL);
  if (file_descriptor < 0) {
    if (errno == EPERM || errno == EACCES)
      return NEXUS_ERROR_PERMISSION_DENIED;
    if (errno == ENOENT || errno == ENODEV || errno == EOPNOTSUPP || errno == EINVAL)
      return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
    return NEXUS_ERROR_IO;
  }

  reader->perf_fd = file_descriptor;
  return NEXUS_ERROR_NONE;
}

static NError n_hardware_performance_perf_read(NexusPerformanceMetricReader *reader, uint64 *out_value) {
  struct {
    uint64 value;
    uint64 time_enabled;
    uint64 time_running;
  } data;
  ssize_t bytes_read;

  do {
    bytes_read = read(reader->perf_fd, &data, NEXUS_SIZEOF(data));
  } while (bytes_read < 0 && errno == EINTR);

  if (bytes_read != (ssize_t)NEXUS_SIZEOF(data))
    return NEXUS_ERROR_IO;
  if (data.time_running == 0)
    return NEXUS_ERROR_IO;

  if (data.time_running != data.time_enabled) {
    long double scaled;
    scaled = (long double)data.value * (long double)data.time_enabled / (long double)data.time_running;
    if (scaled > (long double)UINT64_MAX_VAL)
      *out_value = UINT64_MAX_VAL;
    else
      *out_value = (uint64)scaled;
  } else {
    *out_value = data.value;
  }

  return NEXUS_ERROR_NONE;
}
#endif

static NError n_hardware_performance_rusage_read(NexusPerformanceMetricKind kind, uint64 *out_value) {
#if NEXUS_PLATFORM_POSIX
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != 0)
    return NEXUS_ERROR_IO;
  switch (kind) {
  case NPMK_PROCESS_USER_CPU_NANOSECONDS:
    *out_value =
        ((uint64)usage.ru_utime.tv_sec * NEXUS_NANOSECONDS_PER_SECOND) + ((uint64)usage.ru_utime.tv_usec * NEXUS_NANOSECONDS_PER_MICROSECOND);
    return NEXUS_ERROR_NONE;
  case NPMK_PROCESS_SYSTEM_CPU_NANOSECONDS:
    *out_value =
        ((uint64)usage.ru_stime.tv_sec * NEXUS_NANOSECONDS_PER_SECOND) + ((uint64)usage.ru_stime.tv_usec * NEXUS_NANOSECONDS_PER_MICROSECOND);
    return NEXUS_ERROR_NONE;
  case NPMK_PAGE_FAULTS:
    *out_value = (uint64)usage.ru_minflt + (uint64)usage.ru_majflt;
    return NEXUS_ERROR_NONE;
  case NPMK_MINOR_PAGE_FAULTS:
    *out_value = (uint64)usage.ru_minflt;
    return NEXUS_ERROR_NONE;
  case NPMK_MAJOR_PAGE_FAULTS:
    *out_value = (uint64)usage.ru_majflt;
    return NEXUS_ERROR_NONE;
  case NPMK_CONTEXT_SWITCHES:
    *out_value = (uint64)usage.ru_nvcsw + (uint64)usage.ru_nivcsw;
    return NEXUS_ERROR_NONE;
  case NPMK_PROCESS_MAX_RESIDENT_BYTES:
#  if defined(__APPLE__)
    *out_value = (uint64)usage.ru_maxrss;
#  else
    *out_value = (uint64)usage.ru_maxrss * 1024ULL;
#  endif
    return NEXUS_ERROR_NONE;
  case NPMK_BLOCK_INPUT_OPERATIONS:
    *out_value = (uint64)usage.ru_inblock;
    return NEXUS_ERROR_NONE;
  case NPMK_BLOCK_OUTPUT_OPERATIONS:
    *out_value = (uint64)usage.ru_oublock;
    return NEXUS_ERROR_NONE;
  default:
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }
#else
  (void)kind;
  (void)out_value;
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

static NError n_hardware_performance_debug_memory_read(NexusPerformanceMetricKind kind, uint64 *out_value) {
#if NEXUS_MEMORY_DEBUG_ENABLED
  NexusDebugMemSummary summary;
  nexus_debug_mem_summary_get(&summary);
  switch (kind) {
  case NPMK_DEBUG_ALLOCATIONS:
    *out_value = (uint64)summary.allocation_count;
    return NEXUS_ERROR_NONE;
  case NPMK_DEBUG_BYTES_ALLOCATED:
    *out_value = (uint64)summary.total_bytes_allocated;
    return NEXUS_ERROR_NONE;
  case NPMK_DEBUG_FREES:
    *out_value = (uint64)summary.free_count;
    return NEXUS_ERROR_NONE;
  case NPMK_DEBUG_BYTES_FREED:
    *out_value = (uint64)summary.total_bytes_freed;
    return NEXUS_ERROR_NONE;
  case NPMK_DEBUG_LIVE_BYTES:
    *out_value = (uint64)summary.live_bytes;
    return NEXUS_ERROR_NONE;
  case NPMK_DEBUG_PEAK_LIVE_BYTES:
    *out_value = (uint64)summary.peak_live_bytes;
    return NEXUS_ERROR_NONE;
  case NPMK_DEBUG_LIVE_BLOCKS:
    *out_value = (uint64)summary.live_block_count;
    return NEXUS_ERROR_NONE;
  default:
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }
#else
  (void)kind;
  (void)out_value;
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

static NError n_hardware_performance_allocator_read(NexusPerformanceMetricKind kind, uint64 *out_value) {
#if defined(__GLIBC__) && defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 33)
  struct mallinfo2 info;
  info = mallinfo2();
  switch (kind) {
  case NPMK_ALLOCATOR_IN_USE_BYTES:
    *out_value = (uint64)info.uordblks;
    return NEXUS_ERROR_NONE;
  case NPMK_ALLOCATOR_ARENA_BYTES:
    *out_value = (uint64)info.arena;
    return NEXUS_ERROR_NONE;
  case NPMK_ALLOCATOR_MMAP_BYTES:
    *out_value = (uint64)info.hblkhd;
    return NEXUS_ERROR_NONE;
  default:
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }
#else
  (void)kind;
  (void)out_value;
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_performance_metric_reader_create(const NexusPerformanceMetricDescriptor *descriptor,
                                                       NexusPerformanceMetricReader          **out_reader) {
  NexusPerformanceMetricReader *reader;
  NError                        error;
  uint64                        probe;

  if (descriptor == NULL || out_reader == NULL)
    return NEXUS_ERROR_INVALID_ARGUMENT;
  *out_reader = NULL;

  reader = (NexusPerformanceMetricReader *)malloc(NEXUS_SIZEOF(*reader));
  if (reader == NULL)
    return NEXUS_ERROR_CAPACITY;
  nexus_memory_bytes_clear(reader, NEXUS_SIZEOF(*reader));
  reader->descriptor = *descriptor;
  reader->perf_fd    = -1;
  reader->enabled    = TRUE;

  if (descriptor->source_kind == NPMSK_PMU_HARDWARE || descriptor->source_kind == NPMSK_PMU_CACHE ||
      descriptor->source_kind == NPMSK_KERNEL_SOFTWARE) {
#if defined(NEXUS_PLATFORM_LINUX)
    error = n_hardware_performance_perf_open(reader);
#else
    error = NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
  } else {
    error = nexus_hardware_performance_metric_reader_read(reader, &probe);
  }

  if (error != NEXUS_ERROR_NONE) {
#if defined(NEXUS_PLATFORM_LINUX)
    if (reader->perf_fd >= 0)
      close(reader->perf_fd);
#endif
    free(reader);
    return error;
  }

  *out_reader = reader;
  return NEXUS_ERROR_NONE;
}

NError nexus_hardware_performance_metric_reader_read(NexusPerformanceMetricReader *reader, uint64 *out_value) {
  if (reader == NULL || out_value == NULL)
    return NEXUS_ERROR_INVALID_ARGUMENT;

  switch (reader->descriptor.source_kind) {
  case NPMSK_PMU_HARDWARE:
  case NPMSK_PMU_CACHE:
  case NPMSK_KERNEL_SOFTWARE:
#if defined(NEXUS_PLATFORM_LINUX)
    return n_hardware_performance_perf_read(reader, out_value);
#else
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
  case NPMSK_ARCHITECTURE_CLOCK:
    return nexus_hardware_cpu_clock_cycles_get(out_value);
  case NPMSK_RESOURCE_USAGE:
    return n_hardware_performance_rusage_read(reader->descriptor.kind, out_value);
  case NPMSK_MEMORY_DEBUGGER:
    return n_hardware_performance_debug_memory_read(reader->descriptor.kind, out_value);
  case NPMSK_SYSTEM_ALLOCATOR:
    return n_hardware_performance_allocator_read(reader->descriptor.kind, out_value);
  default:
    return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
  }
}

NError nexus_hardware_performance_metric_reader_enabled_set(NexusPerformanceMetricReader *reader, boolean enabled) {
  if (reader == NULL)
    return NEXUS_ERROR_INVALID_ARGUMENT;

#if defined(NEXUS_PLATFORM_LINUX)
  if (reader->perf_fd >= 0) {
    unsigned long request;
    request = enabled != FALSE ? PERF_EVENT_IOC_ENABLE : PERF_EVENT_IOC_DISABLE;
    if (ioctl(reader->perf_fd, request, 0) != 0)
      return NEXUS_ERROR_IO;
  }
#endif

  reader->enabled = enabled;
  return NEXUS_ERROR_NONE;
}

void nexus_hardware_performance_metric_reader_destroy(NexusPerformanceMetricReader *reader) {
  if (reader == NULL)
    return;
#if defined(NEXUS_PLATFORM_LINUX)
  if (reader->perf_fd >= 0)
    close(reader->perf_fd);
#endif
  free(reader);
}

struct NexusPerformanceRawEventReader {
  int perf_fd;
};

NError nexus_hardware_performance_raw_event_reader_create(NexusPerformanceRawEventConfiguration configuration,
                                                          NexusPerformanceRawEventReader      **out_reader) {
#if defined(NEXUS_PLATFORM_LINUX)
  struct perf_event_attr          attr;
  NexusPerformanceRawEventReader *reader;
  int                             file_descriptor;

  if (out_reader == NULL)
    return NEXUS_ERROR_INVALID_ARGUMENT;
  *out_reader = NULL;

  nexus_memory_bytes_clear(&attr, NEXUS_SIZEOF(attr));
  attr.type           = configuration.source_type;
  attr.size           = (uint32)NEXUS_SIZEOF(attr);
  attr.config         = configuration.config;
  attr.config1        = configuration.config1;
  attr.config2        = configuration.config2;
  attr.exclude_kernel = configuration.exclude_kernel != FALSE ? 1U : 0U;
  attr.exclude_hv     = configuration.exclude_hypervisor != FALSE ? 1U : 0U;
  attr.read_format    = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

  file_descriptor = (int)syscall(SYS_perf_event_open, &attr, (pid_t)0, -1, -1, 0UL);
  if (file_descriptor < 0) {
    if (errno == EPERM || errno == EACCES)
      return NEXUS_ERROR_PERMISSION_DENIED;
    if (errno == ENOENT || errno == ENODEV || errno == EOPNOTSUPP || errno == EINVAL)
      return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
    return NEXUS_ERROR_IO;
  }

  reader = (NexusPerformanceRawEventReader *)malloc(NEXUS_SIZEOF(*reader));
  if (reader == NULL) {
    close(file_descriptor);
    return NEXUS_ERROR_CAPACITY;
  }
  reader->perf_fd = file_descriptor;
  *out_reader     = reader;
  return NEXUS_ERROR_NONE;
#else
  (void)configuration;
  if (out_reader != NULL)
    *out_reader = NULL;
  return out_reader == NULL ? NEXUS_ERROR_INVALID_ARGUMENT : NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_performance_raw_event_reader_read(NexusPerformanceRawEventReader *reader, uint64 *out_value) {
#if defined(NEXUS_PLATFORM_LINUX)
  NexusPerformanceMetricReader adapter;
  if (reader == NULL || out_value == NULL)
    return NEXUS_ERROR_INVALID_ARGUMENT;
  nexus_memory_bytes_clear(&adapter, NEXUS_SIZEOF(adapter));
  adapter.perf_fd = reader->perf_fd;
  return n_hardware_performance_perf_read(&adapter, out_value);
#else
  (void)reader;
  (void)out_value;
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

NError nexus_hardware_performance_raw_event_reader_enabled_set(NexusPerformanceRawEventReader *reader, boolean enabled) {
#if defined(NEXUS_PLATFORM_LINUX)
  unsigned long request;
  if (reader == NULL)
    return NEXUS_ERROR_INVALID_ARGUMENT;
  request = enabled != FALSE ? PERF_EVENT_IOC_ENABLE : PERF_EVENT_IOC_DISABLE;
  return ioctl(reader->perf_fd, request, 0) == 0 ? NEXUS_ERROR_NONE : NEXUS_ERROR_IO;
#else
  (void)reader;
  (void)enabled;
  return NEXUS_ERROR_UNSUPPORTED_ARCHITECTURE;
#endif
}

void nexus_hardware_performance_raw_event_reader_destroy(NexusPerformanceRawEventReader *reader) {
  if (reader == NULL)
    return;
#if defined(NEXUS_PLATFORM_LINUX)
  if (reader->perf_fd >= 0)
    close(reader->perf_fd);
#endif
  free(reader);
}
