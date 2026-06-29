#include <stdarg.h>
#include "../nexus.h"

static uint32 n_tabular_string_width(const char *value) {
  if (value == NULL) {
    return 0;
  }

  return nexus_strings_display_width_get(value);
}

static uint32 n_tabular_max_u32(uint32 left, uint32 right) {
  if (left > right) {
    return left;
  }

  return right;
}

static void n_tabular_report_append_char(NexusTabularReport *report, char character) {
  NEXUS_ASSERT_DEBUG(report != NULL);

  if (report->sizing_pass != FALSE) {
    report->offset += 1;
    return;
  }

  if (report->offset + 1U < report->max_len) {
    report->buffer[report->offset] = character;
  }

  report->offset += 1;
}

static void n_tabular_report_append_repeat(NexusTabularReport *report, char character, uint32 count) {
  uint32 character_i;

  for (character_i = 0; character_i < count; character_i++) {
    n_tabular_report_append_char(report, character);
  }
}

static void n_tabular_report_append_cstring_internal(NexusTabularReport *report, const char *text) {
  uint_large text_length;

  NEXUS_ASSERT_DEBUG(report != NULL);
  NEXUS_ASSERT_DEBUG(text != NULL);

  text_length = nexus_strings_string_length(text);
  if (report->sizing_pass != FALSE) {
    report->offset += text_length;
    return;
  }

  if (report->offset + text_length < report->max_len) {
    nexus_memory_bytes_copy(report->buffer + report->offset, text, text_length);
  }

  report->offset += text_length;
}

static void n_tabular_cell_write(NexusTabularReport *report, const char *value, uint32 width, NexusTabularAlign align) {
  uint32     value_display_width;
  uint32     pad_display;
  uint32     pad_i;
  uint_large copy_byte_length;

  if (value == NULL) {
    value = "";
  }

  value_display_width = n_tabular_string_width(value);
  if (value_display_width > width) {
    copy_byte_length    = nexus_strings_display_width_prefix_length_get(value, width);
    value_display_width = width;
  } else {
    copy_byte_length = nexus_strings_string_length(value);
  }

  pad_display = width - value_display_width;

  if (align == NEXUS_TABULAR_ALIGN_RIGHT) {
    for (pad_i = 0; pad_i < pad_display; pad_i++) {
      n_tabular_report_append_char(report, ' ');
    }
  }

  if (copy_byte_length > 0U) {
    if (report->sizing_pass == FALSE && copy_byte_length < report->max_len - report->offset) {
      nexus_memory_bytes_copy(report->buffer + report->offset, value, copy_byte_length);
    }
    report->offset += copy_byte_length;
  }

  if (align == NEXUS_TABULAR_ALIGN_LEFT) {
    for (pad_i = 0; pad_i < pad_display; pad_i++) {
      n_tabular_report_append_char(report, ' ');
    }
  }
}

static uint32 n_tabular_column_effective_width(const NexusTabularColumn *column) {
  uint32 title_width;
  uint32 effective_width;

  title_width     = n_tabular_string_width(column->title);
  effective_width = n_tabular_max_u32(column->width, title_width);
  effective_width = n_tabular_max_u32(effective_width, column->fit_width);
  if (effective_width == 0U) {
    return title_width;
  }

  return effective_width;
}

static uint32 n_tabular_table_effective_label_width(const NexusTabularTable *table) {
  return n_tabular_max_u32(table->label_width, table->label_fit_width);
}

void nexus_tabular_report_begin(NexusTabularReport *report, char *buffer, uint_large max_len, boolean sizing_pass) {
  NEXUS_ASSERT_DEBUG(report != NULL);

  report->buffer      = buffer;
  report->max_len     = max_len;
  report->offset      = 0;
  report->sizing_pass = sizing_pass;
}

void nexus_tabular_report_section(NexusTabularReport *report, const char *title, uint32 banner_width) {
  NEXUS_ASSERT_DEBUG(report != NULL);

  n_tabular_report_append_repeat(report, '=', banner_width);
  n_tabular_report_append_char(report, '\n');
  if (title == NULL) {
    return;
  }

  n_tabular_report_append_char(report, '[');
  n_tabular_report_append_cstring_internal(report, title);
  n_tabular_report_append_char(report, ']');
  n_tabular_report_append_char(report, '\n');
  n_tabular_report_append_repeat(report, '=', banner_width);
  n_tabular_report_append_char(report, '\n');
}

void nexus_tabular_report_line(NexusTabularReport *report, const char *format, ...) {
  va_list                 args;
  NexusStringFormatResult format_res;

  NEXUS_ASSERT_DEBUG(report != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);

  va_start(args, format);
  if (report->sizing_pass != FALSE) {
    format_res = nexus_strings_vstring_format_required_length(format, args);
    va_end(args);
    report->offset += format_res.required_length + 1U;
    return;
  }

  format_res = nexus_strings_vstring_format_with_truncation(report->buffer + report->offset, report->max_len - report->offset, format, args);
  va_end(args);
  report->offset += format_res.written_length;
  n_tabular_report_append_char(report, '\n');
}

void nexus_tabular_report_blank_line(NexusTabularReport *report) {
  n_tabular_report_append_char(report, '\n');
}

uint_large nexus_tabular_report_offset_get(const NexusTabularReport *report) {
  NEXUS_ASSERT_DEBUG(report != NULL);
  return report->offset;
}

uint64 nexus_tabular_report_required_size_get(const NexusTabularReport *report) {
  NEXUS_ASSERT_DEBUG(report != NULL);
  return (uint64)report->offset + 1U;
}

void nexus_tabular_table_begin(NexusTabularTable *table, NexusTabularReport *report, uint32 label_width) {
  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(report != NULL);

  table->report          = report;
  table->column_count    = 0;
  table->label_width     = label_width;
  table->label_fit_width = 0;
  table->header_written  = FALSE;
}

uint32 nexus_tabular_table_column_add(NexusTabularTable *table, const char *title, uint32 width, NexusTabularAlign align) {
  NexusTabularColumn *column;
  uint32              column_index;

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->header_written == FALSE);
  NEXUS_ASSERT_DEBUG(table->column_count < NEXUS_TABULAR_MAX_COLUMNS);

  column_index        = table->column_count;
  column              = &table->columns[column_index];
  column->title       = title;
  column->width       = width;
  column->fit_width   = 0;
  column->align       = align;
  table->column_count = column_index + 1U;
  return column_index;
}

void nexus_tabular_table_column_fit(NexusTabularTable *table, uint32 column_index, const char *value) {
  uint32 value_width;

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(column_index < table->column_count);

  value_width = n_tabular_string_width(value);
  if (value_width > table->columns[column_index].fit_width) {
    table->columns[column_index].fit_width = value_width;
  }
}

void nexus_tabular_table_label_fit(NexusTabularTable *table, const char *value) {
  uint32 value_width;

  NEXUS_ASSERT_DEBUG(table != NULL);

  value_width = n_tabular_string_width(value);
  if (value_width > table->label_fit_width) {
    table->label_fit_width = value_width;
  }
}

static void n_tabular_table_separator_write(NexusTabularTable *table) {
  uint32 column_i;

  n_tabular_report_append_repeat(table->report, '-', table->label_width);

  for (column_i = 0; column_i < table->column_count; column_i++) {
    n_tabular_report_append_char(table->report, '|');
    n_tabular_report_append_repeat(table->report, '-', table->columns[column_i].width);
  }

  n_tabular_report_append_char(table->report, '\n');
}

static void n_tabular_table_lock_layout(NexusTabularTable *table) {
  uint32 column_i;

  table->label_width     = n_tabular_table_effective_label_width(table);
  table->label_fit_width = 0;

  for (column_i = 0; column_i < table->column_count; column_i++) {
    table->columns[column_i].width     = n_tabular_column_effective_width(&table->columns[column_i]);
    table->columns[column_i].fit_width = 0;
  }
}

void nexus_tabular_table_header_write(NexusTabularTable *table, const char *label_title) {
  uint32 column_i;

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->header_written == FALSE);

  if (label_title != NULL) {
    nexus_tabular_table_label_fit(table, label_title);
  }

  n_tabular_table_lock_layout(table);

  n_tabular_cell_write(table->report, label_title, table->label_width, NEXUS_TABULAR_ALIGN_LEFT);

  for (column_i = 0; column_i < table->column_count; column_i++) {
    n_tabular_report_append_char(table->report, '|');
    n_tabular_cell_write(table->report, table->columns[column_i].title, table->columns[column_i].width, NEXUS_TABULAR_ALIGN_LEFT);
  }

  n_tabular_report_append_char(table->report, '\n');
  n_tabular_table_separator_write(table);
  table->header_written = TRUE;
}

void nexus_tabular_table_row_write(NexusTabularTable *table, const char *row_label, const char *const *cell_values, uint32 cell_count) {
  uint32 column_i;

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->header_written != FALSE);
  NEXUS_ASSERT_DEBUG(cell_count == table->column_count);

  n_tabular_cell_write(table->report, row_label, table->label_width, NEXUS_TABULAR_ALIGN_LEFT);

  for (column_i = 0; column_i < table->column_count; column_i++) {
    n_tabular_report_append_char(table->report, '|');
    n_tabular_cell_write(table->report, cell_values[column_i], table->columns[column_i].width, table->columns[column_i].align);
  }

  n_tabular_report_append_char(table->report, '\n');
}

void nexus_tabular_table_end(NexusTabularTable *table) {
  NEXUS_ASSERT_DEBUG(table != NULL);
  (void)table;
}
