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

/*
If truncation cut mid-codepoint, drop the incomplete trailing UTF-8 lead/continuations.
Prevents lone 0xCE (etc.) from rendering as Latin-1 "Î" and skewing display width.
*/
static void n_strings_utf8_clip_incomplete_tail(char *string) {
  uint_large    length;
  uint_large    lead_index;
  unsigned char lead;
  uint32        expected;

  if (string == NULL) {
    return;
  }
  length = nexus_strings_string_length(string);
  while (length > 0U) {
    lead_index = length;
    while (lead_index > 0U && (((unsigned char)string[lead_index - 1U]) & 0xC0U) == 0x80U) {
      lead_index = lead_index - 1U;
    }
    if (lead_index == length) {
      lead = (unsigned char)string[length - 1U];
      if (lead < 0x80U) {
        return;
      }
      /* Lone lead byte at end — incomplete sequence. */
      string[length - 1U] = '\0';
      length              = length - 1U;
      continue;
    }
    lead = (unsigned char)string[lead_index];
    if ((lead & 0xE0U) == 0xC0U) {
      expected = 2U;
    } else if ((lead & 0xF0U) == 0xE0U) {
      expected = 3U;
    } else if ((lead & 0xF8U) == 0xF0U) {
      expected = 4U;
    } else {
      string[lead_index] = '\0';
      length             = lead_index;
      continue;
    }
    if ((length - lead_index) >= (uint_large)expected) {
      return;
    }
    string[lead_index] = '\0';
    length             = lead_index;
  }
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
  int32                   count;
  int                     stb_result;
  NexusStringFormatResult result;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);
  count = p_string_format_max_length_as_count(max_string_length);

  stb_result = stbsp_vsnprintf(string, count, format, args);
  result     = p_string_format_from_stb_truncated(stb_result, max_string_length);
  if (result.truncated == TRUE) {
    n_strings_utf8_clip_incomplete_tail(string);
    result.written_length = nexus_strings_string_length(string);
  }
  return result;
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

static uint32 n_strings_decimal_places_clamp(uint32 decimal_places) {
  if (decimal_places > 9u) {
    return 9u;
  }

  return decimal_places;
}

static NexusStringFormatResult n_strings_format_f_real_nonfinite(char *string, uint_large max_string_length, f_real value, const char *suffix,
                                                                 boolean *is_nonfinite) {
  if (suffix == NULL) {
    suffix = "";
  }

  *is_nonfinite = FALSE;

  if (value != value) {
    *is_nonfinite = TRUE;
    return nexus_strings_string_format_with_truncation(string, max_string_length, "NaN%s", suffix);
  }

  if (value > REAL64_MAX_VAL) {
    *is_nonfinite = TRUE;
    return nexus_strings_string_format_with_truncation(string, max_string_length, "Inf%s", suffix);
  }

  if (value < -REAL64_MAX_VAL) {
    *is_nonfinite = TRUE;
    return nexus_strings_string_format_with_truncation(string, max_string_length, "-Inf%s", suffix);
  }

  return p_string_format_result_create(0, 0, FALSE, TRUE);
}

static NexusStringFormatResult n_strings_format_signed_fixed(char *string, uint_large max_string_length, f_real signed_value, uint32 decimal_places,
                                                             const char *suffix) {
  boolean negative;
  real64  magnitude;
  char    format_buf[24];
  uint32  places;

  if (suffix == NULL) {
    suffix = "";
  }

  places    = n_strings_decimal_places_clamp(decimal_places);
  negative  = (signed_value < (f_real)0) ? TRUE : FALSE;
  magnitude = negative != FALSE ? -(real64)signed_value : (real64)signed_value;

  if (suffix[0] == '%' && suffix[1] == '\0') {
    if (negative != FALSE) {
      (void)nexus_strings_string_format_with_truncation(format_buf, sizeof(format_buf), "-%%.%uf%%%%", places);
    } else {
      (void)nexus_strings_string_format_with_truncation(format_buf, sizeof(format_buf), "%%.%uf%%%%", places);
    }
  } else if (negative != FALSE) {
    (void)nexus_strings_string_format_with_truncation(format_buf, sizeof(format_buf), "-%%.%uf%s", places, suffix);
  } else {
    (void)nexus_strings_string_format_with_truncation(format_buf, sizeof(format_buf), "%%.%uf%s", places, suffix);
  }

  return nexus_strings_string_format_with_truncation(string, max_string_length, format_buf, magnitude);
}

static NexusStringFormatResult n_strings_quantity_format_scaled(char *string, uint_large max_string_length, real64 signed_value, uint32 unit_index,
                                                                uint32 decimal_places) {
  static const char *quantity_suffixes[] = {"", "K", "M", "G", "T"};
  boolean            negative;
  real64             magnitude;
  uint32             whole;
  char               format_buf[24];
  uint32             places;

  if (unit_index >= (uint32)NEXUS_SIZEOF(quantity_suffixes) / (uint32)NEXUS_SIZEOF(quantity_suffixes[0])) {
    unit_index = ((uint32)NEXUS_SIZEOF(quantity_suffixes) / (uint32)NEXUS_SIZEOF(quantity_suffixes[0])) - 1U;
  }

  places    = n_strings_decimal_places_clamp(decimal_places);
  negative  = (signed_value < 0.0) ? TRUE : FALSE;
  magnitude = negative != FALSE ? -signed_value : signed_value;

  whole = (uint32)magnitude;
  if ((real64)whole == magnitude) {
    if (negative != FALSE) {
      return nexus_strings_string_format_with_truncation(string, max_string_length, "-%u%s", whole, quantity_suffixes[unit_index]);
    }
    return nexus_strings_string_format_with_truncation(string, max_string_length, "%u%s", whole, quantity_suffixes[unit_index]);
  }

  if (negative != FALSE) {
    (void)nexus_strings_string_format_with_truncation(format_buf, sizeof(format_buf), "-%%.%uf%s", places, quantity_suffixes[unit_index]);
  } else {
    (void)nexus_strings_string_format_with_truncation(format_buf, sizeof(format_buf), "%%.%uf%s", places, quantity_suffixes[unit_index]);
  }

  return nexus_strings_string_format_with_truncation(string, max_string_length, format_buf, magnitude);
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

  return n_strings_quantity_format_scaled(string, max_string_length, scaled_value, unit_index, NEXUS_STRINGS_QUANTITY_DEFAULT_DECIMAL_PLACES);
}

NexusStringFormatResult nexus_strings_quantity_format_f_real_precision(char *string, uint_large max_string_length, f_real value,
                                                                       uint32 decimal_places) {
  real64                  scaled_value;
  real64                  magnitude;
  uint32                  unit_index;
  boolean                 negative;
  boolean                 is_nonfinite;
  int64                   rounded_value;
  NexusStringFormatResult nonfinite_result;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(max_string_length > 0);

  nonfinite_result = n_strings_format_f_real_nonfinite(string, max_string_length, value, "", &is_nonfinite);
  if (is_nonfinite != FALSE) {
    return nonfinite_result;
  }

  negative  = (value < (f_real)0) ? TRUE : FALSE;
  magnitude = negative != FALSE ? -(real64)value : (real64)value;

  if (magnitude < (real64)1000) {
    rounded_value = nexus_real_round_to_int64((f_real)magnitude, NRRM_ROUND_EVEN);
    if (magnitude == (real64)rounded_value) {
      if (negative != FALSE) {
        return nexus_strings_string_format_with_truncation(string, max_string_length, "-%lld", (long long)rounded_value);
      }
      return nexus_strings_string_format_with_truncation(string, max_string_length, "%lld", (long long)rounded_value);
    }

    return n_strings_format_signed_fixed(string, max_string_length, value, decimal_places, "");
  }

  scaled_value = magnitude;
  unit_index   = 0;
  while (scaled_value >= (real64)1000 && unit_index < 4U) {
    scaled_value /= (real64)1000;
    unit_index++;
  }

  if (negative != FALSE) {
    scaled_value = -scaled_value;
  }

  return n_strings_quantity_format_scaled(string, max_string_length, scaled_value, unit_index, decimal_places);
}

NexusStringFormatResult nexus_strings_quantity_format_f_real(char *string, uint_large max_string_length, f_real value) {
  return nexus_strings_quantity_format_f_real_precision(string, max_string_length, value, NEXUS_STRINGS_QUANTITY_DEFAULT_DECIMAL_PLACES);
}

NexusStringFormatResult nexus_strings_percent_format_f_real_precision(char *string, uint_large max_string_length, f_real percent_value,
                                                                      uint32 decimal_places) {
  boolean                 is_nonfinite;
  NexusStringFormatResult nonfinite_result;

  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(max_string_length > 0);

  nonfinite_result = n_strings_format_f_real_nonfinite(string, max_string_length, percent_value, "%", &is_nonfinite);
  if (is_nonfinite != FALSE) {
    return nonfinite_result;
  }

  return n_strings_format_signed_fixed(string, max_string_length, percent_value, decimal_places, "%");
}

NexusStringFormatResult nexus_strings_percent_format_f_real(char *string, uint_large max_string_length, f_real percent_value) {
  return nexus_strings_percent_format_f_real_precision(string, max_string_length, percent_value, NEXUS_STRINGS_PERCENT_DEFAULT_DECIMAL_PLACES);
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

void nexus_strings_string_copy(char *dest, uint_large dest_max_len, const char *src) {
  (void)nexus_strings_string_copy_with_truncation(dest, dest_max_len, src);
}

boolean nexus_strings_string_append(char *dest, uint_large dest_max_len, const char *src) {
  uint_large              cursor;
  uint_large              remaining_length;
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

void nexus_strings_string_writer_initialize(NexusStringWriter *writer, char *buffer, uint_large buffer_capacity) {
  NEXUS_ASSERT_DEBUG(writer != NULL);
  NEXUS_ASSERT_DEBUG(buffer != NULL);
  NEXUS_ASSERT_DEBUG(buffer_capacity != 0);

  writer->buffer    = buffer;
  writer->capacity  = buffer_capacity;
  writer->length    = 0;
  writer->truncated = FALSE;

  (void)nexus_strings_string_copy_with_truncation(writer->buffer, writer->capacity, "");
}

NexusStringFormatResult nexus_strings_string_writer_string_append(NexusStringWriter *writer, const char *string) {
  NexusStringFormatResult result;
  uint_large              remaining_capacity;

  NEXUS_ASSERT_DEBUG(writer != NULL);
  NEXUS_ASSERT_DEBUG(string != NULL);
  NEXUS_ASSERT_DEBUG(writer->buffer != NULL);
  NEXUS_ASSERT_DEBUG(writer->capacity != 0);
  NEXUS_ASSERT_DEBUG(writer->length < writer->capacity);

  remaining_capacity = writer->capacity - writer->length;
  result             = nexus_strings_string_copy_with_truncation(writer->buffer + writer->length, remaining_capacity, string);

  writer->length += result.written_length;

  if (result.truncated != FALSE) {
    writer->truncated = TRUE;
  }

  return result;
}

NexusStringFormatResult nexus_strings_string_writer_vformat_append(NexusStringWriter *writer, const char *format, va_list args) {
  NexusStringFormatResult result;
  uint_large              remaining_capacity;

  NEXUS_ASSERT_DEBUG(writer != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);
  NEXUS_ASSERT_DEBUG(writer->buffer != NULL);
  NEXUS_ASSERT_DEBUG(writer->capacity != 0);
  NEXUS_ASSERT_DEBUG(writer->length < writer->capacity);

  remaining_capacity = writer->capacity - writer->length;
  result             = nexus_strings_vstring_format_with_truncation(writer->buffer + writer->length, remaining_capacity, format, args);

  writer->length += result.written_length;

  if (result.truncated != FALSE) {
    writer->truncated = TRUE;
  }

  return result;
}

NexusStringFormatResult nexus_strings_string_writer_format_append(NexusStringWriter *writer, const char *format, ...) {
  NexusStringFormatResult result;
  va_list                 args;

  va_start(args, format);
  result = nexus_strings_string_writer_vformat_append(writer, format, args);
  va_end(args);

  return result;
}

uint_large nexus_strings_string_writer_length_get(const NexusStringWriter *writer) {
  NEXUS_ASSERT_DEBUG(writer != NULL);

  return writer->length;
}

boolean nexus_strings_string_writer_truncated_get(const NexusStringWriter *writer) {
  NEXUS_ASSERT_DEBUG(writer != NULL);

  return writer->truncated;
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
  uint_large              src_length;
  uint_large              copy_length;
  NexusStringFormatResult result;

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
    n_strings_utf8_clip_incomplete_tail(dest);
    result = p_string_format_result_create(nexus_strings_string_length(dest), src_length, TRUE, TRUE);
    return result;
  }

  return p_string_format_result_create(src_length, src_length, FALSE, TRUE);
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
  uint64 parsed_value;
  NError status;

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
  uint64 parsed_value;
  NError status;

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

static const char *n_strings_cursor_skip_ansi(const char *cursor) {
  const char *next_cursor;

  if (cursor == NULL || cursor[0] != '\033') {
    return cursor;
  }

  if (cursor[1] != '[') {
    return cursor + 1;
  }

  next_cursor = cursor + 2;
  while (next_cursor[0] != '\0' && next_cursor[0] != 'm') {
    next_cursor++;
  }

  if (next_cursor[0] == 'm') {
    next_cursor++;
  }

  return next_cursor;
}

static boolean n_strings_utf8_decode_at(const unsigned char *text, uint32 *out_codepoint, uint32 *out_byte_count) {
  unsigned char lead;
  unsigned char byte_1;
  unsigned char byte_2;
  unsigned char byte_3;

  if (text == NULL || out_codepoint == NULL || out_byte_count == NULL) {
    return FALSE;
  }

  lead = text[0];
  if (lead == '\0') {
    return FALSE;
  }

  if (lead < 0x80U) {
    *out_codepoint  = (uint32)lead;
    *out_byte_count = 1;
    return TRUE;
  }

  if ((lead & 0xE0U) == 0xC0U) {
    byte_1 = text[1];
    if (byte_1 == '\0' || (byte_1 & 0xC0U) != 0x80U) {
      *out_codepoint  = (uint32)lead;
      *out_byte_count = 1;
      return TRUE;
    }

    *out_codepoint  = (uint32)(((lead & 0x1FU) << 6) | (byte_1 & 0x3FU));
    *out_byte_count = 2;
    return TRUE;
  }

  if ((lead & 0xF0U) == 0xE0U) {
    byte_1 = text[1];
    byte_2 = text[2];
    if (byte_1 == '\0' || byte_2 == '\0' || (byte_1 & 0xC0U) != 0x80U || (byte_2 & 0xC0U) != 0x80U) {
      *out_codepoint  = (uint32)lead;
      *out_byte_count = 1;
      return TRUE;
    }

    *out_codepoint  = (uint32)(((lead & 0x0FU) << 12) | ((byte_1 & 0x3FU) << 6) | (byte_2 & 0x3FU));
    *out_byte_count = 3;
    return TRUE;
  }

  if ((lead & 0xF8U) == 0xF0U) {
    byte_1 = text[1];
    byte_2 = text[2];
    byte_3 = text[3];
    if (byte_1 == '\0' || byte_2 == '\0' || byte_3 == '\0' || (byte_1 & 0xC0U) != 0x80U || (byte_2 & 0xC0U) != 0x80U || (byte_3 & 0xC0U) != 0x80U) {
      *out_codepoint  = (uint32)lead;
      *out_byte_count = 1;
      return TRUE;
    }

    *out_codepoint  = (uint32)(((lead & 0x07U) << 18) | ((byte_1 & 0x3FU) << 12) | ((byte_2 & 0x3FU) << 6) | (byte_3 & 0x3FU));
    *out_byte_count = 4;
    return TRUE;
  }

  *out_codepoint  = (uint32)lead;
  *out_byte_count = 1;
  return TRUE;
}

static uint32 n_strings_codepoint_display_width(uint32 codepoint) {
  if (codepoint < 0x20U || codepoint == 0x7FU) {
    return 0;
  }

  if (codepoint <= 0x7EU) {
    return 1;
  }

  /*
  Combining marks (e.g. U+0304 on δ̄) occupy no extra terminal column. Counting them
  as width 1 shifts every subsequent tabular cell by one (and looks like "Î" when a
  truncated UTF-8 lead is also left behind).
  */
  if ((codepoint >= 0x0300U && codepoint <= 0x036FU) || (codepoint >= 0x1AB0U && codepoint <= 0x1AFFU) ||
      (codepoint >= 0x1DC0U && codepoint <= 0x1DFFU) || (codepoint >= 0x20D0U && codepoint <= 0x20FFU) ||
      (codepoint >= 0xFE20U && codepoint <= 0xFE2FU) || (codepoint >= 0xFE00U && codepoint <= 0xFE0FU) ||
      (codepoint >= 0x200BU && codepoint <= 0x200DU) || codepoint == 0xFEFFU) {
    return 0;
  }

  if ((codepoint >= 0x1100U && codepoint <= 0x115FU) || (codepoint >= 0x2E80U && codepoint <= 0xA4CFU) ||
      (codepoint >= 0xAC00U && codepoint <= 0xD7A3U) || (codepoint >= 0xF900U && codepoint <= 0xFAFFU) ||
      (codepoint >= 0xFE10U && codepoint <= 0xFE1FU) || (codepoint >= 0xFE30U && codepoint <= 0xFE6FU) ||
      (codepoint >= 0xFF00U && codepoint <= 0xFF60U) || (codepoint >= 0xFFE0U && codepoint <= 0xFFE6U)) {
    return 2;
  }

  return 1;
}

static void n_strings_display_measure(const char *text, uint32 max_display_width, boolean limit_display_width, uint32 *out_display_width,
                                      uint_large *out_byte_length) {
  const char          *cursor;
  const unsigned char *bytes;
  uint32               display_width;
  uint_large           byte_length;

  display_width = 0;
  byte_length   = 0;
  cursor        = text;

  if (text == NULL) {
    if (out_display_width != NULL) {
      *out_display_width = 0;
    }
    if (out_byte_length != NULL) {
      *out_byte_length = 0;
    }
    return;
  }

  while (cursor[0] != '\0') {
    const char *after_ansi;
    uint32      codepoint;
    uint32      glyph_byte_count;
    uint32      glyph_display_width;

    after_ansi = n_strings_cursor_skip_ansi(cursor);
    if (after_ansi != cursor) {
      byte_length += (uint_large)(after_ansi - cursor);
      cursor = after_ansi;
      continue;
    }

    bytes = (const unsigned char *)cursor;
    if (n_strings_utf8_decode_at(bytes, &codepoint, &glyph_byte_count) == FALSE) {
      break;
    }

    glyph_display_width = n_strings_codepoint_display_width(codepoint);
    if (limit_display_width == TRUE && display_width + glyph_display_width > max_display_width) {
      break;
    }

    display_width += glyph_display_width;
    byte_length += glyph_byte_count;
    cursor += glyph_byte_count;
  }

  if (out_display_width != NULL) {
    *out_display_width = display_width;
  }
  if (out_byte_length != NULL) {
    *out_byte_length = byte_length;
  }
}

uint32 nexus_strings_display_width_get(const char *text) {
  uint32 display_width;

  n_strings_display_measure(text, 0, FALSE, &display_width, NULL);
  return display_width;
}

uint_large nexus_strings_display_width_prefix_length_get(const char *text, uint32 max_display_width) {
  uint_large byte_length;

  n_strings_display_measure(text, max_display_width, TRUE, NULL, &byte_length);
  return byte_length;
}

int nexus_strings_string_compare_length(const char *string_a, const char *string_b, uint_large max_length) {
  uint_large i;

  if (string_a == NULL || string_b == NULL) {
    return (string_a == string_b) ? 0 : ((string_a == NULL) ? -1 : 1);
  }

  for (i = 0; i < max_length; i++) {
    if (string_a[i] != string_b[i]) {
      return (int)((unsigned char)string_a[i] - (unsigned char)string_b[i]);
    }
    if (string_a[i] == '\0') {
      return 0;
    }
  }

  return 0;
}

char *nexus_strings_string_duplicate(const char *string) {
  char      *copy;
  uint_large length;

  if (string == NULL) {
    return NULL;
  }

  length = nexus_strings_string_length(string);

  if (length == UINT_LARGE_MAX_VAL) {
    return NULL;
  }

  copy = (char *)malloc(length + 1U);

  if (copy == NULL) {
    return NULL;
  }

  nexus_strings_string_copy(copy, length + 1U, string);

  return copy;
}
