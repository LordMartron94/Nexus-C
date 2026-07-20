#include "../nexus.h"

#if defined(__linux__)
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#endif

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

  if (GetProcessMemoryInfo(GetCurrentProcess(), &memory_counters, (DWORD)sizeof(memory_counters)) == FALSE) {
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

static int     s_hardware_linux_cache_miss_perf_fd     = -1;
static boolean s_hardware_linux_cache_miss_perf_failed = FALSE;

static NError n_hardware_linux_cache_miss_perf_event_try_open(uint64 config, uint32 type) {
  struct perf_event_attr event_attributes;
  int                    perf_event_fd;

  memset(&event_attributes, 0, sizeof(event_attributes));
  event_attributes.type           = type;
  event_attributes.size           = (uint32)sizeof(event_attributes);
  event_attributes.config         = config;
  event_attributes.disabled       = 0;
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
  ssize_t bytes_read;
  NError  open_error;

  open_error = n_hardware_linux_cache_miss_perf_event_open();
  if (open_error != NEXUS_ERROR_NONE) {
    return open_error;
  }

  bytes_read = read(s_hardware_linux_cache_miss_perf_fd, count, (size_t)sizeof(uint64));
  if (bytes_read != (ssize_t)sizeof(uint64)) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
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
