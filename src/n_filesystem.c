#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../nexus.h"
#include "./n_internal.h"

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

  nexus_errors_clear();

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
    NEXUS_ASSERT_MESSAGE_DEBUG(FALSE, "Invalid NexusFileMode");
    c_mode = "r";
    break;
  }

  file = fopen(file_path.buffer, c_mode);
  if (file == NULL) {
    nexus_errors_record_errno();
    return NULL;
  }

  return (NexusFileHandle *)file;
}

void nexus_filesystem_file_close(NexusFileHandle *file_handle) {
  nexus_errors_clear();

  if (file_handle == NULL)
    return;

  if (fclose((FILE *)file_handle) != 0)
    nexus_errors_record_errno();
}

uint_large nexus_filesystem_file_write(NexusFileHandle *file_handle, byte *bytes, uint_large length) {
  size_t     write_length;
  size_t     written_count;
  FILE      *file;

  NEXUS_ASSERT_DEBUG(file_handle != NULL);
  NEXUS_ASSERT_DEBUG(bytes != NULL);

  nexus_errors_clear();

  if (length > (uint_large)((size_t)-1)) {
    nexus_errors_record_code((NexusErrorCode)EINVAL);
    return 0;
  }

  write_length  = (size_t)length;
  file          = (FILE *)file_handle;
  written_count = fwrite(bytes, 1, write_length, file);

  if (written_count != write_length && ferror(file) != 0)
    nexus_errors_record_errno();

  return (uint_large)written_count;
}

void nexus_filesystem_file_flush(NexusFileHandle *file_handle) {
  FILE *file;

  nexus_errors_clear();

  if (file_handle == NULL)
    return;

  file = (FILE *)file_handle;
  if (fflush(file) != 0)
    nexus_errors_record_errno();
}

void nexus_filesystem_file_delete(NexusPath file_path) {
  nexus_errors_clear();

  if (remove(file_path.buffer) != 0 && errno != ENOENT)
    nexus_errors_record_errno();
}

boolean nexus_filesystem_file_rename(NexusPath old_path, NexusPath new_path) {
  nexus_errors_clear();

  if (rename(old_path.buffer, new_path.buffer) == 0)
    return TRUE;

  nexus_errors_record_errno();
  return FALSE;
}

uint_large nexus_filesystem_file_read(NexusFileHandle *file_handle, byte *buffer, uint32 start_byte, uint_large byte_length) {
  size_t read_length;
  size_t read_count;
  FILE  *file;

  NEXUS_ASSERT_DEBUG(file_handle != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL);

  nexus_errors_clear();

  if (byte_length == 0)
    return 0;

  if (byte_length > (uint_large)((size_t)-1)) {
    nexus_errors_record_code((NexusErrorCode)EINVAL);
    return 0;
  }

  read_length = (size_t)byte_length;
  file        = (FILE *)file_handle;

  if (fseek(file, (long)start_byte, SEEK_SET) != 0) {
    nexus_errors_record_errno();
    return 0;
  }

  read_count = fread(buffer, 1, read_length, file);
  if (ferror(file) != 0)
    nexus_errors_record_errno();

  return (uint_large)read_count;
}

#if defined(NEXUS_PLATFORM_WINDOWS)

boolean nexus_filesystem_directory_create(NexusPath directory_path) {
  unsigned long win32_error;

  nexus_errors_clear();

  if (CreateDirectoryA(directory_path.buffer, NULL) != 0)
    return TRUE;

  win32_error = GetLastError();
  if (win32_error == ERROR_ALREADY_EXISTS)
    return TRUE;

  nexus_errors_record_windows_error(win32_error);
  return FALSE;
}

boolean nexus_filesystem_path_exists(NexusPath path) {
  DWORD         attr;
  unsigned long win32_error;

  nexus_errors_clear();

  attr = GetFileAttributesA(path.buffer);
  if (attr != INVALID_FILE_ATTRIBUTES)
    return TRUE;

  win32_error = GetLastError();
  if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND)
    return FALSE;

  nexus_errors_record_windows_error(win32_error);
  return FALSE;
}

boolean nexus_filesystem_path_is_dir(NexusPath path) {
  DWORD         attr;
  unsigned long win32_error;

  nexus_errors_clear();

  attr = GetFileAttributesA(path.buffer);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    win32_error = GetLastError();
    if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND)
      return FALSE;

    nexus_errors_record_windows_error(win32_error);
    return FALSE;
  }

  return (attr & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
}

boolean nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive) {
  WIN32_FIND_DATAA find_data;
  HANDLE           find_handle;
  NexusPath        search_path;
  NexusPath        current_path;
  boolean          is_dir;
  unsigned long    win32_error;

  nexus_errors_clear();

  if (!recursive) {
    if (RemoveDirectoryA(directory_path.buffer) != 0)
      return TRUE;

    win32_error = GetLastError();
    if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND)
      return TRUE;

    nexus_errors_record_windows_error(win32_error);
    return FALSE;
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
        if (!nexus_filesystem_directory_delete(current_path, TRUE))
          return FALSE;
      } else {
        nexus_filesystem_file_delete(current_path);
        if (nexus_errors_occurred())
          return FALSE;
      }
    } while (FindNextFileA(find_handle, &find_data) != 0);
    FindClose(find_handle);
  }

  if (RemoveDirectoryA(directory_path.buffer) != 0)
    return TRUE;

  win32_error = GetLastError();
  if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND)
    return TRUE;

  nexus_errors_record_windows_error(win32_error);
  return FALSE;
}

#else /* POSIX */

boolean nexus_filesystem_directory_create(NexusPath directory_path) {
  nexus_errors_clear();

  if (mkdir(directory_path.buffer, 0777) == 0)
    return TRUE;

  if (errno == EEXIST)
    return TRUE;

  nexus_errors_record_errno();
  return FALSE;
}

boolean nexus_filesystem_path_exists(NexusPath path) {
  nexus_errors_clear();

  if (access(path.buffer, F_OK) == 0)
    return TRUE;

  if (errno == ENOENT)
    return FALSE;

  nexus_errors_record_errno();
  return FALSE;
}

boolean nexus_filesystem_path_is_dir(NexusPath path) {
  struct stat path_stat;

  nexus_errors_clear();

  if (stat(path.buffer, &path_stat) != 0) {
    if (errno == ENOENT)
      return FALSE;

    nexus_errors_record_errno();
    return FALSE;
  }

  return S_ISDIR(path_stat.st_mode) ? TRUE : FALSE;
}

boolean nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive) {
  DIR           *dir;
  struct dirent *entry;
  NexusPath      current_path;
  boolean        is_dir;

  nexus_errors_clear();

  if (!recursive) {
    if (rmdir(directory_path.buffer) == 0)
      return TRUE;

    if (errno == ENOENT)
      return TRUE;

    nexus_errors_record_errno();
    return FALSE;
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
        if (!nexus_filesystem_directory_delete(current_path, TRUE))
          return FALSE;
      } else {
        nexus_filesystem_file_delete(current_path);
        if (nexus_errors_occurred())
          return FALSE;
      }
    }
    closedir(dir);
  } else if (errno != ENOENT) {
    nexus_errors_record_errno();
    return FALSE;
  }

  if (rmdir(directory_path.buffer) == 0)
    return TRUE;

  if (errno == ENOENT)
    return TRUE;

  nexus_errors_record_errno();
  return FALSE;
}

#endif
