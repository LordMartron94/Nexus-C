#include "../nexus.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <windows.h>
#elif defined(NEXUS_PLATFORM_LINUX) || defined(NEXUS_PLATFORM_MACOS)
#  include <dirent.h>
#  include <unistd.h>
#else
#  error "Unsupported platform"
#endif

NexusPath nexus_paths_path_create(const char *base_path) {
  NexusPath path;
  uint16    i = 0;

  while (base_path && base_path[i] && i < NEXUS_MAX_PATH_LENGTH - 1) {
    path.buffer[i] = base_path[i];
    i++;
  }
  path.buffer[i] = '\0';
  path.length    = i;

  return path;
}

void nexus_paths_path_append(NexusPath *path, const char *element) {
  uint16 i = 0;

  if (path->length == 0 || path->length >= NEXUS_MAX_PATH_LENGTH - 2)
    return;

  if (path->buffer[path->length - 1] != '/' && path->buffer[path->length - 1] != '\\') {
#if defined(NEXUS_PLATFORM_WINDOWS)
    path->buffer[path->length] = '\\';
#else
    path->buffer[path->length] = '/';
#endif
    path->length++;
  }

  while (element && element[i] && path->length < NEXUS_MAX_PATH_LENGTH - 1) {
    path->buffer[path->length] = element[i];
    path->length++;
    i++;
  }
  path->buffer[path->length] = '\0';
}

const char *nexus_paths_path_base_name_get(const NexusPath *path) {
  const char *base = path->buffer;
  const char *ptr  = path->buffer;

  while (*ptr) {
    if (*ptr == '/' || *ptr == '\\') {
      base = ptr + 1;
    }
    ptr++;
  }
  return base;
}

#if defined(NEXUS_PLATFORM_WINDOWS)

void nexus_paths_path_walk(NexusPath path, NexusPathWalkCallback *callback, void *user_data, boolean recursive, boolean files_only,
                           boolean dirs_only) {
  WIN32_FIND_DATAA find_data;
  HANDLE           find_handle;
  NexusPath        search_path;
  NexusPath        current_path;
  boolean          is_dir;

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

  dir = opendir(path.buffer);
  if (dir == NULL)
    return;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
      continue;
    }

    current_path = path;
    nexus_paths_path_append(&current_path, entry->d_name);
    is_dir = nexus_filesystem_path_is_dir(current_path);

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
  }
  closedir(dir);
}

#endif
