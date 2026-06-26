#include <stdlib.h>
#include <stdarg.h>
#include "../nexus.h"
#include <stb_sprintf.h>

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

static NexusStringFormatResult p_string_format_result_from_required_length(int stb_result) {
  uint_large required_length;

  NEXUS_ASSERT_DEBUG(stb_result >= 0);

  required_length = (uint_large)stb_result;
  return p_string_format_result_create(0, required_length, FALSE, TRUE);
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
  int32      count;
  int        stb_result;
  uint_large required_length;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);
  count = p_string_format_max_length_as_count(max_string_length);

  string[0]  = '\0';
  stb_result = stbsp_vsnprintf(string, count, format, args);
  NEXUS_ASSERT_DEBUG(stb_result >= 0);

  required_length = (uint_large)stb_result;
  if (required_length >= max_string_length) {
    string[0] = '\0';
    return p_string_format_result_create(0, required_length, TRUE, FALSE);
  }

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

NexusStringFormatResult nexus_strings_vstring_format_required_length(const char *format, va_list args) {
  int stb_result;

  NEXUS_ASSERT_DEBUG(format != NULL);

  stb_result = stbsp_vsnprintf(NULL, 0, format, args);
  return p_string_format_result_from_required_length(stb_result);
}

NexusStringFormatResult nexus_strings_string_format_required_length(const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult result;

  va_start(args, format);
  result = nexus_strings_vstring_format_required_length(format, args);
  va_end(args);

  return result;
}

#define NEXUS_STRINGS_BYTES_PER_KIB 1024ULL
static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};

static char n_internal_g_preformat_message_buffer[NEXUS_STRINGS_PREFORMAT_MESSAGE_MAX];

const char *nexus_strings_string_preformat(const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult format_result;

  NEXUS_ASSERT_DEBUG(format != NULL);

  va_start(args, format);
  format_result =
      nexus_strings_vstring_format_with_truncation(n_internal_g_preformat_message_buffer, NEXUS_STRINGS_PREFORMAT_MESSAGE_MAX, format, args);
  va_end(args);

  if (!format_result.success && format[0] != '\0') {
    return "string preformat failed";
  }

  return n_internal_g_preformat_message_buffer;
}

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

static NexusStringFormatResult n_strings_quantity_format_scaled(char *string, uint_large max_string_length, real64 value, uint32 unit_index) {
  static const char *quantity_suffixes[] = {"", "K", "M", "G", "T"};
  uint32             whole;

  if (unit_index >= (uint32)NEXUS_SIZEOF(quantity_suffixes) / (uint32)NEXUS_SIZEOF(quantity_suffixes[0])) {
    unit_index = ((uint32)NEXUS_SIZEOF(quantity_suffixes) / (uint32)NEXUS_SIZEOF(quantity_suffixes[0])) - 1U;
  }

  whole = (uint32)value;
  if ((real64)whole == value) {
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%u%s", whole, quantity_suffixes[unit_index]);
  }

  if (value >= (real64)10) {
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%.2f%s", value, quantity_suffixes[unit_index]);
  }

  return nexus_strings_string_format_with_truncation(string, max_string_length, "%.3f%s", value, quantity_suffixes[unit_index]);
}

NexusStringFormatResult nexus_strings_quantity_format(char *string, uint_large max_string_length, uint64 value) {
  real64 scaled_value;
  uint32 unit_index;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(max_string_length > 0);

  if (value < 1000ULL) {
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%llu", (unsigned long long)value);
  }

  scaled_value = (real64)value;
  unit_index   = 0;
  while (scaled_value >= (real64)1000 && unit_index < 4U) {
    scaled_value /= (real64)1000;
    unit_index++;
  }

  return n_strings_quantity_format_scaled(string, max_string_length, scaled_value, unit_index);
}

NexusStringFormatResult nexus_strings_quantity_format_f_real(char *string, uint_large max_string_length, f_real value) {
  real64 scaled_value;
  uint32 unit_index;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(max_string_length > 0);

  if (value < (f_real)0) {
    value = -value;
  }

  if (value < (f_real)1000) {
    int64 rounded_value = nexus_real_round_to_int64(value, NRRM_ROUND_EVEN);

    if (value == (f_real)rounded_value) {
      return nexus_strings_string_format_with_truncation(string, max_string_length, "%lld", (long long)rounded_value);
    }

    return nexus_strings_string_format_with_truncation(string, max_string_length, "%.3f", value);
  }

  scaled_value = (real64)value;
  unit_index   = 0;
  while (scaled_value >= (real64)1000 && unit_index < 4U) {
    scaled_value /= (real64)1000;
    unit_index++;
  }

  return n_strings_quantity_format_scaled(string, max_string_length, scaled_value, unit_index);
}

static uint8 n_strings_character_is_alphanumeric(char character) {
  if (character >= 'A' && character <= 'Z') {
    return TRUE;
  }
  if (character >= 'a' && character <= 'z') {
    return TRUE;
  }
  if (character >= '0' && character <= '9') {
    return TRUE;
  }
  return FALSE;
}

NexusStringFormatResult nexus_strings_string_replace_non_alphanumeric(char *dest, uint_large dest_max_len, const char *src, char replacement) {
  uint_large dest_index;
  uint_large src_index;
  uint8      truncated;

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(dest_max_len > 0);
  NEXUS_ASSERT_DEBUG(src != NULL);

  dest_index = 0;
  src_index  = 0;
  truncated  = FALSE;

  while (src[src_index] != '\0') {
    char output_character;

    if (n_strings_character_is_alphanumeric(src[src_index]) == TRUE) {
      output_character = src[src_index];
    } else {
      output_character = replacement;
    }

    if (dest_index + 1 >= dest_max_len) {
      truncated = TRUE;
      break;
    }

    dest[dest_index] = output_character;
    dest_index++;
    src_index++;
  }

  dest[dest_index] = '\0';
  return p_string_format_result_create(dest_index, src_index, truncated, TRUE);
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
  (void)nexus_strings_string_copy_with_truncation(dest, dest_max_len, src);
}

boolean nexus_strings_string_append(char *dest, uint_large dest_max_len, const char *src) {
  uint_large cursor;
  uint_large remaining_length;
  NexusStringFormatResult copy_result;

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(dest_max_len > 0);
  NEXUS_ASSERT_DEBUG(src != NULL);

  cursor = nexus_strings_string_length(dest);
  if (cursor >= dest_max_len) {
    if (dest_max_len > 0) {
      dest[dest_max_len - 1] = '\0';
    }
    return FALSE;
  }

  remaining_length = dest_max_len - cursor;
  copy_result      = nexus_strings_string_copy_with_truncation(dest + cursor, remaining_length, src);
  return copy_result.success == TRUE && copy_result.truncated == FALSE;
}

NexusStringFormatResult nexus_strings_string_copy_exact(char *dest, uint_large dest_max_len, const char *src) {
  uint_large src_length;

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(dest_max_len > 0);
  NEXUS_ASSERT_DEBUG(src != NULL);

  src_length = nexus_strings_string_length(src);
  if (src_length >= dest_max_len) {
    return p_string_format_result_create(0, src_length, TRUE, FALSE);
  }

  if (src_length > 0) {
    nexus_memory_bytes_copy(dest, src, src_length);
  }
  dest[src_length] = '\0';
  return p_string_format_result_create(src_length, src_length, FALSE, TRUE);
}

NexusStringFormatResult nexus_strings_string_copy_with_truncation(char *dest, uint_large dest_max_len, const char *src) {
  uint_large src_length;
  uint_large copy_length;

  NEXUS_ASSERT_DEBUG(dest != NULL);
  NEXUS_ASSERT_DEBUG(dest_max_len > 0);
  NEXUS_ASSERT_DEBUG(src != NULL);

  src_length  = nexus_strings_string_length(src);
  copy_length = src_length;
  if (copy_length >= dest_max_len) {
    copy_length = dest_max_len - 1;
  }

  if (copy_length > 0) {
    nexus_memory_bytes_copy(dest, src, copy_length);
  }
  dest[copy_length] = '\0';

  if (src_length >= dest_max_len) {
    return p_string_format_result_create(copy_length, src_length, TRUE, TRUE);
  }

  return p_string_format_result_create(src_length, src_length, FALSE, TRUE);
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

int32 nexus_strings_string_compare_unsigned(const unsigned char *str1, const unsigned char *str2) {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*str1 == *str2)) {
    str1++;
    str2++;
  }
  return *str1 - *str2;
}

int32 nexus_strings_string_compare_mixed(const unsigned char *str1, const char *str2) {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*str1 == *(const unsigned char *)str2)) {
    str1++;
    str2++;
  }
  return *str1 - *(const unsigned char *)str2;
}

int32 nexus_strings_string_compare_mixed_alt(const char *str1, const unsigned char *str2) {
  NEXUS_ASSERT_DEBUG(str1 != NULL);
  NEXUS_ASSERT_DEBUG(str2 != NULL);
  while (*str1 && (*(const unsigned char *)str1 == *str2)) {
    str1++;
    str2++;
  }
  return *(const unsigned char *)str1 - *str2;
}

boolean nexus_strings_string_equals(const char *str1, const char *str2) {
  return nexus_strings_string_compare(str1, str2) == 0;
}

boolean nexus_strings_string_equals_unsigned(const unsigned char *str1, const unsigned char *str2) {
  return nexus_strings_string_compare_unsigned(str1, str2) == 0;
}

boolean nexus_strings_string_equals_mixed(const unsigned char *str1, const char *str2) {
  return nexus_strings_string_compare_mixed(str1, str2) == 0;
}

boolean nexus_strings_string_equals_mixed_alt(const char *str1, const unsigned char *str2) {
  return nexus_strings_string_compare_mixed_alt(str1, str2) == 0;
}

static NError nexus_strings_string_parse_unsigned_decimal(const char *string, uint64 max_value, uint64 *out_value) {
  uint_large index;
  uint64     parsed_value;

  if (string == NULL || out_value == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (string[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  parsed_value = 0;
  for (index = 0; string[index] != '\0'; index++) {
    if (string[index] < '0' || string[index] > '9') {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }

    parsed_value = (parsed_value * 10ULL) + (uint64)(string[index] - '0');
    if (parsed_value > max_value) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }
  }

  *out_value = parsed_value;
  return NEXUS_ERROR_NONE;
}

static NError nexus_strings_string_parse_signed_decimal(const char *string, int64 min_value, int64 max_value, int64 *out_value) {
  boolean    negative;
  uint_large index;
  uint64     magnitude;
  uint64     max_magnitude;
  NError     status;

  if (string == NULL || out_value == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (string[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  negative = FALSE;
  index    = 0;
  if (string[0] == '-') {
    negative = TRUE;
    index    = 1;
  } else if (string[0] == '+') {
    index = 1;
  }

  if (string[index] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (negative == TRUE) {
    if (min_value == INT64_MIN_VAL) {
      max_magnitude = (uint64)INT64_MAX_VAL + 1ULL;
    } else {
      max_magnitude = (uint64)(-(min_value));
    }
  } else {
    max_magnitude = (uint64)max_value;
  }

  status = nexus_strings_string_parse_unsigned_decimal(string + index, max_magnitude, &magnitude);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (negative == TRUE) {
    if (magnitude == (uint64)INT64_MAX_VAL + 1ULL) {
      *out_value = INT64_MIN_VAL;
      return NEXUS_ERROR_NONE;
    }

    *out_value = -(int64)magnitude;
    return NEXUS_ERROR_NONE;
  }

  *out_value = (int64)magnitude;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_uint8(const char *string, uint8 *out_value) {
  uint64     parsed_value;
  NError     status;

  status = nexus_strings_string_parse_unsigned_decimal(string, (uint64)UINT8_MAX_VAL, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (uint8)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_uint16(const char *string, uint16 *out_value) {
  uint64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_unsigned_decimal(string, (uint64)UINT16_MAX_VAL, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (uint16)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_uint32(const char *string, uint32 *out_value) {
  uint64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_unsigned_decimal(string, (uint64)UINT32_MAX_VAL, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (uint32)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_uint64(const char *string, uint64 *out_value) {
  return nexus_strings_string_parse_unsigned_decimal(string, UINT64_MAX_VAL, out_value);
}

NError nexus_strings_string_parse_int8(const char *string, int8 *out_value) {
  int64  parsed_value;
  NError status;

  status = nexus_strings_string_parse_signed_decimal(string, (int64)INT8_MIN_VAL, (int64)INT8_MAX_VAL, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (int8)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_int16(const char *string, int16 *out_value) {
  int64  parsed_value;
  NError status;

  status = nexus_strings_string_parse_signed_decimal(string, (int64)INT16_MIN_VAL, (int64)INT16_MAX_VAL, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (int16)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_int32(const char *string, int32 *out_value) {
  int64  parsed_value;
  NError status;

  status = nexus_strings_string_parse_signed_decimal(string, (int64)INT32_MIN_VAL, (int64)INT32_MAX_VAL, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (int32)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_int64(const char *string, int64 *out_value) {
  return nexus_strings_string_parse_signed_decimal(string, INT64_MIN_VAL, INT64_MAX_VAL, out_value);
}

boolean nexus_strings_string_find(const char *haystack, const char *needle, const char **out_position) {
  uint_large haystack_length;
  uint_large needle_length;
  uint_large haystack_index;
  uint_large needle_index;

  if (haystack == NULL || needle == NULL) {
    return FALSE;
  }

  if (out_position != NULL) {
    *out_position = NULL;
  }
  haystack_length = nexus_strings_string_length(haystack);
  needle_length   = nexus_strings_string_length(needle);

  if (needle_length == 0) {
    if (out_position != NULL) {
      *out_position = haystack;
    }
    return TRUE;
  }

  if (needle_length > haystack_length) {
    return FALSE;
  }

  for (haystack_index = 0; haystack_index + needle_length <= haystack_length; haystack_index++) {
    for (needle_index = 0; needle_index < needle_length; needle_index++) {
      if (haystack[haystack_index + needle_index] != needle[needle_index]) {
        break;
      }
    }

    if (needle_index == needle_length) {
      if (out_position != NULL) {
        *out_position = haystack + haystack_index;
      }
      return TRUE;
    }
  }

  return FALSE;
}

NError nexus_strings_string_split_on_first_delimiter(const char *string, char delimiter, char *left_buffer, uint_large left_max_length,
                                                       char *right_buffer, uint_large right_max_length) {
  uint_large index;
  uint_large left_length;

  if (string == NULL || left_buffer == NULL || right_buffer == NULL || left_max_length == 0 || right_max_length == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  left_length = 0;
  for (index = 0; string[index] != '\0'; index++) {
    if (string[index] == delimiter) {
      if (left_length == 0 || string[index + 1] == '\0') {
        return NEXUS_ERROR_INVALID_ARGUMENT;
      }

      left_buffer[left_length] = '\0';
      if (nexus_strings_string_copy_with_truncation(right_buffer, right_max_length, string + index + 1).success == FALSE) {
        return NEXUS_ERROR_INVALID_ARGUMENT;
      }

      return NEXUS_ERROR_NONE;
    }

    if (left_length + 1 >= left_max_length) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }

    left_buffer[left_length] = string[index];
    left_length++;
  }

  return NEXUS_ERROR_INVALID_ARGUMENT;
}

NError nexus_strings_string_read_word(const char **cursor, char *buffer, uint_large buffer_max_length) {
  const char *source;
  uint_large  write_index;

  if (cursor == NULL || *cursor == NULL || buffer == NULL || buffer_max_length == 0) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  source = *cursor;
  while (source[0] == ' ' || source[0] == '\t') {
    source++;
  }

  if (source[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  write_index = 0;
  while (source[write_index] != '\0' && source[write_index] != ' ' && source[write_index] != '\t') {
    if (write_index + 1 >= buffer_max_length) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }
    buffer[write_index] = source[write_index];
    write_index++;
  }

  buffer[write_index] = '\0';
  source              = source + write_index;
  while (source[0] == ' ' || source[0] == '\t') {
    source++;
  }

  *cursor = source;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_uint64(const char *string, uint64 *out_value) {
  uint_large index;
  uint64     parsed_value;
  uint64     digit_value;

  if (string == NULL || out_value == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  if (string[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  index = 0;
  if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X')) {
    index = 2;
  }

  if (string[index] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  parsed_value = 0;
  for (; string[index] != '\0'; index++) {
    if (string[index] >= '0' && string[index] <= '9') {
      digit_value = (uint64)(string[index] - '0');
    } else if (string[index] >= 'a' && string[index] <= 'f') {
      digit_value = (uint64)(unsigned char)string[index] - (uint64)(unsigned char)'a' + 10ULL;
    } else if (string[index] >= 'A' && string[index] <= 'F') {
      digit_value = (uint64)(unsigned char)string[index] - (uint64)(unsigned char)'A' + 10ULL;
    } else {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }

    if (parsed_value > (UINT64_MAX_VAL >> 4)) {
      return NEXUS_ERROR_INVALID_ARGUMENT;
    }

    parsed_value = (parsed_value << 4) | digit_value;
  }

  *out_value = parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_uint8(const char *string, uint8 *out_value) {
  uint64     parsed_value;
  NError     status;

  status = nexus_strings_string_parse_hex_uint64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value > (uint64)UINT8_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (uint8)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_uint16(const char *string, uint16 *out_value) {
  uint64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_hex_uint64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value > (uint64)UINT16_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (uint16)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_uint32(const char *string, uint32 *out_value) {
  uint64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_hex_uint64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value > (uint64)UINT32_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (uint32)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_int64(const char *string, int64 *out_value) {
  uint64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_hex_uint64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value > (uint64)INT64_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (int64)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_int32(const char *string, int32 *out_value) {
  int64  parsed_value;
  NError status;

  status = nexus_strings_string_parse_hex_int64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value < (int64)INT32_MIN_VAL || parsed_value > (int64)INT32_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (int32)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_int16(const char *string, int16 *out_value) {
  int64  parsed_value;
  NError status;

  status = nexus_strings_string_parse_hex_int64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value < (int64)INT16_MIN_VAL || parsed_value > (int64)INT16_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (int16)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_hex_int8(const char *string, int8 *out_value) {
  int64  parsed_value;
  NError status;

  status = nexus_strings_string_parse_hex_int64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  if (parsed_value < (int64)INT8_MIN_VAL || parsed_value > (int64)INT8_MAX_VAL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (int8)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_real64(const char *string, real64 *out_value) {
  char       *end_pointer;
  const char *parse_source;
  double      parsed_value;

  if (string == NULL || out_value == NULL || string[0] == '\0') {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  parse_source = string;
  parsed_value = strtod(parse_source, &end_pointer);
  if (end_pointer == parse_source) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  *out_value = (real64)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_real32(const char *string, real32 *out_value) {
  real64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_real64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (real32)parsed_value;
  return NEXUS_ERROR_NONE;
}

NError nexus_strings_string_parse_f_real(const char *string, f_real *out_value) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  real64 parsed_value;
  NError status;

  status = nexus_strings_string_parse_real64(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (f_real)parsed_value;
  return NEXUS_ERROR_NONE;
#else
  real32 parsed_value;
  NError status;

  status = nexus_strings_string_parse_real32(string, &parsed_value);
  if (status != NEXUS_ERROR_NONE) {
    return status;
  }

  *out_value = (f_real)parsed_value;
  return NEXUS_ERROR_NONE;
#endif
}

NexusStringFormatResult nexus_strings_string_format_hex_uint8(char *string, uint_large max_string_length, uint8 value) {
  return nexus_strings_string_format(string, max_string_length, "0x%02x", (unsigned)value);
}

NexusStringFormatResult nexus_strings_string_format_hex_uint16(char *string, uint_large max_string_length, uint16 value) {
  return nexus_strings_string_format(string, max_string_length, "0x%04x", (unsigned)value);
}

NexusStringFormatResult nexus_strings_string_format_hex_uint32(char *string, uint_large max_string_length, uint32 value) {
  return nexus_strings_string_format(string, max_string_length, "0x%08x", (unsigned)value);
}

NexusStringFormatResult nexus_strings_string_format_hex_uint64(char *string, uint_large max_string_length, uint64 value) {
  return nexus_strings_string_format(string, max_string_length, "0x%016llx", (unsigned long long)value);
}

NexusStringFormatResult nexus_strings_string_format_hex_real32_bits(char *string, uint_large max_string_length, real32 value) {
  return nexus_strings_string_format_hex_uint32(string, max_string_length, nexus_bits_uint32_from_real32(value));
}

NexusStringFormatResult nexus_strings_string_format_hex_real64_bits(char *string, uint_large max_string_length, real64 value) {
  return nexus_strings_string_format_hex_uint64(string, max_string_length, nexus_bits_uint64_from_real64(value));
}

NexusStringFormatResult nexus_strings_string_format_hex_f_real_bits(char *string, uint_large max_string_length, f_real value) {
#if NEXUS_FLOAT_DOUBLE_PRECISION
  return nexus_strings_string_format_hex_real64_bits(string, max_string_length, (real64)value);
#else
  return nexus_strings_string_format_hex_real32_bits(string, max_string_length, (real32)value);
#endif
}