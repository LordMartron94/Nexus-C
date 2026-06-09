#include <stdarg.h>
#include "../nexus.h"

/*
Implementation written on 9 June 2026 by Gemini AI Pro.
*/

static uint64 core_format_engine(char *buffer, uint64 max_len, const char *format, va_list args) {
  uint64      written       = 0;
  const char *format_cursor = format;

  while (*format_cursor) {
    int left_justify  = 0;
    int zero_pad      = 0;
    int width         = 0;
    int has_precision = 0;
    int precision     = 0;
    int i;

    if (*format_cursor != '%') {
      if (buffer && written < max_len - 1)
        buffer[written] = *format_cursor;
      written++;
      format_cursor++;
      continue;
    }

    format_cursor++; /* Skip '%' */

    if (*format_cursor == '\0') {
      break;
    }

    /* 1. Parse Flags */
    while (*format_cursor == '-' || *format_cursor == '0') {
      if (*format_cursor == '-')
        left_justify = 1;
      if (*format_cursor == '0')
        zero_pad = 1;
      format_cursor++;
    }

    /* 2. Parse Width */
    if (*format_cursor == '*') {
      width = va_arg(args, int);
      if (width < 0) {
        left_justify = 1;
        width        = -width;
      }
      format_cursor++;
    } else {
      while (*format_cursor >= '0' && *format_cursor <= '9') {
        width = (width * 10) + (*format_cursor - '0');
        format_cursor++;
      }
    }

    /* 3. Parse Precision */
    if (*format_cursor == '.') {
      has_precision = 1;
      format_cursor++;
      if (*format_cursor == '*') {
        precision = va_arg(args, int);
        if (precision < 0)
          has_precision = 0;
        format_cursor++;
      } else {
        while (*format_cursor >= '0' && *format_cursor <= '9') {
          precision = (precision * 10) + (*format_cursor - '0');
          format_cursor++;
        }
      }
    }

    /* 4. Evaluate Specifier */
    if (*format_cursor == '%') {
      if (buffer && written < max_len - 1)
        buffer[written] = '%';
      written++;
    } else if (*format_cursor == 'c') {
      char character = (char)va_arg(args, int);
      if (!left_justify) {
        for (i = 1; i < width; i++) {
          if (buffer && written < max_len - 1)
            buffer[written] = ' ';
          written++;
        }
      }
      if (buffer && written < max_len - 1)
        buffer[written] = character;
      written++;
      if (left_justify) {
        for (i = 1; i < width; i++) {
          if (buffer && written < max_len - 1)
            buffer[written] = ' ';
          written++;
        }
      }
    } else if (*format_cursor == 's') {
      const char *string = va_arg(args, const char *);
      int         len    = 0;
      if (!string)
        string = "(null)";

      /* Calculate string length honoring precision boundaries */
      while (string[len] && (!has_precision || len < precision)) {
        len++;
      }

      if (!left_justify) {
        for (i = len; i < width; i++) {
          if (buffer && written < max_len - 1)
            buffer[written] = ' ';
          written++;
        }
      }
      for (i = 0; i < len; i++) {
        if (buffer && written < max_len - 1)
          buffer[written] = string[i];
        written++;
      }
      if (left_justify) {
        for (i = len; i < width; i++) {
          if (buffer && written < max_len - 1)
            buffer[written] = ' ';
          written++;
        }
      }
    } else if (*format_cursor == 'd' || *format_cursor == 'i' || *format_cursor == 'u' || *format_cursor == 'x') {
      char          num_buf[32];
      int           num_len = 0;
      unsigned long uval    = 0;
      int           is_neg  = 0;
      int           base    = (*format_cursor == 'x') ? 16 : 10;
      int           zeros   = 0;
      int           total_len;

      if (*format_cursor == 'd' || *format_cursor == 'i') {
        int ival = va_arg(args, int);
        if (ival < 0) {
          is_neg = 1;
          uval   = (unsigned long)(-ival);
        } else {
          uval = (unsigned long)ival;
        }
      } else {
        uval = (unsigned long)va_arg(args, unsigned int);
      }

      if (uval == 0) {
        num_buf[num_len++] = '0';
      } else {
        while (uval > 0) {
          unsigned long rem  = uval % base;
          num_buf[num_len++] = (rem < 10) ? (char)('0' + rem) : (char)('a' + rem - 10);
          uval /= base;
        }
      }

      /* Resolve precision and zero padding conflicts */
      if (has_precision) {
        zero_pad = 0;
        if (precision > num_len) {
          zeros = precision - num_len;
        }
      } else if (zero_pad && !left_justify) {
        if (width > (num_len + is_neg)) {
          zeros = width - (num_len + is_neg);
        }
      }

      total_len = num_len + is_neg + zeros;

      if (!left_justify) {
        for (i = total_len; i < width; i++) {
          if (buffer && written < max_len - 1)
            buffer[written] = ' ';
          written++;
        }
      }

      if (is_neg) {
        if (buffer && written < max_len - 1)
          buffer[written] = '-';
        written++;
      }

      for (i = 0; i < zeros; i++) {
        if (buffer && written < max_len - 1)
          buffer[written] = '0';
        written++;
      }

      /* Write reversed string */
      while (num_len > 0) {
        num_len--;
        if (buffer && written < max_len - 1)
          buffer[written] = num_buf[num_len];
        written++;
      }

      if (left_justify) {
        for (i = total_len; i < width; i++) {
          if (buffer && written < max_len - 1)
            buffer[written] = ' ';
          written++;
        }
      }
    } else {
      /* Unsupported specifier: print literally */
      if (buffer && written < max_len - 1)
        buffer[written] = '%';
      written++;
      if (buffer && written < max_len - 1)
        buffer[written] = *format_cursor;
      written++;
    }
    format_cursor++;
  }

  if (buffer && max_len > 0) {
    if (written < max_len) {
      buffer[written] = '\0';
    } else {
      buffer[max_len - 1] = '\0';
    }
  }

  return written;
}

uint64 nexus_strings_string_format(char *string, uint64 max_string_length, const char *format, ...) {
  va_list args;
  uint64  required_length;

  if (!string || !format || max_string_length == 0) {
    return 0;
  }

  va_start(args, format);
  required_length = core_format_engine(NULL, 0, format, args);
  va_end(args);

  if (required_length >= max_string_length) {
    return 0;
  }

  va_start(args, format);
  core_format_engine(string, max_string_length, format, args);
  va_end(args);

  return required_length;
}

uint64 nexus_strings_string_format_with_truncation(char *string, uint64 max_string_length, const char *format, ...) {
  va_list args;
  uint64  required_length;

  if (!string || !format || max_string_length == 0) {
    return 0;
  }

  va_start(args, format);
  required_length = core_format_engine(string, max_string_length, format, args);
  va_end(args);

  if (required_length >= max_string_length) {
    return max_string_length - 1;
  }

  return required_length;
}

uint64 nexus_strings_vstring_format(char *string, uint64 max_string_length, const char *format, va_list args) {
  uint64 required_length;

  if (!string || !format || max_string_length == 0) {
    return 0;
  }

  required_length = core_format_engine(NULL, 0, format, args);

  if (required_length >= max_string_length) {
    return 0;
  }

  core_format_engine(string, max_string_length, format, args);

  return required_length;
}

uint64 nexus_strings_vstring_format_with_truncation(char *string, uint64 max_string_length, const char *format, va_list args) {
  uint64 required_length;

  if (!string || !format || max_string_length == 0) {
    return 0;
  }

  required_length = core_format_engine(string, max_string_length, format, args);

  if (required_length >= max_string_length) {
    return max_string_length - 1;
  }

  return required_length;
}