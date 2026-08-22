#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEXUS_MEMORY_DEBUG_INTERNAL       1
#define NEXUS_MEMORY_DEBUG_IMPLEMENTATION 1
#define NEXUS_EXIT_CRASH_IMPLEMENTATION   1
#include "../nexus.h"

/* MergeSource f_mem_debug.c (quelsolaar/MergeSource), adapted for Nexus. */

#define N_MEMORY_MAGIC_NUMBER        0xCF
#define N_MEMORY_INITIALIZATION      0xCD
#define N_MEMORY_FREED               0xCE
#define N_MEM_OFFSET_INVALID         ((size_t)-1)
#define NEXUS_MEMORY_LOG_MESSAGE_MAX 512

static NexusDebugMemLogCallback *n_mem_log_callback  = NULL;
static void                     *n_mem_log_user_data = NULL;

/*
 * Recursion suppression is per-thread. A process-global guard causes one thread's
 * memory-log callback to suppress unrelated memory logs from every other thread
 * and is itself a data race.
 */
static NEXUS_THREAD_LOCAL boolean n_mem_log_emitting = FALSE;

static boolean        nexus_memory_active        = TRUE;
static unsigned char *nexus_memory_stack_pointer = NULL;
static size_t         nexus_memory_stack_size    = 0;

typedef struct {
  size_t  size;
  void   *buf;
  char   *comment;
  boolean active;
} NexusMemAllocBuf;

typedef struct {
  unsigned int      line;
  char             *file;
  NexusMemAllocBuf *allocs;
  unsigned int      alloc_count;
  size_t            alloc_allocated;
  size_t            size;
  size_t            allocated;
  size_t            freed;
} NexusMemAllocLine;

static NexusMemAllocLine *n_alloc_lines      = NULL;
static unsigned int       n_alloc_line_count = 0;

#define NEXUS_MEMORY_FREE_POINTER_BUFFER_SIZE 1024

typedef struct {
  unsigned int alloc_line;
  char         alloc_file[256];
  unsigned int free_line;
  char         free_file[256];
  size_t       size;
  void        *pointer;
  boolean      realloc;
  boolean      active;
} NexusMemFreeBuf;

static NexusMemFreeBuf *n_freed_memory       = NULL;
static uint_large       n_freed_memory_count = 0;

/*
 * The lock is injected by Nexus' threading implementation immediately before
 * the first secondary thread is created. The lock must be allocation-free and
 * process-lifetime. User callbacks are always invoked after releasing it.
 */
static void *n_alloc_mutex                      = NULL;
static int (*n_alloc_mutex_lock)(void *mutex)   = NULL;
static int (*n_alloc_mutex_unlock)(void *mutex) = NULL;

static size_t     n_mem_stats_live_bytes            = 0;
static size_t     n_mem_stats_peak_live_bytes       = 0;
static uint32     n_mem_stats_live_block_count      = 0;
static uint32     n_mem_stats_peak_live_block_count = 0;
static uint_large n_mem_stats_total_bytes_allocated = 0;
static uint_large n_mem_stats_total_bytes_freed     = 0;
static uint_large n_mem_stats_allocation_count      = 0;
static uint_large n_mem_stats_free_count            = 0;
static size_t     n_mem_stats_largest_allocation    = 0;

typedef struct NexusMemMeasurementNode {
  size_t                          largest_allocation_bytes;
  struct NexusMemMeasurementNode *next;
} NexusMemMeasurementNode;

static NexusMemMeasurementNode *n_mem_measurements = NULL;

static void n_mem_lock(void) {
  if (n_alloc_mutex != NULL && n_alloc_mutex_lock != NULL)
    (void)n_alloc_mutex_lock(n_alloc_mutex);
}

static void n_mem_unlock(void) {
  if (n_alloc_mutex != NULL && n_alloc_mutex_unlock != NULL)
    (void)n_alloc_mutex_unlock(n_alloc_mutex);
}

static char *n_mem_string_duplicate(const char *text) {
  size_t length;
  char  *copy;

  if (text == NULL)
    return NULL;

  length = strlen(text) + 1;
  copy   = (char *)malloc(length);
  if (copy != NULL)
    memcpy(copy, text, length);
  return copy;
}

static void nexus_debug_mem_stats_on_alloc(size_t size) {
  NexusMemMeasurementNode *measurement;

  n_mem_stats_total_bytes_allocated += (uint_large)size;
  n_mem_stats_allocation_count++;
  n_mem_stats_live_bytes += size;
  n_mem_stats_live_block_count++;

  if (size > n_mem_stats_largest_allocation)
    n_mem_stats_largest_allocation = size;
  if (n_mem_stats_live_bytes > n_mem_stats_peak_live_bytes)
    n_mem_stats_peak_live_bytes = n_mem_stats_live_bytes;
  if (n_mem_stats_live_block_count > n_mem_stats_peak_live_block_count)
    n_mem_stats_peak_live_block_count = n_mem_stats_live_block_count;

  measurement = n_mem_measurements;
  while (measurement != NULL) {
    if (size > measurement->largest_allocation_bytes)
      measurement->largest_allocation_bytes = size;
    measurement = measurement->next;
  }
}

/*
 * Suspended debugging deliberately avoids guard storage and allocation-table
 * maintenance. Allocation measurements still need event totals, however, so
 * benchmarks can report allocator activity without enabling full validation.
 */
static void nexus_debug_mem_stats_on_untracked_alloc(size_t size) {
  NexusMemMeasurementNode *measurement;

  n_mem_stats_total_bytes_allocated += (uint_large)size;
  n_mem_stats_allocation_count++;

  if (size > n_mem_stats_largest_allocation)
    n_mem_stats_largest_allocation = size;

  measurement = n_mem_measurements;
  while (measurement != NULL) {
    if (size > measurement->largest_allocation_bytes)
      measurement->largest_allocation_bytes = size;
    measurement = measurement->next;
  }
}

static void nexus_debug_mem_stats_on_free(size_t size) {
  n_mem_stats_total_bytes_freed += (uint_large)size;
  n_mem_stats_free_count++;

  if (n_mem_stats_live_bytes >= size)
    n_mem_stats_live_bytes -= size;
  else
    n_mem_stats_live_bytes = 0;

  if (n_mem_stats_live_block_count > 0)
    n_mem_stats_live_block_count--;
}

static void nexus_debug_mem_stats_reset(void) {
  /* Live state remains live across a statistics reset. */
  n_mem_stats_peak_live_bytes       = n_mem_stats_live_bytes;
  n_mem_stats_peak_live_block_count = n_mem_stats_live_block_count;
  n_mem_stats_total_bytes_allocated = 0;
  n_mem_stats_total_bytes_freed     = 0;
  n_mem_stats_allocation_count      = 0;
  n_mem_stats_free_count            = 0;
  n_mem_stats_largest_allocation    = 0;
}

static boolean nexus_debug_mem_add_unlocked(void *pointer, size_t size, const char *file, unsigned int line);
static boolean nexus_debug_mem_remove_unlocked(unsigned char *buf, const char *file, unsigned int line, boolean was_realloc, boolean validate,
                                               size_t *out_size);
static boolean nexus_debug_mem_find_pointer_in_memory_unlocked(void *pointer);

static void nexus_debug_mem_log_emit(const char *message, const char *file, unsigned int line) {
  NexusDebugMemLogCallback *callback;
  void                     *user_data;
  boolean                   active;

  if (n_mem_log_emitting)
    return;

  n_mem_lock();
  callback  = n_mem_log_callback;
  user_data = n_mem_log_user_data;
  active    = nexus_memory_active;
  n_mem_unlock();

  if (callback == NULL || !active)
    return;

  /*
   * Invoke external code outside the allocator lock. This avoids lock-order
   * deadlocks when the callback enters Echo, I/O, or another synchronized
   * subsystem. Same-thread recursive memory logging is suppressed by TLS.
   * Different threads may invoke the callback concurrently.
   */
  n_mem_log_emitting = TRUE;
  callback(user_data, message, file, (uint32)line);
  n_mem_log_emitting = FALSE;
}

static void nexus_debug_mem_log_format(const char *file, unsigned int line, const char *format, ...) {
  char                    message[NEXUS_MEMORY_LOG_MESSAGE_MAX];
  va_list                 args;
  NexusStringFormatResult format_result;

  if (!nexus_debug_mem_active_get())
    return;

  va_start(args, format);
  format_result = nexus_strings_vstring_format_with_truncation(message, NEXUS_MEMORY_LOG_MESSAGE_MAX, format, args);
  va_end(args);
  if (!format_result.success && format[0] != '\0')
    return;
  nexus_debug_mem_log_emit(message, file, line);
}

void nexus_debug_mem_thread_safe_init(int (*lock)(void *mutex), int (*unlock)(void *mutex), void *mutex) {
  if (lock == NULL || unlock == NULL || mutex == NULL) {
    printf("Nexus Mem debugger error: thread-safe initialization requires non-NULL lock, unlock, and mutex values\n");
    NEXUS_MEMORY_CALL_ON_ERROR
    return;
  }

  /* First successful configuration owns synchronization for process lifetime. */
  if (n_alloc_mutex != NULL) {
    if (n_alloc_mutex == mutex && n_alloc_mutex_lock == lock && n_alloc_mutex_unlock == unlock)
      return;

    /*
     * Replacing a live allocator lock cannot be made safe without stopping every
     * allocating thread. Ignore later configurations rather than switching locks
     * underneath active allocator operations.
     */
    return;
  }

  n_alloc_mutex        = mutex;
  n_alloc_mutex_lock   = lock;
  n_alloc_mutex_unlock = unlock;
}

void nexus_debug_mem_stack_pointer_set(void *lowest_stack_pointer, size_t stack_size_in_bytes) {
  n_mem_lock();
  nexus_memory_stack_pointer = (unsigned char *)lowest_stack_pointer;
  nexus_memory_stack_size    = stack_size_in_bytes;
  n_mem_unlock();
}

boolean nexus_debug_mem_active_exchange(boolean active) {
  boolean previous;

  n_mem_lock();
  previous            = nexus_memory_active;
  nexus_memory_active = active;
  n_mem_unlock();

  return previous;
}

void nexus_debug_mem_active(boolean active) {
  (void)nexus_debug_mem_active_exchange(active);
}

boolean nexus_debug_mem_active_get(void) {
  boolean active;

  n_mem_lock();
  active = nexus_memory_active;
  n_mem_unlock();

  return active;
}

void nexus_debug_mem_log_callback_set(NexusDebugMemLogCallback *callback, void *user_data) {
  n_mem_lock();
  n_mem_log_callback  = callback;
  n_mem_log_user_data = callback != NULL ? user_data : NULL;
  n_mem_unlock();
}

boolean nexus_debug_mem_log_callback_installed_get(void) {
  boolean installed;

  n_mem_lock();
  installed = n_mem_log_callback != NULL ? TRUE : FALSE;
  n_mem_unlock();

  return installed;
}

void nexus_debug_mem_reset(void) {
  unsigned int i;

  n_mem_lock();

  if (nexus_memory_active) {
    for (i = 0; i < n_alloc_line_count; i++) {
      /* size is current live bytes and must not be cleared while blocks are live. */
      n_alloc_lines[i].allocated = 0;
      n_alloc_lines[i].freed     = 0;
    }
  }

  nexus_debug_mem_stats_reset();
  n_mem_unlock();
}

static void nexus_debug_mem_summary_copy_unlocked(NexusDebugMemSummary *summary) {
  summary->live_bytes               = n_mem_stats_live_bytes;
  summary->peak_live_bytes          = n_mem_stats_peak_live_bytes;
  summary->live_block_count         = n_mem_stats_live_block_count;
  summary->peak_live_block_count    = n_mem_stats_peak_live_block_count;
  summary->total_bytes_allocated    = n_mem_stats_total_bytes_allocated;
  summary->total_bytes_freed        = n_mem_stats_total_bytes_freed;
  summary->allocation_count         = n_mem_stats_allocation_count;
  summary->free_count               = n_mem_stats_free_count;
  summary->call_site_count          = n_alloc_line_count;
  summary->largest_allocation_bytes = n_mem_stats_largest_allocation;
}

void nexus_debug_mem_measurement_begin(NexusDebugMemMeasurementContext *context) {
  NexusMemMeasurementNode *node;
  NexusDebugMemSummary     summary;

  if (context == NULL)
    return;

  n_mem_lock();

  nexus_debug_mem_summary_copy_unlocked(&summary);
  context->baseline_allocation_count         = summary.allocation_count;
  context->baseline_total_bytes_allocated    = summary.total_bytes_allocated;
  context->measurement_state                 = NULL;
  context->interval_largest_allocation_bytes = 0;

  node = (NexusMemMeasurementNode *)malloc(sizeof(*node));
  if (node != NULL) {
    node->largest_allocation_bytes = 0;
    node->next                     = n_mem_measurements;
    n_mem_measurements             = node;
    context->measurement_state     = node;
  }

  n_mem_unlock();
}

void nexus_debug_mem_measurement_end(NexusDebugMemMeasurementContext *context, NexusDebugMemMeasurement *measurement) {
  NexusDebugMemSummary     summary;
  NexusMemMeasurementNode *node;
  NexusMemMeasurementNode *previous;
  NexusMemMeasurementNode *target;
  size_t                   largest;

  if (context == NULL || measurement == NULL)
    return;

  n_mem_lock();

  nexus_debug_mem_summary_copy_unlocked(&summary);
  largest  = context->interval_largest_allocation_bytes;
  previous = NULL;
  node     = n_mem_measurements;
  target   = (NexusMemMeasurementNode *)context->measurement_state;

  while (node != NULL) {
    if (node == target) {
      largest = node->largest_allocation_bytes;
      if (previous == NULL)
        n_mem_measurements = node->next;
      else
        previous->next = node->next;
      free(node);
      break;
    }
    previous = node;
    node     = node->next;
  }

  measurement->allocation_count =
      summary.allocation_count >= context->baseline_allocation_count ? summary.allocation_count - context->baseline_allocation_count : 0;
  measurement->total_bytes_allocated    = summary.total_bytes_allocated >= context->baseline_total_bytes_allocated
                                              ? summary.total_bytes_allocated - context->baseline_total_bytes_allocated
                                              : 0;
  measurement->largest_allocation_bytes = largest;
  context->measurement_state            = NULL;

  n_mem_unlock();
}

boolean nexus_debug_mem_check_bounds(void) {
  boolean        output;
  size_t         size;
  unsigned char *buf;
  size_t         i;
  size_t         j;
  size_t         k;

  output = FALSE;

  if (!nexus_debug_mem_active_get())
    return FALSE;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return FALSE;
  }

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (!n_alloc_lines[i].allocs[j].active)
        continue;

      buf  = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
      size = n_alloc_lines[i].allocs[j].size;

      for (k = 0; k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING; k++) {
        if (buf[size + k] != N_MEMORY_MAGIC_NUMBER)
          break;
      }

      if (k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING) {
        if (n_alloc_lines[i].allocs[j].comment == NULL)
          printf("Nexus Mem debugger error: memory overrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line, n_alloc_lines[i].file);
        else
          printf("Nexus Mem debugger error: memory overrun of allocation made on line %u in file %s /* %s */\n", n_alloc_lines[i].line,
                 n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment);
        NEXUS_MEMORY_CALL_ON_ERROR
        output = TRUE;
      }

      buf = (unsigned char *)n_alloc_lines[i].allocs[j].buf - NEXUS_MEMORY_PRE_PADDING;
      for (k = 0; k < NEXUS_MEMORY_PRE_PADDING; k++) {
        if (buf[k] != N_MEMORY_MAGIC_NUMBER)
          break;
      }

      if (k < NEXUS_MEMORY_PRE_PADDING) {
        if (n_alloc_lines[i].allocs[j].comment == NULL)
          printf("Nexus Mem debugger error: memory underrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line,
                 n_alloc_lines[i].file);
        else
          printf("Nexus Mem debugger error: memory underrun of allocation made on line %u in file %s /* %s */\n", n_alloc_lines[i].line,
                 n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment);
        NEXUS_MEMORY_CALL_ON_ERROR
        output = TRUE;
      }
    }
  }

#ifdef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
  if (n_freed_memory != NULL) {
    for (i = 0; i < NEXUS_MEMORY_FREE_POINTER_BUFFER_SIZE; i++) {
      if (!n_freed_memory[i].active || n_freed_memory[i].pointer == NULL || n_freed_memory[i].size == 0)
        continue;

      buf  = (unsigned char *)n_freed_memory[i].pointer;
      size = n_freed_memory[i].size;

      for (k = 0; k < size && buf[k] == N_MEMORY_FREED; k++)
        ;

      if (k < size) {
        if (n_freed_memory[i].realloc)
          printf("Nexus Mem debugger error: pointer reallocated on line %u in file %s and freed on line %u in file %s was written to %u bytes into "
                 "the allocation after being freed\n",
                 n_freed_memory[i].alloc_line, n_freed_memory[i].alloc_file, n_freed_memory[i].free_line, n_freed_memory[i].free_file,
                 (unsigned int)k);
        else
          printf("Nexus Mem debugger error: pointer allocated on line %u in file %s and freed on line %u in file %s was written to %u bytes into "
                 "the allocation after being freed\n",
                 n_freed_memory[i].alloc_line, n_freed_memory[i].alloc_file, n_freed_memory[i].free_line, n_freed_memory[i].free_file,
                 (unsigned int)k);
        NEXUS_MEMORY_CALL_ON_ERROR
        output = TRUE;
      }
    }
  }
#endif

  n_mem_unlock();
  return output;
}

static boolean nexus_debug_mem_add_unlocked(void *pointer, size_t size, const char *file, unsigned int line) {
  unsigned int       i;
  unsigned int       j;
  unsigned char     *pre;
  NexusMemAllocBuf  *new_allocs;
  NexusMemAllocLine *new_lines;
  char              *file_copy;

  pre = (unsigned char *)pointer - NEXUS_MEMORY_PRE_PADDING;
  for (i = 0; i < NEXUS_MEMORY_PRE_PADDING; i++)
    pre[i] = N_MEMORY_MAGIC_NUMBER;

  for (i = 0; i < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING; i++)
    ((unsigned char *)pointer)[size + i] = N_MEMORY_MAGIC_NUMBER;

  for (i = 0; i < n_alloc_line_count; i++) {
    if (line == n_alloc_lines[i].line && strcmp(file, n_alloc_lines[i].file) == 0)
      break;
  }

  if (i < n_alloc_line_count) {
    if (n_alloc_lines[i].alloc_allocated == n_alloc_lines[i].alloc_count) {
      size_t new_capacity;

      new_capacity = n_alloc_lines[i].alloc_allocated + 1024;
      if (new_capacity < n_alloc_lines[i].alloc_allocated)
        return FALSE;

      new_allocs = (NexusMemAllocBuf *)realloc(n_alloc_lines[i].allocs, sizeof(*n_alloc_lines[i].allocs) * new_capacity);
      if (new_allocs == NULL) {
        printf("Nexus Mem debugger error: realloc returns NULL when growing allocation table at line %u in file %s\n", line, file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return FALSE;
      }

      n_alloc_lines[i].allocs          = new_allocs;
      n_alloc_lines[i].alloc_allocated = new_capacity;
    }
  } else {
    if (i % 1024 == 0) {
      size_t new_site_capacity;

      new_site_capacity = (size_t)i + (size_t)1024;
      if (new_site_capacity < (size_t)i || new_site_capacity > ((size_t)-1) / sizeof(*n_alloc_lines))
        return FALSE;

      new_lines = (NexusMemAllocLine *)realloc(n_alloc_lines, sizeof(*n_alloc_lines) * new_site_capacity);
      if (new_lines == NULL) {
        printf("Nexus Mem debugger error: realloc returns NULL when growing allocation-site table at line %u in file %s\n", line, file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return FALSE;
      }
      n_alloc_lines = new_lines;
    }

    file_copy = n_mem_string_duplicate(file);
    if (file_copy == NULL)
      return FALSE;

    n_alloc_lines[i].line            = line;
    n_alloc_lines[i].file            = file_copy;
    n_alloc_lines[i].alloc_allocated = 256;
    n_alloc_lines[i].alloc_count     = 0;
    n_alloc_lines[i].size            = 0;
    n_alloc_lines[i].allocated       = 0;
    n_alloc_lines[i].freed           = 0;
    n_alloc_lines[i].allocs          = (NexusMemAllocBuf *)malloc(sizeof(*n_alloc_lines[i].allocs) * n_alloc_lines[i].alloc_allocated);

    if (n_alloc_lines[i].allocs == NULL) {
      free(file_copy);
      n_alloc_lines[i].file = NULL;
      return FALSE;
    }

    n_alloc_line_count++;
  }

  j                                  = n_alloc_lines[i].alloc_count;
  n_alloc_lines[i].allocs[j].size    = size;
  n_alloc_lines[i].allocs[j].buf     = pointer;
  n_alloc_lines[i].allocs[j].comment = NULL;
  n_alloc_lines[i].allocs[j].active  = TRUE;
  n_alloc_lines[i].alloc_count++;
  n_alloc_lines[i].size += size;
  n_alloc_lines[i].allocated++;
  nexus_debug_mem_stats_on_alloc(size);

  return TRUE;
}

/*
Caller must hold n_alloc_mutex when thread-safe init is configured.
*/
static boolean nexus_debug_mem_lookup_exact_unlocked(const void *buf) {
  unsigned int i;
  unsigned int j;

  if (buf == NULL)
    return FALSE;
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].buf == buf)
        return TRUE;
    }
  }
  return FALSE;
}

void *nexus_debug_mem_malloc(size_t size, char *file, unsigned int line) {
  unsigned char *raw;
  unsigned char *pointer;
  boolean        active;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  if (nexus_debug_mem_active_get())
    (void)nexus_debug_mem_check_bounds();
#endif

  n_mem_lock();
  active = nexus_memory_active;

  if (!active) {
    n_mem_unlock();

    pointer = (unsigned char *)malloc(size);

    if (pointer == NULL)
      return NULL;

    n_mem_lock();
    nexus_debug_mem_stats_on_untracked_alloc(size);
    n_mem_unlock();
    return pointer;
  }

  if (size == 0) {
    printf("Nexus Mem debugger warning: malloc size zero in file %s line %u\n", file, line);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  if (size > (size_t)-1 - NEXUS_MEMORY_OVER_ALLOC) {
    printf("Nexus Mem debugger error: malloc size overflow in file %s line %u\n", file, line);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  raw = (unsigned char *)malloc(size + NEXUS_MEMORY_OVER_ALLOC);
  if (raw == NULL) {
#ifdef NEXUS_MEMORY_NULL_ALLOCATION_ERROR
    printf("Nexus Mem debugger warning: malloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line,
           file);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
#else
    n_mem_unlock();
#endif
    return NULL;
  }

  pointer = raw + NEXUS_MEMORY_PRE_PADDING;
  memset(pointer, N_MEMORY_INITIALIZATION, size);

  if (!nexus_debug_mem_add_unlocked(pointer, size, file, line)) {
    free(raw);
    n_mem_unlock();
    return NULL;
  }

  n_mem_unlock();
  nexus_debug_mem_log_format(file, line, "malloc %u bytes at pointer %p at %s line %u", (unsigned int)size, (void *)pointer, file, line);
  return pointer;
}

void *nexus_debug_mem_calloc(size_t num, size_t size, char *file, unsigned int line) {
  unsigned char *raw;
  unsigned char *pointer;
  size_t         total;
  boolean        active;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  if (nexus_debug_mem_active_get())
    (void)nexus_debug_mem_check_bounds();
#endif

  active = nexus_debug_mem_active_get();
  if (!active) {
    pointer = (unsigned char *)calloc(num, size);
    if (pointer == NULL)
      return NULL;

    if (num == 0 || size <= (size_t)-1 / num) {
      n_mem_lock();
      nexus_debug_mem_stats_on_untracked_alloc(num * size);
      n_mem_unlock();
    }
    return pointer;
  }

  if (num == 0 || size == 0) {
    printf("Nexus Mem debugger warning: calloc size zero in file %s line %u\n", file, line);
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  if (num > (size_t)-1 / size) {
    printf("Nexus Mem debugger error: calloc multiplication overflow in file %s line %u\n", file, line);
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  total = num * size;

  n_mem_lock();

  if (!nexus_memory_active) {
    n_mem_unlock();

    pointer = (unsigned char *)calloc(num, size);
    if (pointer == NULL)
      return NULL;

    n_mem_lock();
    nexus_debug_mem_stats_on_untracked_alloc(total);
    n_mem_unlock();
    return pointer;
  }

  if (total > (size_t)-1 - NEXUS_MEMORY_OVER_ALLOC) {
    printf("Nexus Mem debugger error: calloc size overflow in file %s line %u\n", file, line);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  raw = (unsigned char *)malloc(total + NEXUS_MEMORY_OVER_ALLOC);
  if (raw == NULL) {
#ifdef NEXUS_MEMORY_NULL_ALLOCATION_ERROR
    printf("Nexus Mem debugger warning: calloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)total, line,
           file);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
#else
    n_mem_unlock();
#endif
    return NULL;
  }

  pointer = raw + NEXUS_MEMORY_PRE_PADDING;
  memset(pointer, 0, total);

  if (!nexus_debug_mem_add_unlocked(pointer, total, file, line)) {
    free(raw);
    n_mem_unlock();
    return NULL;
  }

  n_mem_unlock();
  nexus_debug_mem_log_format(file, line, "calloc %u bytes at pointer %p at %s line %u", (unsigned int)total, (void *)pointer, file, line);
  return pointer;
}

static boolean nexus_debug_mem_remove_unlocked(unsigned char *buf, const char *file, unsigned int line, boolean was_realloc, boolean validate,
                                               size_t *out_size) {
  unsigned int   i;
  unsigned int   j;
  unsigned int   k;
  size_t         size;
  unsigned char *raw;

#if !defined(NEXUS_MEMORY_DOUBLE_FREE_CHECK) && !defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
  (void)file;
  (void)line;
  (void)was_realloc;
#endif

  if (out_size != NULL)
    *out_size = 0;

  if (buf == NULL)
    return TRUE;

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      unsigned char *alloc_start;
      unsigned char *alloc_end;

      alloc_start = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
      alloc_end   = alloc_start + n_alloc_lines[i].allocs[j].size;

      if (alloc_start == buf) {
#if defined(NEXUS_MEMORY_DOUBLE_FREE_CHECK) || defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
        NexusMemFreeBuf *free_record;
#endif

        size = n_alloc_lines[i].allocs[j].size;
        raw  = buf - NEXUS_MEMORY_PRE_PADDING;

        if (validate) {
          for (k = 0; k < NEXUS_MEMORY_PRE_PADDING; k++) {
            if (raw[k] != N_MEMORY_MAGIC_NUMBER)
              break;
          }
          if (k < NEXUS_MEMORY_PRE_PADDING) {
            printf("Nexus Mem debugger error: buffer underrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line,
                   n_alloc_lines[i].file);
            NEXUS_MEMORY_CALL_ON_ERROR
          }

          for (k = 0; k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING; k++) {
            if (buf[size + k] != N_MEMORY_MAGIC_NUMBER)
              break;
          }
          if (k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING) {
            printf("Nexus Mem debugger error: buffer overrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line,
                   n_alloc_lines[i].file);
            NEXUS_MEMORY_CALL_ON_ERROR
          }
        }

#if defined(NEXUS_MEMORY_DOUBLE_FREE_CHECK) || defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
        if (validate && n_freed_memory == NULL) {
          n_freed_memory = (NexusMemFreeBuf *)calloc(NEXUS_MEMORY_FREE_POINTER_BUFFER_SIZE, sizeof(*n_freed_memory));
          if (n_freed_memory == NULL) {
            printf("Nexus Mem debugger error: failed to allocate freed-pointer history\n");
            NEXUS_MEMORY_CALL_ON_ERROR
          }
        }

        free_record = NULL;
        if (validate && n_freed_memory != NULL) {
          size_t record_index;

          record_index = (size_t)(n_freed_memory_count % NEXUS_MEMORY_FREE_POINTER_BUFFER_SIZE);
          free_record  = &n_freed_memory[record_index];

#  ifdef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
          if (free_record->active && free_record->pointer != NULL)
            free((unsigned char *)free_record->pointer - NEXUS_MEMORY_PRE_PADDING);
#  endif

          memset(free_record, 0, sizeof(*free_record));
          free_record->alloc_line = n_alloc_lines[i].line;
          strncpy(free_record->alloc_file, n_alloc_lines[i].file, sizeof(free_record->alloc_file) - 1);
          free_record->alloc_file[sizeof(free_record->alloc_file) - 1] = '\0';
          free_record->free_line                                       = line;
          strncpy(free_record->free_file, file, sizeof(free_record->free_file) - 1);
          free_record->free_file[sizeof(free_record->free_file) - 1] = '\0';
          free_record->size                                          = size;
          free_record->pointer                                       = buf;
          free_record->realloc                                       = was_realloc;
          free_record->active                                        = TRUE;
          n_freed_memory_count++;
        }
#endif

        if (validate)
          memset(raw, N_MEMORY_FREED, size + NEXUS_MEMORY_OVER_ALLOC);

        if (n_alloc_lines[i].allocs[j].comment != NULL)
          free(n_alloc_lines[i].allocs[j].comment);

        if (n_alloc_lines[i].size >= size)
          n_alloc_lines[i].size -= size;
        else
          n_alloc_lines[i].size = 0;

        n_alloc_lines[i].freed++;
        nexus_debug_mem_stats_on_free(size);

        n_alloc_lines[i].allocs[j] = n_alloc_lines[i].allocs[n_alloc_lines[i].alloc_count - 1];
        n_alloc_lines[i].alloc_count--;

        if (out_size != NULL)
          *out_size = size;

#if !defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
        free(raw);
#else
        if (!validate)
          free(raw);
#endif
        return TRUE;
      }

      if (buf > alloc_start && buf < alloc_end) {
        printf("Nexus Mem debugger error: trying to free pointer %p that is not at the start (%u bytes into) allocation made on line %u in file %s\n",
               (void *)buf, (unsigned int)(buf - alloc_start), n_alloc_lines[i].line, n_alloc_lines[i].file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return FALSE;
      }
    }
  }

#if defined(NEXUS_MEMORY_DOUBLE_FREE_CHECK) || defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
  if (validate && n_freed_memory != NULL) {
    for (i = 0; i < NEXUS_MEMORY_FREE_POINTER_BUFFER_SIZE; i++) {
      if (!n_freed_memory[i].active || buf != n_freed_memory[i].pointer)
        continue;

      if (n_freed_memory[i].realloc)
        printf("Nexus Mem debugger error: pointer %p was freed twice; freed on line %u in %s, reallocated (%u bytes) on line %u in file %s\n",
               n_freed_memory[i].pointer, n_freed_memory[i].free_line, n_freed_memory[i].free_file, (unsigned int)n_freed_memory[i].size,
               n_freed_memory[i].alloc_line, n_freed_memory[i].alloc_file);
      else
        printf("Nexus Mem debugger error: pointer %p was freed twice; freed on line %u in %s, allocated (%u bytes) on line %u in file %s\n",
               n_freed_memory[i].pointer, n_freed_memory[i].free_line, n_freed_memory[i].free_file, (unsigned int)n_freed_memory[i].size,
               n_freed_memory[i].alloc_line, n_freed_memory[i].alloc_file);
      NEXUS_MEMORY_CALL_ON_ERROR
      return FALSE;
    }
  }
#endif

  if (validate && nexus_memory_stack_size != 0 && buf >= nexus_memory_stack_pointer && buf < nexus_memory_stack_pointer + nexus_memory_stack_size) {
    printf("Nexus Mem debugger error: trying to free pointer %p that points into the configured stack range\n", (void *)buf);
    NEXUS_MEMORY_CALL_ON_ERROR
    return FALSE;
  }

  /* Foreign/untracked libc allocations remain legal. */
  free(buf);
  return TRUE;
}

void nexus_debug_mem_free(void *buf, char *file, unsigned int line) {
  size_t  size;
  boolean removed;
  boolean validate;

  if (buf == NULL)
    return;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  if (nexus_debug_mem_active_get())
    (void)nexus_debug_mem_check_bounds();
#endif

  size = 0;
  n_mem_lock();

  validate = nexus_memory_active;

  if (!validate && n_alloc_line_count == 0) {
    n_mem_unlock();
    free(buf);
    return;
  }

  removed = nexus_debug_mem_remove_unlocked((unsigned char *)buf, file, line, FALSE, validate, &size);
  n_mem_unlock();

  if (removed && size != 0)
    nexus_debug_mem_log_format(file, line, "free %u bytes at pointer %p at %s line %u", (unsigned int)size, buf, file, line);
}

boolean nexus_debug_mem_comment(void *buf, char *comment) {
  unsigned int i;
  unsigned int j;
  char        *copy;

  if (!nexus_debug_mem_active_get())
    return FALSE;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return FALSE;
  }

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].buf == buf) {
        copy = n_mem_string_duplicate(comment);
        if (comment != NULL && copy == NULL) {
          n_mem_unlock();
          return FALSE;
        }

        if (n_alloc_lines[i].allocs[j].comment != NULL)
          free(n_alloc_lines[i].allocs[j].comment);
        n_alloc_lines[i].allocs[j].comment = copy;
        n_mem_unlock();
        return TRUE;
      }
    }
  }

  n_mem_unlock();
  return FALSE;
}

void *nexus_debug_mem_realloc(void *pointer, size_t size, char *file, unsigned int line) {
  unsigned int   i;
  unsigned int   j;
  unsigned char *raw;
  unsigned char *new_pointer;
  size_t         old_size;
  size_t         move;
  char          *comment_copy;
  boolean        active;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  if (nexus_debug_mem_active_get())
    (void)nexus_debug_mem_check_bounds();
#endif

  active = nexus_debug_mem_active_get();

  if (pointer == NULL) {
#ifdef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
    if (active)
      printf("Nexus Mem debugger warning: realloc called with NULL in %s line %u\n", file, line);
#endif
    return nexus_debug_mem_malloc(size, file, line);
  }

  if (size == 0) {
#ifdef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
    if (active)
      printf("Nexus Mem debugger warning: realloc size zero in file %s line %u; treating as free\n", file, line);
#endif
    nexus_debug_mem_free(pointer, file, line);
    return NULL;
  }

  n_mem_lock();

  if (!nexus_memory_active) {
    if (!nexus_debug_mem_lookup_exact_unlocked(pointer)) {
      n_mem_unlock();

      new_pointer = (unsigned char *)realloc(pointer, size);

      if (new_pointer == NULL)
        return NULL;

      n_mem_lock();
      nexus_debug_mem_stats_on_untracked_alloc(size);
      n_mem_unlock();
      return new_pointer;
    }

    for (i = 0; i < n_alloc_line_count; i++) {
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
        if (n_alloc_lines[i].allocs[j].buf == pointer)
          break;
      }
      if (j < n_alloc_lines[i].alloc_count)
        break;
    }

    old_size    = n_alloc_lines[i].allocs[j].size;
    new_pointer = (unsigned char *)malloc(size);
    if (new_pointer == NULL) {
      n_mem_unlock();
      return NULL;
    }

    move = old_size < size ? old_size : size;
    if (move != 0)
      memcpy(new_pointer, pointer, move);
    if (move < size)
      memset(new_pointer + move, 0, size - move);

    (void)nexus_debug_mem_remove_unlocked((unsigned char *)pointer, file, line, TRUE, FALSE, &old_size);
    nexus_debug_mem_stats_on_untracked_alloc(size);
    n_mem_unlock();
    return new_pointer;
  }

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].buf == pointer)
        break;
    }
    if (j < n_alloc_lines[i].alloc_count)
      break;
  }

  if (i == n_alloc_line_count) {
    /* Detect an interior pointer before falling back to libc realloc. */
    for (i = 0; i < n_alloc_line_count; i++) {
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
        unsigned char *start;
        unsigned char *end;
        start = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
        end   = start + n_alloc_lines[i].allocs[j].size;
        if ((unsigned char *)pointer > start && (unsigned char *)pointer < end) {
          printf("Nexus Mem debugger error: trying to reallocate pointer %p that is %u bytes into allocation made in %s on line %u\n", pointer,
                 (unsigned int)((unsigned char *)pointer - start), n_alloc_lines[i].file, n_alloc_lines[i].line);
          n_mem_unlock();
          NEXUS_MEMORY_CALL_ON_ERROR
          return NULL;
        }
      }
    }

    printf("Nexus Mem debugger error: trying to reallocate untracked pointer %p in %s line %u\n", pointer, file, line);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
    return realloc(pointer, size);
  }

  if (size > (size_t)-1 - NEXUS_MEMORY_OVER_ALLOC) {
    printf("Nexus Mem debugger error: realloc size overflow in file %s line %u\n", file, line);
    n_mem_unlock();
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  old_size     = n_alloc_lines[i].allocs[j].size;
  comment_copy = n_mem_string_duplicate(n_alloc_lines[i].allocs[j].comment);
  raw          = (unsigned char *)malloc(size + NEXUS_MEMORY_OVER_ALLOC);

  if (raw == NULL) {
    if (comment_copy != NULL)
      free(comment_copy);
    n_mem_unlock();
    return NULL;
  }

  memset(raw, N_MEMORY_MAGIC_NUMBER, NEXUS_MEMORY_PRE_PADDING);
  new_pointer = raw + NEXUS_MEMORY_PRE_PADDING;
  move        = old_size < size ? old_size : size;

  if (move != 0)
    memcpy(new_pointer, pointer, move);
  if (move < size)
    memset(new_pointer + move, N_MEMORY_INITIALIZATION, size - move);
  memset(new_pointer + size, N_MEMORY_MAGIC_NUMBER, NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING);

  if (!nexus_debug_mem_add_unlocked(new_pointer, size, file, line)) {
    free(raw);
    if (comment_copy != NULL)
      free(comment_copy);
    n_mem_unlock();
    return NULL;
  }

  /* Apply the old comment to the newly tracked block, if any. */
  if (comment_copy != NULL) {
    unsigned int ni;
    unsigned int nj;
    for (ni = 0; ni < n_alloc_line_count; ni++) {
      for (nj = 0; nj < n_alloc_lines[ni].alloc_count; nj++) {
        if (n_alloc_lines[ni].allocs[nj].buf == new_pointer) {
          n_alloc_lines[ni].allocs[nj].comment = comment_copy;
          comment_copy                         = NULL;
          break;
        }
      }
      if (comment_copy == NULL)
        break;
    }
  }

  (void)nexus_debug_mem_remove_unlocked((unsigned char *)pointer, file, line, TRUE, TRUE, &old_size);
  n_mem_unlock();

  if (comment_copy != NULL)
    free(comment_copy);

  nexus_debug_mem_log_format(file, line, "realloc %u bytes at pointer %p to %u bytes at pointer %p at %s line %u", (unsigned int)old_size, pointer,
                             (unsigned int)size, (void *)new_pointer, file, line);
  return new_pointer;
}

void nexus_debug_mem_summary_get(NexusDebugMemSummary *summary) {
  if (summary == NULL)
    return;

  n_mem_lock();
  nexus_debug_mem_summary_copy_unlocked(summary);
  n_mem_unlock();
}

static void nexus_debug_mem_summary_print_bytes(const char *label, uint_large byte_count) {
  char formatted[64];

  (void)nexus_strings_bytes_format(formatted, (uint_large)(sizeof formatted), byte_count);
  printf("%-28s%s\n", label, formatted);
}

/*
Caller must hold n_alloc_mutex when thread-safe init is configured.
*/
static void nexus_debug_mem_live_allocation_print(const NexusMemAllocLine *site, const NexusMemAllocBuf *alloc) {
  char size_label[64];

  (void)nexus_strings_bytes_format(size_label, (uint_large)(sizeof size_label), (uint_large)alloc->size);
  printf("  %p  %s  %s:%u", alloc->buf, size_label, site->file, site->line);
  if (alloc->comment != NULL)
    printf("  (%s)", alloc->comment);
  printf("\n");
}

void nexus_debug_mem_summary_print(void) {
  NexusDebugMemSummary summary;

  nexus_debug_mem_summary_get(&summary);
  printf("Memory summary\n----------------------------------------------\n");
  nexus_debug_mem_summary_print_bytes("Live bytes:", (uint_large)summary.live_bytes);
  nexus_debug_mem_summary_print_bytes("Peak live bytes:", (uint_large)summary.peak_live_bytes);
  printf("Live blocks:                %u\n", summary.live_block_count);
  printf("Peak live blocks:           %u\n", summary.peak_live_block_count);
  nexus_debug_mem_summary_print_bytes("Total bytes allocated:", summary.total_bytes_allocated);
  nexus_debug_mem_summary_print_bytes("Total bytes freed:", summary.total_bytes_freed);
  printf("Allocation events:          %llu\n", (unsigned long long)summary.allocation_count);
  printf("Free events:                %llu\n", (unsigned long long)summary.free_count);
  printf("Call sites tracked:         %u\n", summary.call_site_count);
  nexus_debug_mem_summary_print_bytes("Largest single allocation:", (uint_large)summary.largest_allocation_bytes);
  printf("----------------------------------------------\n");
}

void nexus_debug_mem_print(unsigned int min_allocs) {
  unsigned int i;
  unsigned int j;
  unsigned int alloc_count;
  size_t       consumption;
  char         consumption_label[64];

  if (!nexus_debug_mem_active_get())
    return;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return;
  }

  consumption = 0;
  for (i = 0; i < n_alloc_line_count; i++)
    consumption += n_alloc_lines[i].size;

  (void)nexus_strings_bytes_format(consumption_label, (uint_large)(sizeof consumption_label), (uint_large)consumption);
  printf("Memory report: %s\n----------------------------------------------\n", consumption_label);

  for (i = 0; i < n_alloc_line_count; i++) {
    alloc_count = n_alloc_lines[i].alloc_count;
    if (min_allocs < alloc_count) {
      char site_bytes[64];

      (void)nexus_strings_bytes_format(site_bytes, (uint_large)(sizeof site_bytes), (uint_large)n_alloc_lines[i].size);
      printf("%s line: %u\n", n_alloc_lines[i].file, n_alloc_lines[i].line);
      printf(" - live bytes: %s\n - live allocations: %u\n - allocations since reset: %u\n - frees since reset: %u\n", site_bytes, alloc_count,
             (unsigned int)n_alloc_lines[i].allocated, (unsigned int)n_alloc_lines[i].freed);
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++)
        nexus_debug_mem_live_allocation_print(&n_alloc_lines[i], &n_alloc_lines[i].allocs[j]);
      printf("\n");
    }
  }

  printf("----------------------------------------------\n");
  n_mem_unlock();
}

size_t nexus_debug_mem_footprint(unsigned int min_allocs) {
  unsigned int i;
  size_t       total;

  (void)min_allocs;
  total = 0;

  if (!nexus_debug_mem_active_get())
    return 0;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return 0;
  }
  for (i = 0; i < n_alloc_line_count; i++)
    total += n_alloc_lines[i].size;
  n_mem_unlock();

  return total;
}

void *nexus_debug_mem_query_allocation(void *pointer, unsigned int *line, char **file, size_t *size) {
  unsigned int   i;
  unsigned int   j;
  unsigned char *user_pointer;
  unsigned char *alloc_start;
  unsigned char *alloc_end;
  void          *result;

  result = NULL;

  if (!nexus_debug_mem_active_get())
    return NULL;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return NULL;
  }

  user_pointer = (unsigned char *)pointer;
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      alloc_start = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
      alloc_end   = alloc_start + n_alloc_lines[i].allocs[j].size;

      if (alloc_start <= user_pointer && user_pointer < alloc_end) {
        if (line != NULL)
          *line = n_alloc_lines[i].line;
        if (file != NULL)
          *file = n_alloc_lines[i].file; /* separately allocated; stable across site-table realloc */
        if (size != NULL)
          *size = n_alloc_lines[i].allocs[j].size;
        result = n_alloc_lines[i].allocs[j].buf;
        n_mem_unlock();
        return result;
      }
    }
  }

  n_mem_unlock();
  return NULL;
}

boolean nexus_debug_mem_query_is_allocated(const void *pointer, size_t size, boolean ignore_not_found) {
  unsigned int         i;
  unsigned int         j;
  const unsigned char *user_pointer;
  const unsigned char *alloc_start;
  const unsigned char *alloc_end;

  if (size == 0)
    return TRUE;

  if (!nexus_debug_mem_active_get())
    return TRUE;

  if (pointer == NULL)
    return FALSE;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return TRUE;
  }

  user_pointer = (const unsigned char *)pointer;

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      alloc_start = (const unsigned char *)n_alloc_lines[i].allocs[j].buf;
      alloc_end   = alloc_start + n_alloc_lines[i].allocs[j].size;

      if (alloc_start <= user_pointer && user_pointer <= alloc_end) {
        if (user_pointer == alloc_end || size > (size_t)(alloc_end - user_pointer)) {
          size_t missing;
          missing = user_pointer >= alloc_end ? size : size - (size_t)(alloc_end - user_pointer);
          printf("Nexus Mem debugger error: not enough memory to access pointer %p, %u bytes missing\n", pointer, (unsigned int)missing);
          n_mem_unlock();
          return FALSE;
        }

        n_mem_unlock();
        return TRUE;
      }
    }
  }

  if (n_freed_memory != NULL) {
    for (i = 0; i < NEXUS_MEMORY_FREE_POINTER_BUFFER_SIZE; i++) {
      if (!n_freed_memory[i].active || n_freed_memory[i].pointer == NULL || n_freed_memory[i].size == 0)
        continue;

      alloc_start = (const unsigned char *)n_freed_memory[i].pointer;
      alloc_end   = alloc_start + n_freed_memory[i].size;
      if (user_pointer >= alloc_start && user_pointer < alloc_end && size <= (size_t)(alloc_end - user_pointer)) {
        printf("Nexus Mem debugger error: pointer %p was freed on line %u in file %s\n", pointer, n_freed_memory[i].free_line,
               n_freed_memory[i].free_file);
        n_mem_unlock();
        return FALSE;
      }
    }
  }

  if (nexus_memory_stack_size != 0 && user_pointer >= nexus_memory_stack_pointer &&
      user_pointer < nexus_memory_stack_pointer + nexus_memory_stack_size) {
    if (size > (size_t)((nexus_memory_stack_pointer + nexus_memory_stack_size) - user_pointer))
      printf("Nexus Mem debugger error: memory is in stack, but access range does not fit\n");
    else
      printf("Nexus Mem debugger warning: memory is in stack\n");
    n_mem_unlock();
    return FALSE;
  }

  n_mem_unlock();

  if (!ignore_not_found)
    printf("Nexus Mem debugger warning: no matching memory for pointer %p found\n", pointer);
  return FALSE;
}

void nexus_debug_mem_bytes_copy(void *dest, const void *src, uint_large byte_count, char *file, uint32 line) {
  if (byte_count == 0)
    return;

  if (!nexus_debug_mem_active_get()) {
    memcpy(dest, src, (size_t)byte_count);
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(src != NULL);

  (void)nexus_debug_mem_query_is_allocated(dest, (size_t)byte_count, TRUE);
  (void)nexus_debug_mem_query_is_allocated(src, (size_t)byte_count, TRUE);

  nexus_debug_mem_log_format(file, line, "memcpy %u bytes from %p to %p at %s line %u", (unsigned int)byte_count, src, dest, file, line);
  memcpy(dest, src, (size_t)byte_count);
}

void nexus_debug_mem_bytes_set(void *dest, uint8 byte, uint_large byte_count, char *file, uint32 line) {
  if (byte_count == 0)
    return;

  if (!nexus_debug_mem_active_get()) {
    memset(dest, (int)byte, (size_t)byte_count);
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);

  (void)nexus_debug_mem_query_is_allocated(dest, (size_t)byte_count, TRUE);
  nexus_debug_mem_log_format(file, line, "memset %u bytes value %u at pointer %p at %s line %u", (unsigned int)byte_count, (unsigned int)byte, dest,
                             file, line);
  memset(dest, (int)byte, (size_t)byte_count);
}

void nexus_debug_mem_bytes_clear(void *dest, uint_large byte_count, char *file, uint32 line) {
  if (byte_count == 0)
    return;

  if (!nexus_debug_mem_active_get()) {
    memset(dest, 0, (size_t)byte_count);
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);

  (void)nexus_debug_mem_query_is_allocated(dest, (size_t)byte_count, TRUE);
  nexus_debug_mem_log_format(file, line, "memclear %u bytes at pointer %p at %s line %u", (unsigned int)byte_count, dest, file, line);
  memset(dest, 0, (size_t)byte_count);
}

size_t nexus_debug_mem_consumption(void) {
  unsigned int i;
  size_t       sum;

  sum = 0;

  if (!nexus_debug_mem_active_get())
    return 0;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return 0;
  }
  for (i = 0; i < n_alloc_line_count; i++)
    sum += n_alloc_lines[i].size;
  n_mem_unlock();

  return sum;
}

void exit_crash(uint32 status_code) {
  volatile uint32 *trap = NULL;
  (void)status_code;
  /* NOLINTNEXTLINE(clang-analyzer-core.NullDereference) intentional debugger trap */
  trap[0] = 0;
}

boolean nexus_debug_mem_check_stack_reference(void) {
  boolean output;
  size_t  size;
  size_t  i;
  size_t  j;
  size_t  k;
  void  **slots;

  output = FALSE;

  if (!nexus_debug_mem_active_get())
    return FALSE;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return FALSE;
  }

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      size_t distance;
      size_t best;
      size_t found;

      if (!n_alloc_lines[i].allocs[j].active)
        continue;

      slots = (void **)n_alloc_lines[i].allocs[j].buf;
      size  = n_alloc_lines[i].allocs[j].size / sizeof(void *);
      best  = NEXUS_MEMORY_STACK_GUESS_SIZE;
      found = N_MEM_OFFSET_INVALID;

      if (nexus_memory_stack_size != 0) {
        for (k = 0; k < size; k++) {
          if ((unsigned char *)slots[k] >= nexus_memory_stack_pointer &&
              (unsigned char *)slots[k] < nexus_memory_stack_pointer + nexus_memory_stack_size) {
            if (n_alloc_lines[i].allocs[j].comment == NULL)
              printf("Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s\n",
                     (unsigned int)(k * sizeof(void *)), n_alloc_lines[i].line, n_alloc_lines[i].file);
            else
              printf(
                  "Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s /* %s */\n",
                  (unsigned int)(k * sizeof(void *)), n_alloc_lines[i].line, n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment);
            output = TRUE;
          }
        }
      } else {
        for (k = 0; k < size; k++) {
          if ((unsigned char *)slots[k] > (unsigned char *)&i)
            distance = (size_t)((unsigned char *)slots[k] - (unsigned char *)&i);
          else
            distance = (size_t)((unsigned char *)&i - (unsigned char *)slots[k]);

          if (distance < best) {
            best  = distance;
            found = k * sizeof(void *);
          }
        }

        if (found != N_MEM_OFFSET_INVALID) {
          if (n_alloc_lines[i].allocs[j].comment == NULL)
            printf("Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s (%u bytes "
                   "from current stack pointer)\n",
                   (unsigned int)found, n_alloc_lines[i].line, n_alloc_lines[i].file, (unsigned int)best);
          else
            printf("Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s /* %s */ "
                   "(%u bytes from current stack pointer)\n",
                   (unsigned int)found, n_alloc_lines[i].line, n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment, (unsigned int)best);
          output = TRUE;
        }
      }
    }
  }

  n_mem_unlock();
  return output;
}

static boolean nexus_debug_mem_find_pointer_in_memory_unlocked(void *pointer) {
  size_t i;
  size_t j;
  size_t k;
  size_t size;
  void **slots;

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      slots = (void **)n_alloc_lines[i].allocs[j].buf;
      size  = n_alloc_lines[i].allocs[j].size / sizeof(void *);
      for (k = 0; k < size; k++) {
        if (slots[k] == pointer)
          return TRUE;
      }
    }
  }

  if (nexus_memory_stack_pointer != NULL && nexus_memory_stack_size != 0) {
    void **stack_slots;
    stack_slots = (void **)nexus_memory_stack_pointer;
    for (j = 0; j < nexus_memory_stack_size / sizeof(void *); j++) {
      if (stack_slots[j] == pointer)
        return TRUE;
    }
  }

  return FALSE;
}

void nexus_debug_mem_check_heap_reference(unsigned int minimum_allocations) {
  size_t i;
  size_t j;
  size_t found;

  if (!nexus_debug_mem_active_get())
    return;

  n_mem_lock();
  if (!nexus_memory_active) {
    n_mem_unlock();
    return;
  }

  for (i = 0; i < n_alloc_line_count; i++) {
    if (n_alloc_lines[i].alloc_count >= minimum_allocations) {
      found = 0;
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
        if (n_alloc_lines[i].allocs[j].active && nexus_debug_mem_find_pointer_in_memory_unlocked(n_alloc_lines[i].allocs[j].buf))
          found++;
      }

      if (found != n_alloc_lines[i].alloc_count) {
        if (nexus_memory_stack_size != 0)
          printf(
              "Nexus Mem debugger error: cannot find any reference in heap memory or configured stack for %u out of %u allocations made on line %u "
              "in file %s\n",
              n_alloc_lines[i].alloc_count - (unsigned int)found, n_alloc_lines[i].alloc_count, n_alloc_lines[i].line, n_alloc_lines[i].file);
        else
          printf("Nexus Mem debugger warning: cannot find any reference in heap memory for %u out of %u allocations made on line %u in file %s\n",
                 n_alloc_lines[i].alloc_count - (unsigned int)found, n_alloc_lines[i].alloc_count, n_alloc_lines[i].line, n_alloc_lines[i].file);
      }
    }
  }

  n_mem_unlock();
}
