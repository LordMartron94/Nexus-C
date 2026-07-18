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

static NexusDebugMemLogCallback *n_mem_log_callback         = NULL;
static void                     *n_mem_log_user_data        = NULL;
static boolean                   nexus_memory_log_emitting  = FALSE;
static boolean                   nexus_memory_active        = TRUE;
static unsigned char            *nexus_memory_stack_pointer = NULL;
static size_t                    nexus_memory_stack_size    = 0;

typedef struct {
  size_t  size;
  void   *buf;
  char   *comment;
  boolean active;
} NexusMemAllocBuf;

typedef struct {
  unsigned int      line;
  char              file[256];
  NexusMemAllocBuf *allocs;
  unsigned int      alloc_count;
  size_t            alloc_allocated;
  size_t            size;
  size_t            allocated;
  size_t            freed;
} NexusMemAllocLine;

static NexusMemAllocLine *n_alloc_lines;
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

static NexusMemFreeBuf *n_freed_memory;
static unsigned int     n_freed_memory_count = 0;
static unsigned int     n_freed_memory_store = 1024;

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

static NexusDebugMemMeasurementContext *n_mem_measurement_context = NULL;

static void nexus_debug_mem_stats_on_alloc(size_t size);
static void nexus_debug_mem_stats_on_free(size_t size);
static void nexus_debug_mem_stats_reset(void);

static void nexus_debug_mem_stats_on_alloc(size_t size) {
  if (!nexus_memory_active)
    return;
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

  if (n_mem_measurement_context != NULL && size > n_mem_measurement_context->interval_largest_allocation_bytes)
    n_mem_measurement_context->interval_largest_allocation_bytes = size;
}

static void nexus_debug_mem_stats_on_free(size_t size) {
  if (!nexus_memory_active)
    return;
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
  n_mem_stats_live_bytes            = 0;
  n_mem_stats_peak_live_bytes       = 0;
  n_mem_stats_live_block_count      = 0;
  n_mem_stats_peak_live_block_count = 0;
  n_mem_stats_total_bytes_allocated = 0;
  n_mem_stats_total_bytes_freed     = 0;
  n_mem_stats_allocation_count      = 0;
  n_mem_stats_free_count            = 0;
  n_mem_stats_largest_allocation    = 0;
}

static void    nexus_debug_mem_add(void *pointer, size_t size, char *file, unsigned int line);
static boolean nexus_debug_mem_remove(unsigned char *buf, char *file, unsigned int line, boolean was_realloc, size_t *out_size);
static boolean nexus_debug_mem_find_pointer_in_memory(void *pointer);

static void nexus_debug_mem_log_emit(const char *message, const char *file, unsigned int line) {
  if (n_mem_log_callback == NULL || nexus_memory_log_emitting || !nexus_memory_active)
    return;
  nexus_memory_log_emitting = TRUE;
  n_mem_log_callback(n_mem_log_user_data, message, file, (uint32)line);
  nexus_memory_log_emitting = FALSE;
}

static void nexus_debug_mem_log_format(const char *file, unsigned int line, const char *format, ...) {
  char                    message[NEXUS_MEMORY_LOG_MESSAGE_MAX];
  va_list                 args;
  NexusStringFormatResult format_result;

  va_start(args, format);
  format_result = nexus_strings_vstring_format_with_truncation(message, NEXUS_MEMORY_LOG_MESSAGE_MAX, format, args);
  va_end(args);
  if (!format_result.success && format[0] != '\0')
    return;
  nexus_debug_mem_log_emit(message, file, line);
}

void nexus_debug_mem_thread_safe_init(int (*lock)(void *mutex), int (*unlock)(void *mutex), void *mutex) {
  n_alloc_mutex        = mutex;
  n_alloc_mutex_lock   = lock;
  n_alloc_mutex_unlock = unlock;
}

void nexus_debug_mem_stack_pointer_set(void *lowest_stack_pointer, size_t stack_size_in_bytes) {
  nexus_memory_stack_pointer = (unsigned char *)lowest_stack_pointer;
  nexus_memory_stack_size    = stack_size_in_bytes;
}

void nexus_debug_mem_active(boolean active) {
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  nexus_memory_active = active;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
}

void nexus_debug_mem_log_callback_set(NexusDebugMemLogCallback *callback, void *user_data) {
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  n_mem_log_callback  = callback;
  n_mem_log_user_data = user_data;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
}

boolean nexus_debug_mem_log_callback_installed_get(void) {
  boolean installed;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  installed = n_mem_log_callback != NULL ? TRUE : FALSE;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return installed;
}

void nexus_debug_mem_reset(void) {
  unsigned int i;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++) {
    n_alloc_lines[i].allocated = 0;
    n_alloc_lines[i].size      = 0;
    n_alloc_lines[i].freed     = 0;
  }
  nexus_debug_mem_stats_reset();
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
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
  NexusDebugMemSummary summary;

  if (context == NULL)
    return;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);

  nexus_debug_mem_summary_copy_unlocked(&summary);
  context->baseline_allocation_count         = summary.allocation_count;
  context->baseline_total_bytes_allocated    = summary.total_bytes_allocated;
  context->interval_largest_allocation_bytes = 0;
  n_mem_measurement_context                  = context;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
}

void nexus_debug_mem_measurement_end(const NexusDebugMemMeasurementContext *context, NexusDebugMemMeasurement *measurement) {
  NexusDebugMemSummary summary;

  if (context == NULL || measurement == NULL)
    return;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);

  nexus_debug_mem_summary_copy_unlocked(&summary);
  measurement->allocation_count         = summary.allocation_count - context->baseline_allocation_count;
  measurement->total_bytes_allocated    = summary.total_bytes_allocated - context->baseline_total_bytes_allocated;
  measurement->largest_allocation_bytes = context->interval_largest_allocation_bytes;

  if (n_mem_measurement_context == context)
    n_mem_measurement_context = NULL;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
}

boolean nexus_debug_mem_check_bounds(void) {
  boolean        output = FALSE;
  size_t         size;
  unsigned char *buf;
  size_t         i;
  size_t         j;
  size_t         k;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].active) {
        buf  = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
        size = n_alloc_lines[i].allocs[j].size;
        for (k = 0; k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING; k++)
          if (buf[size + k] != N_MEMORY_MAGIC_NUMBER)
            break;
        if (k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING) {
          if (n_alloc_lines[i].allocs[j].comment == NULL)
            printf("Nexus Mem debugger error: memory overrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line,
                   n_alloc_lines[i].file);
          else
            printf("Nexus Mem debugger error: memory overrun of allocation made on line %u in file %s /* %s */\n", n_alloc_lines[i].line,
                   n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment);
          NEXUS_MEMORY_CALL_ON_ERROR
          output = TRUE;
        }
        buf = (unsigned char *)n_alloc_lines[i].allocs[j].buf - NEXUS_MEMORY_PRE_PADDING;
        for (k = 0; k < NEXUS_MEMORY_PRE_PADDING; k++)
          if (buf[k] != N_MEMORY_MAGIC_NUMBER)
            break;
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
  }
#ifdef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
  for (i = 0; i < n_freed_memory_count && i < n_freed_memory_store; i++) {
    buf  = (unsigned char *)n_freed_memory[i].pointer;
    size = n_freed_memory[i].size;
    for (k = 0; k < size && buf[k] == N_MEMORY_FREED; k++)
      ;
    if (k < size) {
      if (n_freed_memory[i].realloc)
        printf("Nexus Mem debugger error: pointer reallocated on line %u in file %s and freed on line %u in file %s was written to %u bytes into "
               "the allocation after being freed\n",
               n_freed_memory[i].alloc_line, n_freed_memory[i].alloc_file, n_freed_memory[i].free_line, n_freed_memory[i].free_file, (unsigned int)k);
      else
        printf("Nexus Mem debugger error: pointer allocated on line %u in file %s and freed on line %u in file %s was written to %u bytes into "
               "the allocation after being freed\n",
               n_freed_memory[i].alloc_line, n_freed_memory[i].alloc_file, n_freed_memory[i].free_line, n_freed_memory[i].free_file, (unsigned int)k);
      NEXUS_MEMORY_CALL_ON_ERROR
    }
  }
#endif
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return output;
}

static void nexus_debug_mem_add(void *pointer, size_t size, char *file, unsigned int line) {
  unsigned int       i;
  unsigned int       j;
  unsigned char     *pre;
  NexusMemAllocBuf  *new_allocs;
  NexusMemAllocLine *new_lines;

  pre = (unsigned char *)pointer - NEXUS_MEMORY_PRE_PADDING;
  for (i = 0; i < NEXUS_MEMORY_PRE_PADDING; i++)
    pre[i] = N_MEMORY_MAGIC_NUMBER;

  for (i = 0; i < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING; i++)
    ((unsigned char *)pointer)[size + i] = N_MEMORY_MAGIC_NUMBER;

  for (i = 0; i < n_alloc_line_count; i++) {
    if (line == n_alloc_lines[i].line) {
      for (j = 0; file[j] != 0 && file[j] == n_alloc_lines[i].file[j]; j++)
        ;
      if (file[j] == n_alloc_lines[i].file[j])
        break;
    }
  }
  if (i < n_alloc_line_count) {
    if (n_alloc_lines[i].alloc_allocated == n_alloc_lines[i].alloc_count) {
      n_alloc_lines[i].alloc_allocated += 1024;
      new_allocs = (NexusMemAllocBuf *)realloc(n_alloc_lines[i].allocs, (sizeof *n_alloc_lines[i].allocs) * n_alloc_lines[i].alloc_allocated);
      if (new_allocs == NULL) {
        printf("Nexus Mem debugger error: realloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line,
               file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return;
      }
      n_alloc_lines[i].allocs = new_allocs;
    }
    n_alloc_lines[i].allocs[n_alloc_lines[i].alloc_count].size    = size;
    n_alloc_lines[i].allocs[n_alloc_lines[i].alloc_count].comment = NULL;
    n_alloc_lines[i].allocs[n_alloc_lines[i].alloc_count].active  = nexus_memory_active;
    n_alloc_lines[i].allocs[n_alloc_lines[i].alloc_count++].buf   = pointer;
    if (nexus_memory_active) {
      n_alloc_lines[i].size += size;
      n_alloc_lines[i].allocated++;
      nexus_debug_mem_stats_on_alloc(size);
    }
  } else {
    if (i % 1024 == 0) {
      new_lines = (NexusMemAllocLine *)realloc(n_alloc_lines, (sizeof *n_alloc_lines) * (i + 1024));
      if (new_lines == NULL) {
        printf("Nexus Mem debugger error: realloc returns NULL when growing allocation table at line %u in file %s\n", line, file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return;
      }
      n_alloc_lines = new_lines;
    }
    n_alloc_lines[i].line = line;
    for (j = 0; j < 255 && file[j] != 0; j++)
      n_alloc_lines[i].file[j] = file[j];
    n_alloc_lines[i].file[j]           = 0;
    n_alloc_lines[i].alloc_allocated   = 256;
    n_alloc_lines[i].allocs            = (NexusMemAllocBuf *)malloc((sizeof *n_alloc_lines[i].allocs) * n_alloc_lines[i].alloc_allocated);
    n_alloc_lines[i].allocs[0].size    = size;
    n_alloc_lines[i].allocs[0].buf     = pointer;
    n_alloc_lines[i].allocs[0].comment = NULL;
    n_alloc_lines[i].allocs[0].active  = nexus_memory_active;
    n_alloc_lines[i].alloc_count       = 1;
    n_alloc_lines[i].freed             = 0;
    if (nexus_memory_active) {
      n_alloc_lines[i].allocated = 1;
      n_alloc_lines[i].size      = size;
      nexus_debug_mem_stats_on_alloc(size);
    } else {
      n_alloc_lines[i].allocated = 0;
      n_alloc_lines[i].size      = 0;
    }
    n_alloc_line_count++;
  }
}

void *nexus_debug_mem_malloc(size_t size, char *file, unsigned int line) {
  unsigned char *pointer;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  nexus_debug_mem_check_bounds();
#endif
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  if (size == 0) {
    printf("Nexus Mem debugger warning: malloc size zero in file %s line %u\n", file, line);
    NEXUS_MEMORY_CALL_ON_ERROR
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return NULL;
  }
  pointer = (unsigned char *)malloc(size + NEXUS_MEMORY_OVER_ALLOC);

  if (pointer == NULL) {
#ifdef NEXUS_MEMORY_NULL_ALLOCATION_ERROR
    printf("Nexus Mem debugger warning: malloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line,
           file);
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    NEXUS_MEMORY_CALL_ON_ERROR
#endif
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return NULL;
  }
  pointer += NEXUS_MEMORY_PRE_PADDING;
  nexus_memory_bytes_set(pointer, N_MEMORY_INITIALIZATION, (uint_large)size);
  nexus_debug_mem_add(pointer, size, file, line);
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  if (n_mem_log_callback != NULL)
    nexus_debug_mem_log_format(file, line, "malloc %u bytes at pointer %p at %s line %u", (unsigned int)size, (void *)pointer, file, line);
  return pointer;
}

void *nexus_debug_mem_calloc(size_t num, size_t size, char *file, unsigned int line) {
  unsigned char *pointer;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  nexus_debug_mem_check_bounds();
#endif
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  size *= num;
  if (size == 0) {
    printf("Nexus Mem debugger warning: calloc size zero in file %s line %u\n", file, line);
    NEXUS_MEMORY_CALL_ON_ERROR
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return NULL;
  }

  pointer = (unsigned char *)malloc(size + NEXUS_MEMORY_OVER_ALLOC);

  if (pointer == NULL) {
#ifdef NEXUS_MEMORY_NULL_ALLOCATION_ERROR
    printf("Nexus Mem debugger warning: calloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line,
           file);
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    NEXUS_MEMORY_CALL_ON_ERROR
#endif
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return NULL;
  }
  pointer += NEXUS_MEMORY_PRE_PADDING;
  nexus_memory_bytes_clear(pointer, (uint_large)size);
  nexus_debug_mem_add(pointer, size, file, line);
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  if (n_mem_log_callback != NULL)
    nexus_debug_mem_log_format(file, line, "calloc %u bytes at pointer %p at %s line %u", (unsigned int)size, (void *)pointer, file, line);
  return pointer;
}

static boolean nexus_debug_mem_remove(unsigned char *buf, char *file, unsigned int line, boolean was_realloc, size_t *out_size) {
  NexusMemFreeBuf *free_buf = NULL;
  unsigned int     i;
  unsigned int     j;
  unsigned int     k;
  size_t           distance;
  size_t           query_size;
  NexusMemFreeBuf *new_freed;

#if defined(NEXUS_MEMORY_DOUBLE_FREE_CHECK) || defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
  if (n_freed_memory_count % 1024 == 0 && n_freed_memory_count < n_freed_memory_store) {
    if (n_freed_memory_count == 0)
      n_freed_memory = (NexusMemFreeBuf *)malloc((sizeof *n_freed_memory) * 1024);
    else {
      new_freed = (NexusMemFreeBuf *)realloc(n_freed_memory, (sizeof *n_freed_memory) * (n_freed_memory_count + 1024));
      if (new_freed == NULL) {
        printf("Nexus Mem debugger error: realloc returns NULL when growing freed-pointer table at line %u in file %s\n", line, file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return FALSE;
      }
      n_freed_memory = new_freed;
    }
  }
#  ifdef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
  if (n_freed_memory_count >= n_freed_memory_store) {
    if (n_freed_memory[n_freed_memory_count % n_freed_memory_store].pointer != NULL) {
      free((unsigned char *)n_freed_memory[n_freed_memory_count % n_freed_memory_store].pointer - NEXUS_MEMORY_PRE_PADDING);
      n_freed_memory[n_freed_memory_count % n_freed_memory_store].pointer = NULL;
    }
  }
#  endif
  free_buf = &n_freed_memory[n_freed_memory_count++ % n_freed_memory_store];
  for (i = 0; i < 255 && file[i] != 0; i++)
    free_buf->free_file[i] = file[i];
  free_buf->free_file[i] = 0;
  free_buf->free_line    = line;
  free_buf->realloc      = was_realloc;
  free_buf->size         = 0;
  free_buf->pointer      = buf;
  free_buf->active       = nexus_memory_active;
#endif

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].buf == buf) {
        buf -= NEXUS_MEMORY_PRE_PADDING;
        for (k = 0; k < NEXUS_MEMORY_PRE_PADDING; k++)
          if (buf[k] != N_MEMORY_MAGIC_NUMBER)
            break;
        if (k < NEXUS_MEMORY_PRE_PADDING) {
          printf("Nexus Mem debugger error: buffer underrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line,
                 n_alloc_lines[i].file);
          NEXUS_MEMORY_CALL_ON_ERROR
        }
        for (k = 0; k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING; k++)
          if (buf[n_alloc_lines[i].allocs[j].size + NEXUS_MEMORY_PRE_PADDING + k] != N_MEMORY_MAGIC_NUMBER)
            break;
        if (k < NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING) {
          printf("Nexus Mem debugger error: buffer overrun of allocation made on line %u in file %s\n", n_alloc_lines[i].line, n_alloc_lines[i].file);
          NEXUS_MEMORY_CALL_ON_ERROR
        }
        memset(buf, N_MEMORY_FREED, n_alloc_lines[i].allocs[j].size + NEXUS_MEMORY_OVER_ALLOC);
        free_buf->alloc_line = n_alloc_lines[i].line;
        for (k = 0; k < 255 && n_alloc_lines[i].file[k] != 0; k++)
          free_buf->alloc_file[k] = n_alloc_lines[i].file[k];
        free_buf->alloc_file[k] = 0;
        free_buf->size          = n_alloc_lines[i].allocs[j].size;
        *out_size               = n_alloc_lines[i].allocs[j].size;
        n_alloc_lines[i].size -= n_alloc_lines[i].allocs[j].size;
        n_alloc_lines[i].allocs[j] = n_alloc_lines[i].allocs[--n_alloc_lines[i].alloc_count];
        if (nexus_memory_active) {
          n_alloc_lines[i].freed++;
          nexus_debug_mem_stats_on_free(*out_size);
        }

#ifndef NEXUS_MEMORY_USE_AFTER_FREE_CHECK
        free(buf);
#endif
        return TRUE;
      }
      if ((unsigned char *)n_alloc_lines[i].allocs[j].buf < buf &&
          (unsigned char *)n_alloc_lines[i].allocs[j].buf + n_alloc_lines[i].allocs[j].size > buf) {
        printf("Nexus Mem debugger error: trying to free pointer %p that is not at the start (%i bytes into) allocation made on line %u in file %s\n",
               (void *)buf, (int)(buf - (unsigned char *)n_alloc_lines[i].allocs[j].buf), n_alloc_lines[i].line, n_alloc_lines[i].file);
        NEXUS_MEMORY_CALL_ON_ERROR
        return FALSE;
      }
    }
  }
#if defined(NEXUS_MEMORY_DOUBLE_FREE_CHECK) || defined(NEXUS_MEMORY_USE_AFTER_FREE_CHECK)
  for (i = 0; i < n_freed_memory_count && i < n_freed_memory_store; i++) {
    if (free_buf != &n_freed_memory[i] && buf == n_freed_memory[i].pointer) {
      if (free_buf->realloc)
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
  if (nexus_memory_stack_size != 0) {
    if (nexus_memory_stack_pointer <= buf && nexus_memory_stack_pointer + nexus_memory_stack_size > buf) {
      printf("Nexus Mem debugger error: trying to free stack pointer on line %u in file %s\n", line, file);
      NEXUS_MEMORY_CALL_ON_ERROR
      return TRUE;
    }
  } else {
    if (buf > (unsigned char *)&i)
      distance = (size_t)(buf - (unsigned char *)&i);
    else
      distance = (size_t)((unsigned char *)&i - buf);
    if (distance < NEXUS_MEMORY_STACK_GUESS_SIZE) {
      printf("Nexus Mem debugger error: trying to free pointer not tracked by Nexus on line %u in file %s; likely a stack pointer (%u bytes from a "
             "known stack location)\n",
             line, file, (unsigned int)distance);
      NEXUS_MEMORY_CALL_ON_ERROR
      return TRUE;
    }
  }

  printf("Nexus Mem debugger warning: trying to free pointer not tracked by Nexus on line %u in file %s\n", line, file);

  if (NULL != nexus_debug_mem_query_allocation(buf, &line, &file, &query_size))
    printf("Nexus Mem debugger error: pointer is part of allocation made at line %u in file %s\n", line, file);

  free(buf);
  return TRUE;
}

void nexus_debug_mem_free(void *buf, char *file, unsigned int line) {
  size_t  size = 0;
  boolean removed;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  nexus_debug_mem_check_bounds();
#endif
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  removed = nexus_debug_mem_remove((unsigned char *)buf, file, line, FALSE, &size);
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  if (removed && n_mem_log_callback != NULL)
    nexus_debug_mem_log_format(file, line, "free %u bytes at pointer %p at %s line %u", (unsigned int)size, buf, file, line);
}

boolean nexus_debug_mem_comment(void *buf, char *comment) {
  unsigned int i;
  unsigned int j;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].buf == buf) {
        n_alloc_lines[i].allocs[j].comment = comment;
        if (n_alloc_mutex != NULL)
          n_alloc_mutex_unlock(n_alloc_mutex);
        return TRUE;
      }
    }
  }
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return FALSE;
}

void *nexus_debug_mem_realloc(void *pointer, size_t size, char *file, unsigned int line) {
  size_t         i;
  size_t         j;
  size_t         k;
  size_t         move;
  unsigned char *pointer2;

#ifdef NEXUS_MEMORY_CHECK_ALWAYS
  nexus_debug_mem_check_bounds();
#endif

  if (pointer == NULL) {
#ifdef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
    printf("Nexus Mem debugger warning: reallocating NULL in %s line %u; undefined behavior since C23\n", file, line);
#endif
    return nexus_debug_mem_malloc(size, file, line);
  }

  if (size == 0) {
    printf("Nexus Mem debugger warning: realloc size zero in file %s line %u\n", file, line);
    NEXUS_MEMORY_CALL_ON_ERROR
    return NULL;
  }

  j = 0;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++)
      if (n_alloc_lines[i].allocs[j].buf == pointer)
        break;
    if (j < n_alloc_lines[i].alloc_count)
      break;
  }
  if (i == n_alloc_line_count) {
    printf("Nexus Mem debugger error: trying to reallocate pointer %p in %s line %u; pointer is not allocated\n", pointer, file, line);
    for (i = 0; i < n_alloc_line_count; i++) {
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
        unsigned char *alloc_buf;
        alloc_buf = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
        for (k = 0; k < n_alloc_lines[i].allocs[j].size; k++) {
          if ((void *)&alloc_buf[k] == pointer) {
            printf("trying to reallocate pointer %u bytes (out of %u) into allocation made in %s on line %u\n", (unsigned int)k,
                   (unsigned int)n_alloc_lines[i].allocs[j].size, n_alloc_lines[i].file, n_alloc_lines[i].line);
            NEXUS_MEMORY_CALL_ON_ERROR
            if (n_alloc_mutex != NULL)
              n_alloc_mutex_unlock(n_alloc_mutex);
            return NULL;
          }
        }
      }
    }

    printf("\n");
    NEXUS_MEMORY_CALL_ON_ERROR

#ifdef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
    if (size == 0)
      printf("Nexus Mem debugger warning: reallocating pointer to zero size on line %u in file %s; undefined behavior since C23\n", line, file);
#endif
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return realloc(pointer, size);
  }
  if (size == 0) {
#ifdef NEXUS_MEMORY_WARN_ON_REALLOC_NULL
    printf("Nexus Mem debugger warning: reallocating pointer to zero size on line %u in file %s; undefined behavior since C23\n", line, file);
#endif
    nexus_debug_mem_free(pointer, file, line);
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return NULL;
  }

  pointer2 = (unsigned char *)malloc(size + NEXUS_MEMORY_OVER_ALLOC);
  if (pointer2 == NULL) {
    printf("Nexus Mem debugger warning: realloc returns NULL when trying to allocate %u bytes at line %u in file %s\n", (unsigned int)size, line,
           file);
    NEXUS_MEMORY_CALL_ON_ERROR
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return NULL;
  }
  memset(pointer2, N_MEMORY_MAGIC_NUMBER, NEXUS_MEMORY_PRE_PADDING);
  move = n_alloc_lines[i].allocs[j].size;
  if (move > size)
    move = size;
  memcpy(pointer2 + NEXUS_MEMORY_PRE_PADDING, pointer, move);
  if (move < size)
    memset(pointer2 + NEXUS_MEMORY_PRE_PADDING + move, N_MEMORY_INITIALIZATION, size - move);
  memset(pointer2 + NEXUS_MEMORY_PRE_PADDING + size, N_MEMORY_MAGIC_NUMBER, NEXUS_MEMORY_OVER_ALLOC - NEXUS_MEMORY_PRE_PADDING);
  pointer2 += NEXUS_MEMORY_PRE_PADDING;
  nexus_debug_mem_add(pointer2, size, file, line);
  move = n_alloc_lines[i].allocs[j].size;
  nexus_debug_mem_remove((unsigned char *)pointer, file, line, TRUE, &move);
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  if (n_mem_log_callback != NULL)
    nexus_debug_mem_log_format(file, line, "realloc %u bytes at pointer %p to %u bytes at pointer %p at %s line %u", (unsigned int)size, pointer,
                               (unsigned int)move, (void *)pointer2, file, line);
  return pointer2;
}

void nexus_debug_mem_summary_get(NexusDebugMemSummary *summary) {
  if (summary == NULL)
    return;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  nexus_debug_mem_summary_copy_unlocked(summary);
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
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
  char         consumption_label[64];

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);

  (void)nexus_strings_bytes_format(consumption_label, (uint_large)(sizeof consumption_label), (uint_large)nexus_debug_mem_consumption());
  printf("Memory report: %s\n----------------------------------------------\n", consumption_label);
  for (i = 0; i < n_alloc_line_count; i++) {
    alloc_count = n_alloc_lines[i].alloc_count;
    if (min_allocs < alloc_count) {
      char site_bytes[64];

      (void)nexus_strings_bytes_format(site_bytes, (uint_large)(sizeof site_bytes), (uint_large)n_alloc_lines[i].size);
      printf("%s line: %u\n", n_alloc_lines[i].file, n_alloc_lines[i].line);
      printf(" - bytes allocated: %s\n - allocations: %u\n - frees: %u\n", site_bytes, alloc_count, (unsigned int)n_alloc_lines[i].freed);
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++)
        nexus_debug_mem_live_allocation_print(&n_alloc_lines[i], &n_alloc_lines[i].allocs[j]);
      printf("\n");
    }
  }
  printf("----------------------------------------------\n");
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
}

size_t nexus_debug_mem_footprint(unsigned int min_allocs) {
  unsigned int i;
  size_t       total = 0;

  (void)min_allocs;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++)
    total += n_alloc_lines[i].size;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return total;
}

void *nexus_debug_mem_query_allocation(void *pointer, unsigned int *line, char **file, size_t *size) {
  unsigned int   i;
  unsigned int   j;
  unsigned char *user_pointer;
  unsigned char *alloc_start;
  unsigned char *alloc_end;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  user_pointer = (unsigned char *)pointer;
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      alloc_start = (unsigned char *)n_alloc_lines[i].allocs[j].buf;
      alloc_end   = alloc_start + n_alloc_lines[i].allocs[j].size;
      if (alloc_start <= user_pointer && alloc_end > user_pointer) {
        if (line != NULL)
          *line = n_alloc_lines[i].line;
        if (file != NULL)
          *file = n_alloc_lines[i].file;
        if (size != NULL)
          *size = n_alloc_lines[i].allocs[j].size;
        if (n_alloc_mutex != NULL)
          n_alloc_mutex_unlock(n_alloc_mutex);
        return n_alloc_lines[i].allocs[j].buf;
      }
    }
  }
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return NULL;
}

boolean nexus_debug_mem_query_is_allocated(const void *pointer, size_t size, boolean ignore_not_found) {
  unsigned int         i;
  unsigned int         j;
  const unsigned char *user_pointer;
  const unsigned char *alloc_start;
  const unsigned char *alloc_end;
  const unsigned char *access_end;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  user_pointer = (const unsigned char *)pointer;
  access_end   = user_pointer + size;
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      alloc_start = (const unsigned char *)n_alloc_lines[i].allocs[j].buf;
      alloc_end   = alloc_start + n_alloc_lines[i].allocs[j].size;
      if (alloc_start <= user_pointer && alloc_end >= user_pointer) {
        if (alloc_end < access_end) {
          printf("Nexus Mem debugger error: not enough memory to access pointer %p, %u bytes missing\n", pointer,
                 (unsigned int)(access_end - alloc_end));
          if (n_alloc_mutex != NULL)
            n_alloc_mutex_unlock(n_alloc_mutex);
          return FALSE;
        }
        if (n_alloc_mutex != NULL)
          n_alloc_mutex_unlock(n_alloc_mutex);
        return TRUE;
      }
    }
  }

  for (i = 0; i < n_freed_memory_count; i++) {
    alloc_start = (const unsigned char *)n_freed_memory[i].pointer;
    alloc_end   = alloc_start + n_freed_memory[i].size;
    if (user_pointer >= alloc_start && access_end <= alloc_end) {
      printf("Nexus Mem debugger error: pointer %p was freed on line %u in file %s\n", pointer, n_freed_memory[i].free_line,
             n_freed_memory[i].free_file);
    }
  }
  if (nexus_memory_stack_size != 0 && user_pointer >= nexus_memory_stack_pointer &&
      user_pointer < nexus_memory_stack_pointer + nexus_memory_stack_size) {
    if (nexus_memory_stack_pointer + nexus_memory_stack_size < access_end)
      printf("Nexus Mem debugger error: memory is in stack, but access range does not fit\n");
    else
      printf("Nexus Mem debugger warning: memory is in stack\n");
    if (n_alloc_mutex != NULL)
      n_alloc_mutex_unlock(n_alloc_mutex);
    return FALSE;
  }
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  if (ignore_not_found)
    return FALSE;
  printf("Nexus Mem debugger warning: no matching memory for pointer %p found\n", pointer);
  return FALSE;
}

void nexus_debug_mem_bytes_copy(void *dest, const void *src, uint_large byte_count, char *file, uint32 line) {
  if (byte_count == 0) {
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(src != NULL);

  (void)nexus_debug_mem_query_is_allocated(dest, (size_t)byte_count, TRUE);
  (void)nexus_debug_mem_query_is_allocated(src, (size_t)byte_count, TRUE);

  if (n_mem_log_callback != NULL) {
    nexus_debug_mem_log_format(file, line, "memcpy %u bytes from %p to %p at %s line %u", (unsigned int)byte_count, src, dest, file, line);
  }

  memcpy(dest, src, (size_t)byte_count);
}

void nexus_debug_mem_bytes_set(void *dest, uint8 byte, uint_large byte_count, char *file, uint32 line) {
  if (byte_count == 0) {
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);

  (void)nexus_debug_mem_query_is_allocated(dest, (size_t)byte_count, TRUE);

  if (n_mem_log_callback != NULL) {
    nexus_debug_mem_log_format(file, line, "memset %u bytes value %u at pointer %p at %s line %u", (unsigned int)byte_count, (unsigned int)byte, dest,
                               file, line);
  }

  memset(dest, (int)byte, (size_t)byte_count);
}

void nexus_debug_mem_bytes_clear(void *dest, uint_large byte_count, char *file, uint32 line) {
  if (byte_count == 0) {
    return;
  }

  NEXUS_ASSERT_DEBUG(dest != NULL);

  (void)nexus_debug_mem_query_is_allocated(dest, (size_t)byte_count, TRUE);

  if (n_mem_log_callback != NULL) {
    nexus_debug_mem_log_format(file, line, "memclear %u bytes at pointer %p at %s line %u", (unsigned int)byte_count, dest, file, line);
  }

  memset(dest, 0, (size_t)byte_count);
}

size_t nexus_debug_mem_consumption(void) {
  unsigned int i;
  size_t       sum = 0;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++)
    sum += n_alloc_lines[i].size;
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return sum;
}

void exit_crash(uint32 status_code) {
  volatile uint32 *trap = NULL;
  (void)status_code;
  /* NOLINTNEXTLINE(clang-analyzer-core.NullDereference) intentional debugger trap */
  trap[0] = 0;
}

boolean nexus_debug_mem_check_stack_reference(void) {
  boolean output = FALSE;
  size_t  size;
  size_t  i;
  size_t  j;
  size_t  k;
  size_t  distance;
  size_t  best  = NEXUS_MEMORY_STACK_GUESS_SIZE;
  size_t  found = N_MEM_OFFSET_INVALID;
  void  **slots;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);

  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      if (n_alloc_lines[i].allocs[j].active) {
        slots = (void **)n_alloc_lines[i].allocs[j].buf;
        size  = n_alloc_lines[i].allocs[j].size / sizeof(void *);
        if (nexus_memory_stack_size != 0) {
          for (k = 0; k < size; k++) {
            if ((unsigned char *)slots[k] >= nexus_memory_stack_pointer &&
                (unsigned char *)slots[k] < nexus_memory_stack_pointer + nexus_memory_stack_size) {
              if (n_alloc_lines[i].allocs[j].comment == NULL)
                printf("Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s (%u "
                       "bytes from known stack pointer)\n",
                       (unsigned int)(k * sizeof(void *)), n_alloc_lines[i].line, n_alloc_lines[i].file, (unsigned int)best);
              else
                printf("Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s /* %s "
                       "*/ (%u bytes from known stack pointer)\n",
                       (unsigned int)(k * sizeof(void *)), n_alloc_lines[i].line, n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment,
                       (unsigned int)best);
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
              printf(
                  "Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s (%u bytes "
                  "from known stack pointer)\n",
                  (unsigned int)found, n_alloc_lines[i].line, n_alloc_lines[i].file, (unsigned int)best);
            else
              printf("Nexus Mem debugger warning: suspected reference to stack variable %u bytes into allocation made on line %u in file %s /* %s "
                     "*/ (%u bytes from known stack pointer)\n",
                     (unsigned int)found, n_alloc_lines[i].line, n_alloc_lines[i].file, n_alloc_lines[i].allocs[j].comment, (unsigned int)best);
            output = TRUE;
          }
        }
      }
    }
  }
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return output;
}

static boolean nexus_debug_mem_find_pointer_in_memory(void *pointer) {
  size_t size;
  size_t i;
  size_t j;
  size_t k;
  void **slots;
  void  *read;
  void **stack_slots;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);
  for (i = 0; i < n_alloc_line_count; i++) {
    for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
      slots = (void **)n_alloc_lines[i].allocs[j].buf;
      size  = n_alloc_lines[i].allocs[j].size / sizeof(void *);
      for (k = 0; k < size; k++) {
        read = slots[k];
        if (pointer == read) {
          if (n_alloc_mutex != NULL)
            n_alloc_mutex_unlock(n_alloc_mutex);
          return TRUE;
        }
      }
    }
  }
  stack_slots = (void **)nexus_memory_stack_pointer;
  for (j = 0; j < nexus_memory_stack_size / sizeof(void *); j++) {
    void *stack_ref;
    stack_ref = stack_slots[j];
    if (stack_ref == pointer) {
      if (n_alloc_mutex != NULL)
        n_alloc_mutex_unlock(n_alloc_mutex);
      return TRUE;
    }
  }
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
  return FALSE;
}

void nexus_debug_mem_check_heap_reference(unsigned int minimum_allocations) {
  size_t i;
  size_t j;
  size_t found;

  if (n_alloc_mutex != NULL)
    n_alloc_mutex_lock(n_alloc_mutex);

  for (i = 0; i < n_alloc_line_count; i++) {
    if (n_alloc_lines[i].alloc_count >= minimum_allocations) {
      found = 0;
      for (j = 0; j < n_alloc_lines[i].alloc_count; j++) {
        if (n_alloc_lines[i].allocs[j].active) {
          if (nexus_debug_mem_find_pointer_in_memory(n_alloc_lines[i].allocs[j].buf))
            found++;
        }
      }
      if (found != n_alloc_lines[i].alloc_count) {
        if (nexus_memory_stack_size)
          printf("Nexus Mem debugger error: cannot find any reference in heap memory or stack for %u out of %u allocations made on line %u in file "
                 "%s\n",
                 n_alloc_lines[i].alloc_count - (unsigned int)found, n_alloc_lines[i].alloc_count, n_alloc_lines[i].line, n_alloc_lines[i].file);
        else
          printf("Nexus Mem debugger warning: cannot find any reference in heap memory for %u out of %u allocations made on line %u in file %s\n",
                 n_alloc_lines[i].alloc_count - (unsigned int)found, n_alloc_lines[i].alloc_count, n_alloc_lines[i].line, n_alloc_lines[i].file);
      }
    }
  }
  if (n_alloc_mutex != NULL)
    n_alloc_mutex_unlock(n_alloc_mutex);
}
