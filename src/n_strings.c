#include <stdarg.h>
#include "../nexus.h"
#include <stb/stb_sprintf.h>

static NexusStringFormatResult p_string_format_result_create(uint64 written_length, uint64 required_length, uint8 truncated, uint8 success) {
  NexusStringFormatResult result;

  result.written_length  = written_length;
  result.required_length = required_length;
  result.truncated       = truncated;
  result.success         = success;

  return result;
}

static NexusStringFormatResult p_string_format_result_failure(void) {
  return p_string_format_result_create(0, 0, FALSE, FALSE);
}

static uint8 p_string_format_max_length_valid(uint64 max_string_length, int32 *out_count) {
  if (max_string_length == 0 || max_string_length > (uint64)2147483647) {
    return FALSE;
  }

  *out_count = (int32)max_string_length;
  return TRUE;
}

static NexusStringFormatResult p_string_format_from_stb_truncated(int stb_result, uint64 max_string_length) {
  uint64 required_length;

  if (stb_result < 0) {
    return p_string_format_result_failure();
  }

  required_length = (uint64)stb_result;

  if (required_length >= max_string_length) {
    return p_string_format_result_create(max_string_length - 1, required_length, TRUE, TRUE);
  }

  return p_string_format_result_create(required_length, required_length, FALSE, TRUE);
}

NexusStringFormatResult nexus_strings_string_format(char *string, uint64 max_string_length, const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult result;

  va_start(args, format);
  result = nexus_strings_vstring_format(string, max_string_length, format, args);
  va_end(args);

  return result;
}

NexusStringFormatResult nexus_strings_string_format_with_truncation(char *string, uint64 max_string_length, const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult result;

  va_start(args, format);
  result = nexus_strings_vstring_format_with_truncation(string, max_string_length, format, args);
  va_end(args);

  return result;
}

NexusStringFormatResult nexus_strings_vstring_format(char *string, uint64 max_string_length, const char *format, va_list args) {
  int32  count;
  int    stb_result;
  uint64 required_length;

  if (!string || !format || !p_string_format_max_length_valid(max_string_length, &count)) {
    return p_string_format_result_failure();
  }

  stb_result = stbsp_vsnprintf(NULL, 0, format, args);
  if (stb_result < 0) {
    return p_string_format_result_failure();
  }

  required_length = (uint64)stb_result;
  if (required_length >= max_string_length) {
    return p_string_format_result_create(0, required_length, TRUE, FALSE);
  }

  stb_result = stbsp_vsnprintf(string, count, format, args);
  if (stb_result < 0) {
    return p_string_format_result_failure();
  }

  return p_string_format_result_create(required_length, required_length, FALSE, TRUE);
}

NexusStringFormatResult nexus_strings_vstring_format_with_truncation(char *string, uint64 max_string_length, const char *format, va_list args) {
  int32 count;
  int   stb_result;

  if (!string || !format || !p_string_format_max_length_valid(max_string_length, &count)) {
    return p_string_format_result_failure();
  }

  stb_result = stbsp_vsnprintf(string, count, format, args);
  return p_string_format_from_stb_truncated(stb_result, max_string_length);
}

uint64 nexus_strings_string_length(const char *string) {
  const char *ptr = string;
  while (*ptr) {
    ptr++;
  }
  return (uint64)(ptr - string);
}

boolean nexus_strings_string_starts_with(const char *string, const char *prefix) {
  if (!string || !prefix)
    return FALSE;
  while (*prefix) {
    if (*string != *prefix)
      return FALSE;
    string++;
    prefix++;
  }
  return TRUE;
}

void nexus_strings_string_copy(char *dest, uint64 dest_max_len, const char *src) {
  uint64 i = 0;
  if (!dest || dest_max_len == 0)
    return;
  if (!src) {
    dest[0] = '\0';
    return;
  }
  while (src[i] && i < (dest_max_len - 1)) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

int32 nexus_strings_string_compare(const char *str1, const char *str2) {
  if (!str1 || !str2)
    return 0;
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const unsigned char *)str1 - *(const unsigned char *)str2;
}
