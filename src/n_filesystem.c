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

NexusFileHandle *nexus_filesystem_file_open(NexusPath file_path, NexusFileMode mode) {
  const char *c_mode;
  FILE       *file;

  switch (mode) {
  case NFM_READ:
    c_mode = "r";
    break;
  case NFM_READ_BINARY:
    c_mode = "rb";
    break;
  case NFM_WRITE:
    c_mode = "w";
    break;
  case NFM_WRITE_BINARY:
    c_mode = "wb";
    break;
  case NFM_APPEND:
    c_mode = "a";
    break;
  case NFM_APPEND_BINARY:
    c_mode = "ab";
    break;
  case NFM_READ_PLUS:
    c_mode = "r+";
    break;
  case NFM_READ_BINARY_PLUS:
    c_mode = "rb+";
    break;
  case NFM_WRITE_PLUS:
    c_mode = "w+";
    break;
  case NFM_WRITE_BINARY_PLUS:
    c_mode = "wb+";
    break;
  case NFM_APPEND_PLUS:
    c_mode = "a+";
    break;
  case NFM_APPEND_BINARY_PLUS:
    c_mode = "ab+";
    break;
  default:
    c_mode = "r";
    break;
  }

  file = fopen(file_path.buffer, c_mode);
  return (NexusFileHandle *)file;
}

void nexus_filesystem_file_close(NexusFileHandle *file_handle) {
  if (file_handle) {
    (void)fclose((FILE *)file_handle);
  }
}

uint64 nexus_filesystem_file_write(NexusFileHandle *file_handle, byte *bytes, uint64 length) {
  if (!file_handle || !bytes)
    return 0;
  return (uint64)fwrite(bytes, 1, (size_t)length, (FILE *)file_handle);
}

void nexus_filesystem_file_flush(NexusFileHandle *file_handle) {
  if (file_handle) {
    (void)fflush((FILE *)file_handle);
  }
}

void nexus_filesystem_file_delete(NexusPath file_path) {
  (void)remove(file_path.buffer);
}

boolean nexus_filesystem_file_rename(NexusPath old_path, NexusPath new_path) {
  return rename(old_path.buffer, new_path.buffer) == 0 ? TRUE : FALSE;
}

#if defined(NEXUS_PLATFORM_WINDOWS)

boolean nexus_filesystem_directory_create(NexusPath directory_path) {
  if (CreateDirectoryA(directory_path.buffer, NULL) != 0)
    return TRUE;
  if (GetLastError() == ERROR_ALREADY_EXISTS)
    return TRUE;
  return FALSE;
}

boolean nexus_filesystem_path_exists(NexusPath path) {
  DWORD attr = GetFileAttributesA(path.buffer);
  return (attr != INVALID_FILE_ATTRIBUTES) ? TRUE : FALSE;
}

boolean nexus_filesystem_path_is_dir(NexusPath path) {
  DWORD attr = GetFileAttributesA(path.buffer);
  if (attr == INVALID_FILE_ATTRIBUTES)
    return FALSE;
  return (attr & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
}

boolean nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive) {
  WIN32_FIND_DATAA find_data;
  HANDLE           find_handle;
  NexusPath        search_path;
  NexusPath        current_path;
  boolean          is_dir;

  if (!recursive) {
    return RemoveDirectoryA(directory_path.buffer) != 0 ? TRUE : FALSE;
  }

  search_path = directory_path;
  nexus_paths_path_append(&search_path, "*");

  find_handle = FindFirstFileA(search_path.buffer, &find_data);
  if (find_handle != INVALID_HANDLE_VALUE) {
    do {
      if (find_data.cFileName[0] == '.' && (find_data.cFileName[1] == '\0' || (find_data.cFileName[1] == '.' && find_data.cFileName[2] == '\0'))) {
        continue;
      }

      current_path = directory_path;
      nexus_paths_path_append(&current_path, find_data.cFileName);
      is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;

      if (is_dir) {
        nexus_filesystem_directory_delete(current_path, TRUE);
      } else {
        nexus_filesystem_file_delete(current_path);
      }
    } while (FindNextFileA(find_handle, &find_data) != 0);
    FindClose(find_handle);
  }

  return RemoveDirectoryA(directory_path.buffer) != 0 ? TRUE : FALSE;
}

#else /* POSIX */

boolean nexus_filesystem_directory_create(NexusPath directory_path) {
  if (mkdir(directory_path.buffer, 0777) == 0)
    return TRUE;
  return FALSE;
}

boolean nexus_filesystem_path_exists(NexusPath path) {
  return access(path.buffer, F_OK) == 0 ? TRUE : FALSE;
}

boolean nexus_filesystem_path_is_dir(NexusPath path) {
  struct stat path_stat;
  if (stat(path.buffer, &path_stat) != 0)
    return FALSE;
  return S_ISDIR(path_stat.st_mode) ? TRUE : FALSE;
}

boolean nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive) {
  DIR           *dir;
  struct dirent *entry;
  NexusPath      current_path;
  boolean        is_dir;

  if (!recursive) {
    return rmdir(directory_path.buffer) == 0 ? TRUE : FALSE;
  }

  dir = opendir(directory_path.buffer);
  if (dir != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
        continue;
      }

      current_path = directory_path;
      nexus_paths_path_append(&current_path, entry->d_name);
      is_dir = nexus_filesystem_path_is_dir(current_path);

      if (is_dir) {
        nexus_filesystem_directory_delete(current_path, TRUE);
      } else {
        nexus_filesystem_file_delete(current_path);
      }
    }
    closedir(dir);
  }

  return rmdir(directory_path.buffer) == 0 ? TRUE : FALSE;
}

#endif
