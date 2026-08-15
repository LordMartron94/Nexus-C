#include "../nexus.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <io.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <unistd.h>
#else
#  error "Unsupported platform"
#endif

NError nexus_stdio_stdout_write(const byte *bytes, uint_large byte_length, uint_large *out_bytes_written) {
  const byte *write_cursor;
  size_t      chunk_length;
  size_t      written_count;
  uint_large  remaining_bytes;

  NEXUS_ASSERT_DEBUG(bytes != NULL);
  NEXUS_ASSERT_DEBUG(out_bytes_written != NULL);

  *out_bytes_written = 0;
  if (byte_length == 0) {
    return NEXUS_ERROR_NONE;
  }

  write_cursor    = bytes;
  remaining_bytes = byte_length;
  while (remaining_bytes > 0) {
    chunk_length  = (size_t)((remaining_bytes > (uint_large)((size_t)-1)) ? (size_t)-1 : remaining_bytes);
    written_count = fwrite(write_cursor, 1, chunk_length, stdout);
    if (written_count != chunk_length) {
      if (ferror(stdout) != 0) {
        return NEXUS_ERROR_IO;
      }
      return NEXUS_ERROR_IO;
    }

    *out_bytes_written += (uint_large)written_count;
    write_cursor += written_count;
    remaining_bytes -= (uint_large)written_count;
  }

  return NEXUS_ERROR_NONE;
}

NError nexus_stdio_stdout_write_cstring(const char *text) {
  uint_large byte_length;
  uint_large bytes_written;

  if (text == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  byte_length = nexus_strings_string_length(text);
  return nexus_stdio_stdout_write((const byte *)text, byte_length, &bytes_written);
}

NError nexus_stdio_stdout_write_formatted(const char *format, ...) {
  char                    message_buffer[4096];
  va_list                 arguments;
  NexusStringFormatResult format_result;

  if (format == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  va_start(arguments, format);
  format_result = nexus_strings_vstring_format_with_truncation(message_buffer, (uint_large)NEXUS_SIZEOF(message_buffer), format, arguments);
  va_end(arguments);

  if (format_result.success == FALSE) {
    return NEXUS_ERROR_IO;
  }

  return nexus_stdio_stdout_write_cstring(message_buffer);
}

NError nexus_stdio_stdout_flush(void) {
  if (fflush(stdout) != 0) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
}

NError nexus_stdio_stderr_write(const byte *bytes, uint_large byte_length, uint_large *out_bytes_written) {
  const byte *write_cursor;
  size_t      chunk_length;
  size_t      written_count;
  uint_large  remaining_bytes;

  NEXUS_ASSERT_DEBUG(bytes != NULL);
  NEXUS_ASSERT_DEBUG(out_bytes_written != NULL);

  *out_bytes_written = 0;
  if (byte_length == 0) {
    return NEXUS_ERROR_NONE;
  }

  write_cursor    = bytes;
  remaining_bytes = byte_length;
  while (remaining_bytes > 0) {
    chunk_length  = (size_t)((remaining_bytes > (uint_large)((size_t)-1)) ? (size_t)-1 : remaining_bytes);
    written_count = fwrite(write_cursor, 1, chunk_length, stderr);
    if (written_count != chunk_length) {
      if (ferror(stderr) != 0) {
        return NEXUS_ERROR_IO;
      }
      return NEXUS_ERROR_IO;
    }

    *out_bytes_written += (uint_large)written_count;
    write_cursor += written_count;
    remaining_bytes -= (uint_large)written_count;
  }

  return NEXUS_ERROR_NONE;
}

NError nexus_stdio_stderr_write_cstring(const char *text) {
  uint_large byte_length;
  uint_large bytes_written;

  if (text == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  byte_length = nexus_strings_string_length(text);
  return nexus_stdio_stderr_write((const byte *)text, byte_length, &bytes_written);
}

NError nexus_stdio_stderr_write_formatted(const char *format, ...) {
  char                    message_buffer[4096];
  va_list                 arguments;
  NexusStringFormatResult format_result;

  if (format == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  va_start(arguments, format);
  format_result = nexus_strings_vstring_format_with_truncation(message_buffer, (uint_large)NEXUS_SIZEOF(message_buffer), format, arguments);
  va_end(arguments);

  if (format_result.success == FALSE) {
    return NEXUS_ERROR_IO;
  }

  return nexus_stdio_stderr_write_cstring(message_buffer);
}

NError nexus_stdio_stderr_flush(void) {
  if (fflush(stderr) != 0) {
    return NEXUS_ERROR_IO;
  }

  return NEXUS_ERROR_NONE;
}

boolean nexus_stdio_stdin_is_terminal(void) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  return _isatty(_fileno(stdin)) != 0;
#else
  return isatty(STDIN_FILENO) != 0;
#endif
}

NError nexus_stdio_stdin_read_line(char *buffer, uint_large buffer_max_length, boolean *out_reached_eof) {
  uint_large length;

  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(out_reached_eof != NULL);
  NEXUS_ASSERT_MESSAGE_DEBUG(buffer_max_length > 0, "stdin read buffer must be non-zero");

  (void)nexus_stdio_stdout_flush();

  while (TRUE) {
    if (fgets(buffer, (int)buffer_max_length, stdin) != NULL) {
      *out_reached_eof = FALSE;

      length = nexus_strings_string_length(buffer);

      if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        length--;
      }

      if (length > 0 && buffer[length - 1] == '\r') {
        buffer[length - 1] = '\0';
      }

      return NEXUS_ERROR_NONE;
    }

#if defined(EINTR)
    if (errno == EINTR) {
      continue;
    }
#endif

    *out_reached_eof = TRUE;
    return NEXUS_ERROR_NONE;
  }
}
