#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../nexus.h"

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <windows.h>
#elif NEXUS_PLATFORM_POSIX
#  include <dirent.h>
#  include <unistd.h>
#else
#  error "Unsupported platform"
#endif

#include "./n_internal.h"

NError nexus_filesystem_file_open(NexusPath file_path, NexusFileMode mode, NexusFileHandle **out_file_handle) {
  const char *c_mode;
  FILE       *file;

  NEXUS_ASSERT_DEBUG(out_file_handle != NULL);

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
  if (file == NULL)
    return nexus_errors_from_errno();

  *out_file_handle = (NexusFileHandle *)file;
  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_close(NexusFileHandle *file_handle) {
  if (file_handle == NULL)
    return NEXUS_ERROR_NONE;

  if (fclose((FILE *)file_handle) != 0)
    return nexus_errors_from_errno();

  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_write(NexusFileHandle *file_handle, const byte *bytes, uint_large length, uint_large *out_bytes_written) {
  size_t written_count;
  FILE  *file;

  NEXUS_ASSERT_DEBUG(file_handle != NULL);
  NEXUS_ASSERT_DEBUG(bytes != NULL);
  NEXUS_ASSERT_DEBUG(out_bytes_written != NULL);

  if (length > (uint_large)((size_t)-1)) {
    *out_bytes_written = 0;
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  file               = (FILE *)file_handle;
  written_count      = fwrite(bytes, 1, (size_t)length, file);
  *out_bytes_written = (uint_large)written_count;

  if (written_count != (size_t)length && ferror(file) != 0)
    return nexus_errors_from_errno();

  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_flush(NexusFileHandle *file_handle) {
  FILE *file;

  if (file_handle == NULL)
    return NEXUS_ERROR_NONE;

  file = (FILE *)file_handle;
  if (fflush(file) != 0)
    return nexus_errors_from_errno();

  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_delete(NexusPath file_path) {
  if (remove(file_path.buffer) != 0 && errno != ENOENT)
    return nexus_errors_from_errno();

  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_rename(NexusPath old_path, NexusPath new_path) {
  if (rename(old_path.buffer, new_path.buffer) == 0)
    return NEXUS_ERROR_NONE;

  return nexus_errors_from_errno();
}

NError nexus_filesystem_file_ensure_parent_directory(NexusPath file_path) {
  NexusPath parent_path;

  if (file_path.length == 0u || file_path.buffer[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  parent_path = nexus_paths_path_parent(file_path);
  if (parent_path.length == 0u || parent_path.buffer[0] == '\0') {
    return NEXUS_ERROR_NONE;
  }

  return nexus_filesystem_directory_create_parents(parent_path);
}

#ifndef NEXUS_FILESYSTEM_COPY_CHUNK_SIZE
#  define NEXUS_FILESYSTEM_COPY_CHUNK_SIZE (65536u)
#endif

NError nexus_filesystem_file_copy(NexusPath source_path, NexusPath destination_path) {
  NexusFileHandle *source_handle;
  NexusFileHandle *destination_handle;
  FILE            *source_file;
  byte             chunk[NEXUS_FILESYSTEM_COPY_CHUNK_SIZE];
  boolean          source_exists;
  NError           status;

  if (source_path.length == 0u || destination_path.length == 0u || source_path.buffer[0] == '\0' || destination_path.buffer[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (nexus_strings_string_equals(source_path.buffer, destination_path.buffer) == TRUE) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  status = nexus_filesystem_path_exists(source_path, &source_exists);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }
  if (source_exists != TRUE) {
    return NEXUS_ERROR_FILE_NOT_FOUND;
  }

  status = nexus_filesystem_file_ensure_parent_directory(destination_path);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  source_handle = NULL;
  status        = nexus_filesystem_file_open(source_path, NFM_READ_BINARY, &source_handle);
  if (status != NEXUS_ERROR_NONE || source_handle == NULL) {
    return status;
  }

  destination_handle = NULL;
  status             = nexus_filesystem_file_open(destination_path, NFM_WRITE_BINARY, &destination_handle);
  if (status != NEXUS_ERROR_NONE || destination_handle == NULL) {
    (void)nexus_filesystem_file_close(source_handle);
    return status;
  }

  source_file = (FILE *)source_handle;
  for (;;) {
    size_t     read_count;
    uint_large written_count;

    read_count = fread(chunk, 1, (size_t)NEXUS_FILESYSTEM_COPY_CHUNK_SIZE, source_file);
    if (read_count > 0u) {
      written_count = 0u;
      status        = nexus_filesystem_file_write(destination_handle, chunk, (uint_large)read_count, &written_count);
      if (status != NEXUS_ERROR_NONE || written_count != (uint_large)read_count) {
        if (status == NEXUS_ERROR_NONE) {
          status = NEXUS_ERROR_IO;
        }
        (void)nexus_filesystem_file_close(destination_handle);
        (void)nexus_filesystem_file_close(source_handle);
        return status;
      }
    }

    if (read_count < (size_t)NEXUS_FILESYSTEM_COPY_CHUNK_SIZE) {
      if (ferror(source_file) != 0) {
        status = nexus_errors_from_errno();
        (void)nexus_filesystem_file_close(destination_handle);
        (void)nexus_filesystem_file_close(source_handle);
        return status;
      }
      break;
    }
  }

  status = nexus_filesystem_file_flush(destination_handle);
  (void)nexus_filesystem_file_close(destination_handle);
  (void)nexus_filesystem_file_close(source_handle);
  return status;
}

NError nexus_filesystem_file_read(NexusFileHandle *file_handle, byte *buffer, uint32 start_byte, uint_large byte_length, uint_large *out_bytes_read) {
  size_t read_count;
  FILE  *file;

  NEXUS_ASSERT_DEBUG(file_handle != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_bytes_read != NULL);

  if (byte_length == 0) {
    *out_bytes_read = 0;
    return NEXUS_ERROR_NONE;
  }

  if (byte_length > (uint_large)((size_t)-1)) {
    *out_bytes_read = 0;
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  file = (FILE *)file_handle;

  if (fseek(file, (long)start_byte, SEEK_SET) != 0) {
    *out_bytes_read = 0;
    return nexus_errors_from_errno();
  }

  read_count      = fread(buffer, 1, (size_t)byte_length, file);
  *out_bytes_read = (uint_large)read_count;

  if (ferror(file) != 0)
    return nexus_errors_from_errno();

  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_read_line(NexusFileHandle *file_handle, char *buffer, uint_large buffer_max_length, uint_large *out_bytes_read) {
  FILE *file;

  NEXUS_ASSERT_DEBUG(file_handle != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_bytes_read != NULL);

  if (buffer_max_length == 0) {
    *out_bytes_read = 0;
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  file = (FILE *)file_handle;
  if (fgets(buffer, (int)buffer_max_length, file) == NULL) {
    *out_bytes_read = 0;
    if (feof(file) != 0) {
      return NEXUS_ERROR_NONE;
    }
    return nexus_errors_from_errno();
  }

  *out_bytes_read = nexus_strings_string_length(buffer);
  if (*out_bytes_read > 0 && buffer[*out_bytes_read - 1] == '\n') {
    buffer[*out_bytes_read - 1] = '\0';
    (*out_bytes_read)--;
  }

  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_temp_directory_get(char *buffer, uint_large buffer_max_length) {
  NexusStringFormatResult copy_result;
  NError                  environment_error;

  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_MESSAGE_DEBUG(buffer_max_length > 0, "temporary directory buffer must be non-zero");

#if defined(NEXUS_PLATFORM_WINDOWS)
  {
    DWORD copied_length;

    copied_length = GetTempPathA((DWORD)buffer_max_length, buffer);
    if (copied_length == 0 || copied_length >= (DWORD)buffer_max_length) {
      return NEXUS_ERROR_IO;
    }

    return NEXUS_ERROR_NONE;
  }
#else
  environment_error = nexus_environment_variable_get("TMPDIR", buffer, buffer_max_length);
  if (environment_error == NEXUS_ERROR_NONE) {
    return NEXUS_ERROR_NONE;
  }

  environment_error = nexus_environment_variable_get("TMP", buffer, buffer_max_length);
  if (environment_error == NEXUS_ERROR_NONE) {
    return NEXUS_ERROR_NONE;
  }

  environment_error = nexus_environment_variable_get("TEMP", buffer, buffer_max_length);
  if (environment_error == NEXUS_ERROR_NONE) {
    return NEXUS_ERROR_NONE;
  }

  copy_result = nexus_strings_string_copy_exact(buffer, buffer_max_length, "/tmp");
  if (copy_result.success == FALSE) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
#endif
}

#if defined(NEXUS_PLATFORM_WINDOWS)

NError nexus_filesystem_directory_visit(NexusPath directory_path, NexusFilesystemDirectoryVisitCallback *callback, void *user_data) {
  WIN32_FIND_DATAA find_data;
  HANDLE           find_handle;
  NexusPath        search_path;
  unsigned long    win32_error;
  NError           error;

  if (callback == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }
  search_path = directory_path;
  nexus_paths_path_append(&search_path, "*");
  find_handle = FindFirstFileA(search_path.buffer, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    win32_error = GetLastError();
    if (win32_error == ERROR_FILE_NOT_FOUND) {
      return NEXUS_ERROR_NONE;
    }
    return nexus_errors_from_windows_error(win32_error);
  }
  error = NEXUS_ERROR_NONE;
  do {
    NexusPath entry_path;

    if (find_data.cFileName[0] == '.' && (find_data.cFileName[1] == '\0' || (find_data.cFileName[1] == '.' && find_data.cFileName[2] == '\0'))) {
      continue;
    }
    entry_path = directory_path;
    nexus_paths_path_append(&entry_path, find_data.cFileName);
    error = callback(entry_path, user_data);
    if (error != NEXUS_ERROR_NONE) {
      break;
    }
  } while (FindNextFileA(find_handle, &find_data) != 0);
  if (error == NEXUS_ERROR_NONE) {
    win32_error = GetLastError();
    if (win32_error != ERROR_NO_MORE_FILES) {
      error = nexus_errors_from_windows_error(win32_error);
    }
  }
  FindClose(find_handle);
  return error;
}

NError nexus_filesystem_directory_create(NexusPath directory_path) {
  unsigned long win32_error;

  if (CreateDirectoryA(directory_path.buffer, NULL) != 0)
    return NEXUS_ERROR_NONE;

  win32_error = GetLastError();
  if (win32_error == ERROR_ALREADY_EXISTS)
    return NEXUS_ERROR_NONE;

  return nexus_errors_from_windows_error(win32_error);
}

NError nexus_filesystem_path_exists(NexusPath path, boolean *out_exists) {
  DWORD         attr;
  unsigned long win32_error;

  NEXUS_ASSERT_DEBUG(out_exists != NULL);

  attr = GetFileAttributesA(path.buffer);
  if (attr != INVALID_FILE_ATTRIBUTES) {
    *out_exists = TRUE;
    return NEXUS_ERROR_NONE;
  }

  win32_error = GetLastError();
  if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND) {
    *out_exists = FALSE;
    return NEXUS_ERROR_NONE;
  }

  *out_exists = FALSE;
  return nexus_errors_from_windows_error(win32_error);
}

NError nexus_filesystem_path_is_dir(NexusPath path, boolean *out_is_dir) {
  DWORD         attr;
  unsigned long win32_error;

  NEXUS_ASSERT_DEBUG(out_is_dir != NULL);

  attr = GetFileAttributesA(path.buffer);
  if (attr == INVALID_FILE_ATTRIBUTES) {
    win32_error = GetLastError();
    if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND) {
      *out_is_dir = FALSE;
      return NEXUS_ERROR_NONE;
    }

    *out_is_dir = FALSE;
    return nexus_errors_from_windows_error(win32_error);
  }

  *out_is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_size_get(NexusPath file_path, uint_large *out_byte_size) {
  WIN32_FILE_ATTRIBUTE_DATA file_info;

  NEXUS_ASSERT_DEBUG(out_byte_size != NULL);

  if (GetFileAttributesExA(file_path.buffer, GetFileExInfoStandard, &file_info) == 0) {
    *out_byte_size = 0;
    return nexus_errors_from_windows_error(GetLastError());
  }

  if ((file_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    *out_byte_size = 0;
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_byte_size = ((uint_large)file_info.nFileSizeHigh << 32) | (uint_large)file_info.nFileSizeLow;
  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive) {
  WIN32_FIND_DATAA find_data;
  HANDLE           find_handle;
  NexusPath        search_path;
  NexusPath        current_path;
  boolean          is_dir;
  NError           status;
  unsigned long    win32_error;

  if (!recursive) {
    if (RemoveDirectoryA(directory_path.buffer) != 0)
      return NEXUS_ERROR_NONE;

    win32_error = GetLastError();
    if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND)
      return NEXUS_ERROR_NONE;

    return nexus_errors_from_windows_error(win32_error);
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
        status = nexus_filesystem_directory_delete(current_path, TRUE);
        if (status != NEXUS_ERROR_NONE)
          return status;
      } else {
        status = nexus_filesystem_file_delete(current_path);
        if (status != NEXUS_ERROR_NONE)
          return status;
      }
    } while (FindNextFileA(find_handle, &find_data) != 0);
    FindClose(find_handle);
  }

  if (RemoveDirectoryA(directory_path.buffer) != 0)
    return NEXUS_ERROR_NONE;

  win32_error = GetLastError();
  if (win32_error == ERROR_FILE_NOT_FOUND || win32_error == ERROR_PATH_NOT_FOUND)
    return NEXUS_ERROR_NONE;

  return nexus_errors_from_windows_error(win32_error);
}

#else /* POSIX */

NError nexus_filesystem_directory_visit(NexusPath directory_path, NexusFilesystemDirectoryVisitCallback *callback, void *user_data) {
  DIR           *directory;
  struct dirent *entry;
  NError         error;

  if (callback == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }
  directory = opendir(directory_path.buffer);
  if (directory == NULL) {
    return nexus_errors_from_errno();
  }
  error = NEXUS_ERROR_NONE;
  for (;;) {
    NexusPath entry_path;

    errno = 0;
    entry = readdir(directory);
    if (entry == NULL) {
      if (errno != 0) {
        error = nexus_errors_from_errno();
      }
      break;
    }
    if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
      continue;
    }
    entry_path = directory_path;
    nexus_paths_path_append(&entry_path, entry->d_name);
    error = callback(entry_path, user_data);
    if (error != NEXUS_ERROR_NONE) {
      break;
    }
  }
  if (closedir(directory) != 0 && error == NEXUS_ERROR_NONE) {
    error = nexus_errors_from_errno();
  }
  return error;
}

NError nexus_filesystem_directory_create(NexusPath directory_path) {
  if (mkdir(directory_path.buffer, 0777) == 0)
    return NEXUS_ERROR_NONE;

  if (errno == EEXIST)
    return NEXUS_ERROR_NONE;

  return nexus_errors_from_errno();
}

NError nexus_filesystem_path_exists(NexusPath path, boolean *out_exists) {
  NEXUS_ASSERT_DEBUG(out_exists != NULL);

  if (access(path.buffer, F_OK) == 0) {
    *out_exists = TRUE;
    return NEXUS_ERROR_NONE;
  }

  if (errno == ENOENT) {
    *out_exists = FALSE;
    return NEXUS_ERROR_NONE;
  }

  *out_exists = FALSE;
  return nexus_errors_from_errno();
}

NError nexus_filesystem_path_is_dir(NexusPath path, boolean *out_is_dir) {
  struct stat path_stat;

  NEXUS_ASSERT_DEBUG(out_is_dir != NULL);

  if (stat(path.buffer, &path_stat) != 0) {
    if (errno == ENOENT) {
      *out_is_dir = FALSE;
      return NEXUS_ERROR_NONE;
    }

    *out_is_dir = FALSE;
    return nexus_errors_from_errno();
  }

  *out_is_dir = S_ISDIR(path_stat.st_mode) ? TRUE : FALSE;
  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_file_size_get(NexusPath file_path, uint_large *out_byte_size) {
  struct stat path_stat;

  NEXUS_ASSERT_DEBUG(out_byte_size != NULL);

  if (stat(file_path.buffer, &path_stat) != 0) {
    *out_byte_size = 0;
    return nexus_errors_from_errno();
  }

  if (!S_ISREG(path_stat.st_mode)) {
    *out_byte_size = 0;
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_byte_size = (uint_large)path_stat.st_size;
  return NEXUS_ERROR_NONE;
}

NError nexus_filesystem_directory_delete(NexusPath directory_path, boolean recursive) {
  DIR           *dir;
  struct dirent *entry;
  NexusPath      current_path;
  boolean        is_dir;
  NError         status;

  if (!recursive) {
    if (rmdir(directory_path.buffer) == 0)
      return NEXUS_ERROR_NONE;

    if (errno == ENOENT)
      return NEXUS_ERROR_NONE;

    return nexus_errors_from_errno();
  }

  dir = opendir(directory_path.buffer);
  if (dir != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
        continue;
      }

      current_path = directory_path;
      nexus_paths_path_append(&current_path, entry->d_name);
      status = nexus_filesystem_path_is_dir(current_path, &is_dir);
      if (status != NEXUS_ERROR_NONE)
        is_dir = FALSE;

      if (is_dir) {
        status = nexus_filesystem_directory_delete(current_path, TRUE);
        if (status != NEXUS_ERROR_NONE)
          return status;
      } else {
        status = nexus_filesystem_file_delete(current_path);
        if (status != NEXUS_ERROR_NONE)
          return status;
      }
    }
    closedir(dir);
  } else if (errno != ENOENT) {
    return nexus_errors_from_errno();
  }

  if (rmdir(directory_path.buffer) == 0)
    return NEXUS_ERROR_NONE;

  if (errno == ENOENT)
    return NEXUS_ERROR_NONE;

  return nexus_errors_from_errno();
}

#endif

NError nexus_filesystem_directory_create_parents(NexusPath directory_path) {
  NexusPath parent_path;
  boolean   directory_exists;
  boolean   is_dir;
  NError    status;

  if (directory_path.length == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  status = nexus_filesystem_path_exists(directory_path, &directory_exists);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (directory_exists == TRUE) {
    status = nexus_filesystem_path_is_dir(directory_path, &is_dir);
    if (status != NEXUS_ERROR_NONE) {
      return status;
    }

    if (is_dir == FALSE) {
      return NEXUS_ERROR_ALREADY_EXISTS;
    }

    return NEXUS_ERROR_NONE;
  }

  parent_path = nexus_paths_path_parent(directory_path);
  if (nexus_strings_string_equals(parent_path.buffer, ".") == FALSE &&
      nexus_strings_string_equals(parent_path.buffer, directory_path.buffer) == FALSE) {
    status = nexus_filesystem_directory_create_parents(parent_path);
    if (status != NEXUS_ERROR_NONE) {
      return status;
    }
  }

  return nexus_filesystem_directory_create(directory_path);
}
