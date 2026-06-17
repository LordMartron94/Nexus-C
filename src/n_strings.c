#include <stdarg.h>
#include "../nexus.h"
#include <stb/stb_sprintf.h>

static NexusStringFormatResult p_string_format_result_create(uint_large written_length, uint_large required_length, uint8 truncated, uint8 success) {
  NexusStringFormatResult result;

  result.written_length  = written_length;
  result.required_length = required_length;
  result.truncated       = truncated;
  result.success         = success;

  return result;
}

static int32 p_string_format_max_length_as_count(uint_large max_string_length) {
  NEXUS_ASSERT_DEBUG(max_string_length > 0);
  NEXUS_ASSERT_DEBUG(max_string_length <= (uint_large)2147483647);
  return (int32)max_string_length;
}

static NexusStringFormatResult p_string_format_from_stb_truncated(int stb_result, uint_large max_string_length) {
  uint_large required_length;

  NEXUS_ASSERT_DEBUG(stb_result >= 0);

  required_length = (uint_large)stb_result;

  if (required_length >= max_string_length) {
    return p_string_format_result_create(max_string_length - 1, required_length, TRUE, TRUE);
  }

  return p_string_format_result_create(required_length, required_length, FALSE, TRUE);
}

NexusStringFormatResult nexus_strings_string_format(char *string, uint_large max_string_length, const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult result;

  va_start(args, format);
  result = nexus_strings_vstring_format(string, max_string_length, format, args);
  va_end(args);

  return result;
}

NexusStringFormatResult nexus_strings_string_format_with_truncation(char *string, uint_large max_string_length, const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult result;

  va_start(args, format);
  result = nexus_strings_vstring_format_with_truncation(string, max_string_length, format, args);
  va_end(args);

  return result;
}

NexusStringFormatResult nexus_strings_vstring_format(char *string, uint_large max_string_length, const char *format, va_list args) {
  int32  count;
  int    stb_result;
  uint_large required_length;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);
  count = p_string_format_max_length_as_count(max_string_length);

  stb_result = stbsp_vsnprintf(NULL, 0, format, args);
  NEXUS_ASSERT_DEBUG(stb_result >= 0);

  required_length = (uint_large)stb_result;
  if (required_length >= max_string_length) {
    return p_string_format_result_create(0, required_length, TRUE, FALSE);
  }

  stb_result = stbsp_vsnprintf(string, count, format, args);
  NEXUS_ASSERT_DEBUG(stb_result >= 0);

  return p_string_format_result_create(required_length, required_length, FALSE, TRUE);
}

NexusStringFormatResult nexus_strings_vstring_format_with_truncation(char *string, uint_large max_string_length, const char *format, va_list args) {
  int32 count;
  int   stb_result;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);
  count = p_string_format_max_length_as_count(max_string_length);

  stb_result = stbsp_vsnprintf(string, count, format, args);
  return p_string_format_from_stb_truncated(stb_result, max_string_length);
}

#define NEXUS_STRINGS_BYTES_PER_KIB 1024ULL
static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};

NexusStringFormatResult nexus_strings_bytes_format(char *string, uint_large max_string_length, uint_large byte_count) {
  uint32 unit_index;
  real64 value;
  uint32 whole;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(max_string_length > 0);

  if (byte_count < NEXUS_STRINGS_BYTES_PER_KIB)
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%llu B", (unsigned long long)byte_count);

  value      = (real64)byte_count;
  unit_index = 0;
  while (value >= (real64)NEXUS_STRINGS_BYTES_PER_KIB && unit_index < 4) {
    value /= (real64)NEXUS_STRINGS_BYTES_PER_KIB;
    unit_index++;
  }

  whole = (uint32)value;
  if ((real64)whole == value)
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%u %s", whole, units[unit_index]);
  if (value >= (real64)10)
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%.1f %s", value, units[unit_index]);
  return nexus_strings_string_format_with_truncation(string, max_string_length, "%.2f %s", value, units[unit_index]);
}

uint_large nexus_strings_string_length(const char *string) {
  const char *ptr;

  NEXUS_ASSERT_DEBUG(string != NULL);
  ptr = string;
  while (*ptr) {
    ptr++;
  }
  return (uint_large)(ptr - string);
}

boolean nexus_strings_string_starts_with(const char *string, const char *prefix) {
  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(prefix != NULL);
  while (*prefix) {
    if (*string != *prefix)
      return FALSE;
    string++;
    prefix++;
  }
  return TRUE;
}

void nexus_strings_string_copy(char *dest, uint_large dest_max_len, const char *src) {
  uint_large i = 0;

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(dest_max_len > 0);
  NEXUS_ASSERT_DEBUG(src != NULL);

  while (src[i] && i < (dest_max_len - 1)) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

int32 nexus_strings_string_compare(const char *str1, const char *str2) {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const unsigned char *)str1 - *(const unsigned char *)str2;
}

boolean nexus_strings_string_equals(const char *str1, const char *str2) {
  return nexus_strings_string_compare(str1, str2) == 0;
}
