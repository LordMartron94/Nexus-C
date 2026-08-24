#include "../nexus.h"

typedef struct NexusRegion {
  uint_large          byte_size;
  boolean             available;
  struct NexusRegion *next;
  struct NexusRegion *prev;
} NexusRegion;

typedef struct Page {
  void        *memory_base;
  uint_large   capacity;
  struct Page *next;
  NexusRegion *first_region;
} Page;

typedef struct PagedAllocator {
  Page      *page_head;
  uint64     page_count;
  uint_large page_size;
} PagedAllocator;

NexusPagedAllocator *nexus_memory_paged_allocator_create(uint_large page_size, uint64 initial_pages) {
  PagedAllocator *allocator;
  Page           *current_page;
  Page           *previous_page;
  NexusRegion    *initial_region;
  uint64          i;

  allocator = (PagedAllocator *)malloc(NEXUS_SIZEOF(PagedAllocator));
  if (allocator == NULL) {
    return NULL;
  }

  allocator->page_head  = NULL;
  allocator->page_count = 0;
  allocator->page_size  = page_size;

  previous_page = NULL;
  for (i = 0; i < initial_pages; i++) {
    current_page = (Page *)malloc(NEXUS_SIZEOF(Page) + page_size);
    if (current_page == NULL) {
      nexus_memory_paged_allocator_destroy((NexusPagedAllocator *)allocator);
      return NULL;
    }

    current_page->memory_base = (void *)((char *)current_page + NEXUS_SIZEOF(Page));
    current_page->capacity    = page_size;
    current_page->next        = NULL;

    initial_region            = (NexusRegion *)current_page->memory_base;
    initial_region->byte_size = page_size - NEXUS_SIZEOF(NexusRegion);
    initial_region->available = TRUE;
    initial_region->next      = NULL;
    initial_region->prev      = NULL;

    current_page->first_region = initial_region;

    if (previous_page != NULL) {
      previous_page->next = current_page;
    } else {
      allocator->page_head = current_page;
    }
    previous_page = current_page;
    allocator->page_count++;
  }

  return (NexusPagedAllocator *)allocator;
}

void nexus_memory_paged_allocator_destroy(NexusPagedAllocator *allocator_handle) {
  PagedAllocator *allocator;
  Page           *page;
  Page           *next_page;

  if (allocator_handle == NULL) {
    return;
  }

  allocator = (PagedAllocator *)allocator_handle;
  page      = allocator->page_head;

  while (page != NULL) {
    next_page = page->next;
    free(page);
    page = next_page;
  }

  free(allocator);
}

void *nexus_memory_paged_allocator_malloc(NexusPagedAllocator *allocator_handle, uint_large size, uint_large alignment) {
  PagedAllocator *allocator;
  Page           *page;
  Page           *last_page;
  NexusRegion    *region;
  NexusRegion    *new_region;
  char           *raw_ptr;
  char           *aligned_ptr;
  uint_large      remainder;
  uint_large      required_space;
  uint_large      space_after;
  uint_large      padding;
  uint_large      alignment_fix;

  if (size == 0 || allocator_handle == NULL) {
    return NULL;
  }

  /* Align size to machine word so split headers remain naturally aligned */
  alignment_fix = size % NEXUS_SIZEOF(void *);
  if (alignment_fix != 0) {
    size += (NEXUS_SIZEOF(void *) - alignment_fix);
  }

  allocator = (PagedAllocator *)allocator_handle;

retry_allocation:
  page      = allocator->page_head;
  last_page = NULL;

  while (page != NULL) {
    region = page->first_region;
    while (region != NULL) {
      if (region->available) {
        /* Payload must have room for the header back-pointer just before it */
        raw_ptr     = (char *)region + NEXUS_SIZEOF(NexusRegion) + NEXUS_SIZEOF(NexusRegion *);
        aligned_ptr = raw_ptr;

        if (alignment > 0) {
          remainder = (uint_large)aligned_ptr % alignment;
          if (remainder != 0) {
            aligned_ptr += (alignment - remainder);
          }
        }

        padding        = (uint_large)(aligned_ptr - ((char *)region + NEXUS_SIZEOF(NexusRegion)));
        required_space = padding + size;

        if (region->byte_size >= required_space) {
          space_after = region->byte_size - required_space;

          /* Split block if leftover space can hold a header + 32 bytes of usable payload */
          if (space_after > NEXUS_SIZEOF(NexusRegion) + 32) {
            new_region            = (NexusRegion *)(aligned_ptr + size);
            new_region->byte_size = space_after - NEXUS_SIZEOF(NexusRegion);
            new_region->available = TRUE;
            new_region->prev      = region;
            new_region->next      = region->next;

            if (region->next != NULL) {
              region->next->prev = new_region;
            }

            region->next      = new_region;
            region->byte_size = required_space;
          }

          region->available = FALSE;

          /* Store the pointer to the region header immediately before the returned payload */
          *((NexusRegion **)(aligned_ptr)-1) = region;

          return (void *)aligned_ptr;
        }
      }
      region = region->next;
    }
    last_page = page;
    page      = page->next;
  }

  /* Exhausted. Allocate and link a new page. */
  if (size + NEXUS_SIZEOF(NexusRegion *) + alignment + NEXUS_SIZEOF(NexusRegion) > allocator->page_size) {
    return NULL; /* Request physically cannot fit in the configured page size */
  }

  page = (Page *)malloc(NEXUS_SIZEOF(Page) + allocator->page_size);
  if (page == NULL) {
    return NULL;
  }

  page->memory_base = (void *)((char *)page + NEXUS_SIZEOF(Page));
  page->capacity    = allocator->page_size;
  page->next        = NULL;

  region            = (NexusRegion *)page->memory_base;
  region->byte_size = allocator->page_size - NEXUS_SIZEOF(NexusRegion);
  region->available = TRUE;
  region->next      = NULL;
  region->prev      = NULL;

  page->first_region = region;

  if (last_page != NULL) {
    last_page->next = page;
  } else {
    allocator->page_head = page;
  }
  allocator->page_count++;

  goto retry_allocation;
}

void *nexus_memory_paged_allocator_calloc(NexusPagedAllocator *allocator_handle, uint_large size, uint_large alignment) {
  void *ptr;

  ptr = nexus_memory_paged_allocator_malloc(allocator_handle, size, alignment);
  if (ptr != NULL) {
    nexus_memory_bytes_set(ptr, 0, size);
  }

  return ptr;
}

void nexus_memory_paged_allocator_free(NexusPagedAllocator *allocator_handle, void *ptr) {
  NexusRegion *region;

  if (ptr == NULL || allocator_handle == NULL) {
    return;
  }

  /* O(1) lookup: The region pointer is stored immediately before the payload */
  region = *((NexusRegion **)ptr - 1);

  region->available = TRUE;

  /* O(1) Coalesce Right */
  if (region->next != NULL && region->next->available) {
    region->byte_size += NEXUS_SIZEOF(NexusRegion) + region->next->byte_size;
    region->next = region->next->next;
    if (region->next != NULL) {
      region->next->prev = region;
    }
  }

  /* O(1) Coalesce Left */
  if (region->prev != NULL && region->prev->available) {
    region->prev->byte_size += NEXUS_SIZEOF(NexusRegion) + region->byte_size;
    region->prev->next = region->next;
    if (region->next != NULL) {
      region->next->prev = region->prev;
    }
  }
}

void nexus_memory_paged_allocator_reset(NexusPagedAllocator *allocator_handle) {
  PagedAllocator *allocator;
  Page           *page;
  NexusRegion    *initial_region;

  if (allocator_handle == NULL) {
    return;
  }

  allocator = (PagedAllocator *)allocator_handle;
  page      = allocator->page_head;

  while (page != NULL) {
    initial_region            = (NexusRegion *)page->memory_base;
    initial_region->byte_size = page->capacity - NEXUS_SIZEOF(NexusRegion);
    initial_region->available = TRUE;
    initial_region->next      = NULL;
    initial_region->prev      = NULL;

    page->first_region = initial_region;
    page               = page->next;
  }
}