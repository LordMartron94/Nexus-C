#include "../nexus.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <windows.h>
#elif NEXUS_PLATFORM_POSIX
#  include <dirent.h>
#  include <unistd.h>

extern char *realpath(const char *path, char *resolved_path);
#else
#  error "Unsupported platform"
#endif

static boolean nexus_paths_path_is_separator(char character) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  return character == '/' || character == '\\';
#else
  return character == '/';
#endif
}

static boolean nexus_paths_path_canonicalize(const char *path, char *out_canonical) {
  NexusPath joined_path;

#if defined(NEXUS_PLATFORM_WINDOWS)
  DWORD resolved_length;

  resolved_length = GetFullPathNameA(path, (DWORD)NEXUS_MAX_PATH_LENGTH, out_canonical, NULL);
  if (resolved_length == 0 || resolved_length >= (DWORD)NEXUS_MAX_PATH_LENGTH)
    return FALSE;

  return TRUE;
#else
  if (realpath(path, out_canonical) != NULL)
    return TRUE;

  if (path[0] == '/') {
    nexus_strings_string_copy(out_canonical, (uint_large)NEXUS_MAX_PATH_LENGTH, path);
    return TRUE;
  }

  if (getcwd(out_canonical, (size_t)NEXUS_MAX_PATH_LENGTH) == NULL)
    return FALSE;

  joined_path = nexus_paths_path_create(out_canonical);
  nexus_paths_path_append(&joined_path, path);
  nexus_strings_string_copy(out_canonical, (uint_large)NEXUS_MAX_PATH_LENGTH, joined_path.buffer);
  return TRUE;
#endif
}

static NexusPath nexus_paths_path_relative_between(const char *from_canonical, const char *to_canonical) {
  NexusPath result;
  uint16    prefix_length;
  uint16    scan_index;
  uint16    to_index;
  uint16    result_length;
  boolean   has_component;

  result.length    = 0;
  result.buffer[0] = '\0';
  result_length    = 0;
  prefix_length    = 0;
  scan_index       = 0;

  while (from_canonical[scan_index] && to_canonical[scan_index] && from_canonical[scan_index] == to_canonical[scan_index]) {
    if (nexus_paths_path_is_separator(from_canonical[scan_index]))
      prefix_length = (uint16)(scan_index + 1);
    scan_index++;
  }

  if (from_canonical[scan_index] == '\0' && (to_canonical[scan_index] == '\0' || nexus_paths_path_is_separator(to_canonical[scan_index]))) {
    if (nexus_paths_path_is_separator(to_canonical[scan_index]))
      prefix_length = (uint16)(scan_index + 1);
    else
      prefix_length = scan_index;
  } else {
    while (prefix_length > 0 && !nexus_paths_path_is_separator(from_canonical[prefix_length - 1]))
      prefix_length--;
  }

  scan_index    = prefix_length;
  has_component = FALSE;
  while (from_canonical[scan_index]) {
    if (nexus_paths_path_is_separator(from_canonical[scan_index])) {
      if (has_component) {
        if (result_length + 3 >= NEXUS_MAX_PATH_LENGTH)
          return nexus_paths_path_create(to_canonical);

        if (result_length > 0) {
          result.buffer[result_length] = '/';
          result_length++;
        }

        result.buffer[result_length]     = '.';
        result.buffer[result_length + 1] = '.';
        result_length                    = (uint16)(result_length + 2);
        has_component                    = FALSE;
      }
    } else {
      has_component = TRUE;
    }

    scan_index++;
  }

  if (has_component) {
    if (result_length + 2 >= NEXUS_MAX_PATH_LENGTH)
      return nexus_paths_path_create(to_canonical);

    if (result_length > 0) {
      result.buffer[result_length] = '/';
      result_length++;
    }

    result.buffer[result_length]     = '.';
    result.buffer[result_length + 1] = '.';
    result_length                    = (uint16)(result_length + 2);
  }

  to_index = prefix_length;
  while (nexus_paths_path_is_separator(to_canonical[to_index]))
    to_index++;

  if (to_canonical[to_index] != '\0' && result_length > 0) {
    result.buffer[result_length] = '/';
    result_length++;
  }

  while (to_canonical[to_index]) {
    if (result_length + 1 >= NEXUS_MAX_PATH_LENGTH)
      return nexus_paths_path_create(to_canonical);

    result.buffer[result_length] = to_canonical[to_index];
    result_length++;
    to_index++;
  }

  result.buffer[result_length] = '\0';
  result.length                = result_length;

  if (result.length == 0)
    return nexus_paths_path_create(".");

  return result;
}

NexusPath nexus_paths_path_create(const char *base_path) {
  NexusPath path;
  uint16    i = 0;

  NEXUS_ASSERT_DEBUG(base_path != NULL);

  while (base_path[i] && i < NEXUS_MAX_PATH_LENGTH - 1) {
    path.buffer[i] = base_path[i];
    i++;
  }
  path.buffer[i] = '\0';
  path.length    = i;

  return path;
}

void nexus_paths_path_append(NexusPath *path, const char *element) {
  uint16 i = 0;

  NEXUS_ASSERT_DEBUG(path != NULL);
  NEXUS_ASSERT_DEBUG(element != NULL);
  NEXUS_ASSERT_DEBUG(path->length < NEXUS_MAX_PATH_LENGTH - 2);

  if (path->length > 0 && path->buffer[path->length - 1] != '/' && path->buffer[path->length - 1] != '\\') {
#if defined(NEXUS_PLATFORM_WINDOWS)
    path->buffer[path->length] = '\\';
#else
    path->buffer[path->length] = '/';
#endif
    path->length++;
  }

  while (element[i] && path->length < NEXUS_MAX_PATH_LENGTH - 1) {
    path->buffer[path->length] = element[i];
    path->length++;
    i++;
  }
  path->buffer[path->length] = '\0';
}

const char *nexus_paths_path_base_name_get(const NexusPath *path) {
  const char *base;
  const char *ptr;

  NEXUS_ASSERT_DEBUG(path != NULL);

  base = path->buffer;
  ptr  = path->buffer;

  while (*ptr) {
    if (*ptr == '/' || *ptr == '\\') {
      base = ptr + 1;
    }
    ptr++;
  }
  return base;
}

boolean nexus_paths_path_is_absolute(NexusPath path) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  if (path.length >= 2 && nexus_paths_path_is_separator(path.buffer[0]) && nexus_paths_path_is_separator(path.buffer[1]))
    return TRUE;

  if (path.length >= 2 && ((path.buffer[0] >= 'A' && path.buffer[0] <= 'Z') || (path.buffer[0] >= 'a' && path.buffer[0] <= 'z')) &&
      path.buffer[1] == ':') {
    if (path.length == 2)
      return FALSE;

    return nexus_paths_path_is_separator(path.buffer[2]);
  }

  return FALSE;
#else
  return path.length > 0 && path.buffer[0] == '/';
#endif
}

NexusPath nexus_paths_path_relative_to_absolute(NexusPath path) {
  char canonical_path[NEXUS_MAX_PATH_LENGTH];

  if (nexus_paths_path_is_absolute(path))
    return path;

  if (!nexus_paths_path_canonicalize(path.buffer, canonical_path))
    return path;

  return nexus_paths_path_create(canonical_path);
}

NexusPath nexus_paths_path_absolute_to_relative(NexusPath path) {
  char cwd_buffer[NEXUS_MAX_PATH_LENGTH];
  char absolute_path[NEXUS_MAX_PATH_LENGTH];
  char absolute_cwd[NEXUS_MAX_PATH_LENGTH];

  if (!nexus_paths_path_is_absolute(path))
    return path;

#if defined(NEXUS_PLATFORM_WINDOWS)
  if (GetCurrentDirectoryA((DWORD)NEXUS_MAX_PATH_LENGTH, cwd_buffer) == 0)
    return path;
#else
  if (getcwd(cwd_buffer, (size_t)NEXUS_MAX_PATH_LENGTH) == NULL)
    return path;
#endif

  if (!nexus_paths_path_canonicalize(cwd_buffer, absolute_cwd))
    return path;

  if (!nexus_paths_path_canonicalize(path.buffer, absolute_path))
    return path;

  return nexus_paths_path_relative_between(absolute_cwd, absolute_path);
}

typedef enum NexusPathListCollectMode {
  NEXUS_PATH_LIST_COLLECT_COUNT,
  NEXUS_PATH_LIST_COLLECT_WRITE
} NexusPathListCollectMode;

typedef struct NexusPathListCollector {
  NexusPathList           *path_list;
  NexusPathListCollectMode mode;
  uint32                   write_index;
} NexusPathListCollector;

static void nexus_paths_path_list_collector_callback(NexusPath path, void *user_data) {
  NexusPathListCollector *collector;
  NexusPathList          *path_list;

  collector = (NexusPathListCollector *)user_data;
  path_list = collector->path_list;

  switch (collector->mode) {
  case NEXUS_PATH_LIST_COLLECT_COUNT:
    path_list->count++;
    break;

  case NEXUS_PATH_LIST_COLLECT_WRITE:
    NEXUS_ASSERT_DEBUG(path_list->paths != NULL);
    NEXUS_ASSERT_DEBUG(collector->write_index < path_list->count);
    path_list->paths[collector->write_index] = path;
    collector->write_index++;
    break;
  }
}

void nexus_paths_path_list_collect(NexusPath path, boolean recursive, boolean files_only, boolean dirs_only, NexusPathList *path_list) {
  NexusPathListCollector collector;

  NEXUS_ASSERT_DEBUG(path_list != NULL);

  collector.path_list   = path_list;
  collector.write_index = 0;

  if (path_list->paths == NULL) {
    path_list->count  = 0;
    collector.mode    = NEXUS_PATH_LIST_COLLECT_COUNT;
    nexus_paths_path_walk(path, nexus_paths_path_list_collector_callback, &collector, recursive, files_only, dirs_only);
    return;
  }

  collector.mode = NEXUS_PATH_LIST_COLLECT_WRITE;
  nexus_paths_path_walk(path, nexus_paths_path_list_collector_callback, &collector, recursive, files_only, dirs_only);
  NEXUS_ASSERT_MESSAGE_DEBUG(collector.write_index == path_list->count, "Path list write count mismatch.");
}

NexusPathList nexus_paths_path_list_collect_allocated(NexusPath path, boolean recursive, boolean files_only, boolean dirs_only) {
  NexusPathList path_list;

  path_list.paths = NULL;
  path_list.count = 0;

  nexus_paths_path_list_collect(path, recursive, files_only, dirs_only, &path_list);
  if (path_list.count == 0) {
    return path_list;
  }

  path_list.paths = (NexusPath *)malloc((size_t)path_list.count * sizeof(NexusPath));
  NEXUS_ASSERT_DEBUG(path_list.paths != NULL);

  nexus_paths_path_list_collect(path, recursive, files_only, dirs_only, &path_list);
  return path_list;
}

void nexus_paths_path_list_destroy(NexusPathList *path_list) {
  NEXUS_ASSERT_DEBUG(path_list != NULL);

  if (path_list->paths != NULL) {
    free(path_list->paths);
  }

  path_list->paths = NULL;
  path_list->count = 0;
}

#if defined(NEXUS_PLATFORM_WINDOWS)

void nexus_paths_path_walk(NexusPath path, NexusPathWalkCallback *callback, void *user_data, boolean recursive, boolean files_only,
                           boolean dirs_only) {
  WIN32_FIND_DATAA find_data;
  HANDLE           find_handle;
  NexusPath        search_path;
  NexusPath        current_path;
  boolean          is_dir;

  NEXUS_ASSERT_DEBUG(callback != NULL);

  search_path = path;
  nexus_paths_path_append(&search_path, "*");

  find_handle = FindFirstFileA(search_path.buffer, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE)
    return;

  do {
    if (find_data.cFileName[0] == '.' && (find_data.cFileName[1] == '\0' || (find_data.cFileName[1] == '.' && find_data.cFileName[2] == '\0'))) {
      continue;
    }

    current_path = path;
    nexus_paths_path_append(&current_path, find_data.cFileName);
    is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;

    if (files_only && is_dir) {
      /* Skip */
    } else if (dirs_only && !is_dir) {
      /* Skip */
    } else {
      callback(current_path, user_data);
    }

    if (is_dir && recursive) {
      nexus_paths_path_walk(current_path, callback, user_data, TRUE, files_only, dirs_only);
    }
  } while (FindNextFileA(find_handle, &find_data) != 0);

  FindClose(find_handle);
}

#else /* POSIX */

void nexus_paths_path_walk(NexusPath path, NexusPathWalkCallback *callback, void *user_data, boolean recursive, boolean files_only,
                           boolean dirs_only) {
  DIR           *dir;
  struct dirent *entry;
  NexusPath      current_path;
  boolean        is_dir;

  NEXUS_ASSERT_DEBUG(callback != NULL);

  dir = opendir(path.buffer);
  if (dir == NULL)
    return;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
      continue;
    }

    current_path = path;
    nexus_paths_path_append(&current_path, entry->d_name);
    is_dir = FALSE;
    (void)nexus_filesystem_path_is_dir(current_path, &is_dir);

    if (files_only && is_dir) { /* NOLINT */
      /* Skip */
    } else if (dirs_only && !is_dir) {
      /* Skip */
    } else {
      callback(current_path, user_data);
    }

    if (is_dir && recursive) {
      nexus_paths_path_walk(current_path, callback, user_data, TRUE, files_only, dirs_only);
    }
  }
  closedir(dir);
}

#endif
