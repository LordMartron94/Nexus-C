#include <stdarg.h>
#include <stdlib.h>
#include "../nexus.h"

#define N_TABULAR_TABLE_SESSION_TAG 0x54626C52u /* 'TblR' */

static void n_tabular_table_buffered_rows_release(NexusTabularTable *table) {
  NEXUS_ASSERT_DEBUG(table != NULL);

  NEXUS_FREE_IF_NOT_NULL(table->buffered_rows);
  table->buffered_rows         = NULL;
  table->buffered_row_capacity = 0;
  table->buffered_row_count    = 0;
}

static void n_tabular_table_buffered_row_pointers_rebind(NexusTabularBufferedRow *row) {
  uint32 column_i;

  NEXUS_ASSERT_DEBUG(row != NULL);

  for (column_i = 0; column_i < row->cell_count; column_i++) {
    row->cell_pointers[column_i] = row->cell_values[column_i];
  }
}

static void n_tabular_table_buffered_rows_ensure(NexusTabularTable *table, uint32 needed_count) {
  NexusTabularBufferedRow *grown;
  uint32                   new_capacity;
  uint32                   row_i;

  NEXUS_ASSERT_DEBUG(table != NULL);

  if (needed_count <= table->buffered_row_capacity) {
    return;
  }

  new_capacity = table->buffered_row_capacity;
  if (new_capacity == 0U) {
    new_capacity = NEXUS_TABULAR_BUFFERED_ROWS_INITIAL;
  }

  while (new_capacity < needed_count) {
    uint32 doubled;

    doubled = new_capacity * 2U;
    if (doubled <= new_capacity) {
      new_capacity = needed_count;
      break;
    }
    new_capacity = doubled;
  }

  if (table->buffered_rows == NULL) {
    grown = (NexusTabularBufferedRow *)malloc((size_t)new_capacity * NEXUS_SIZEOF(NexusTabularBufferedRow));
  } else {
    grown = (NexusTabularBufferedRow *)realloc(table->buffered_rows, (size_t)new_capacity * NEXUS_SIZEOF(NexusTabularBufferedRow));
  }

  NEXUS_ASSERT_MESSAGE_DEBUG(grown != NULL, "Tabular row buffer allocation failed.");

  table->buffered_rows         = grown;
  table->buffered_row_capacity = new_capacity;

  for (row_i = 0; row_i < table->buffered_row_count; row_i++) {
    n_tabular_table_buffered_row_pointers_rebind(&table->buffered_rows[row_i]);
  }
}

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

  /*
  Non-sizing: offset is written length only. Never advance past bytes actually stored, or
  callers that dump offset bytes (Aegis file reports) will emit uninitialized tail garbage.
  Leave one byte free for a trailing NUL planted by report consumers.
  */
  if (report->offset + 1U < report->max_len) {
    report->buffer[report->offset] = character;
    report->offset += 1;
  }
}

static void n_tabular_report_append_repeat(NexusTabularReport *report, char character, uint32 count) {
  uint32 character_i;

  for (character_i = 0; character_i < count; character_i++) {
    n_tabular_report_append_char(report, character);
  }
}

static void n_tabular_report_append_bytes(NexusTabularReport *report, const char *text, uint_large text_length) {
  uint_large writable;
  uint_large to_copy;

  NEXUS_ASSERT_DEBUG(report != NULL);
  NEXUS_ASSERT_DEBUG(text != NULL || text_length == 0U);

  if (text_length == 0U) {
    return;
  }

  if (report->sizing_pass != FALSE) {
    report->offset += text_length;
    return;
  }

  if (report->offset + 1U >= report->max_len) {
    return;
  }

  writable = report->max_len - report->offset - 1U;
  to_copy  = text_length;
  if (to_copy > writable) {
    to_copy = writable;
  }
  if (to_copy == 0U) {
    return;
  }

  nexus_memory_bytes_copy(report->buffer + report->offset, text, to_copy);
  report->offset += to_copy;
}

static void n_tabular_report_append_cstring_internal(NexusTabularReport *report, const char *text) {
  NEXUS_ASSERT_DEBUG(report != NULL);
  NEXUS_ASSERT_DEBUG(text != NULL);

  n_tabular_report_append_bytes(report, text, nexus_strings_string_length(text));
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
    n_tabular_report_append_bytes(report, value, copy_byte_length);
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

void nexus_tabular_report_vline(NexusTabularReport *report, const char *format, va_list args) {
  NexusStringFormatResult format_res;
  uint_large              remaining;

  NEXUS_ASSERT_DEBUG(report != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);

  if (report->sizing_pass != FALSE) {
    format_res = nexus_strings_vstring_format_required_length(format, args);
    report->offset += format_res.required_length + 1U;
    return;
  }

  /*
  Non-sizing: stop cleanly when full. Do not inflate offset past written bytes (that used to
  poison file dumps). Accurate required size still comes from a sizing_pass begin.
  */
  if (report->offset + 1U >= report->max_len) {
    return;
  }

  remaining  = report->max_len - report->offset;
  format_res = nexus_strings_vstring_format_with_truncation(report->buffer + report->offset, remaining, format, args);
  report->offset += format_res.written_length;
  n_tabular_report_append_char(report, '\n');
}

void nexus_tabular_report_line(NexusTabularReport *report, const char *format, ...) {
  va_list args;

  NEXUS_ASSERT_DEBUG(report != NULL);
  NEXUS_ASSERT_DEBUG(format != NULL);

  va_start(args, format);
  nexus_tabular_report_vline(report, format, args);
  va_end(args);
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

  if (table->session_tag == N_TABULAR_TABLE_SESSION_TAG) {
    n_tabular_table_buffered_rows_release(table);
  } else {
    table->buffered_rows         = NULL;
    table->buffered_row_capacity = 0;
    table->buffered_row_count    = 0;
  }

  table->session_tag          = N_TABULAR_TABLE_SESSION_TAG;
  table->report               = report;
  table->column_count         = 0;
  table->label_width          = label_width;
  table->label_fit_width      = 0;
  table->header_written       = FALSE;
  table->header_deferred      = FALSE;
  table->deferred_label_title = NULL;
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

  n_tabular_report_append_repeat(table->report, '-', n_tabular_table_effective_label_width(table));

  for (column_i = 0; column_i < table->column_count; column_i++) {
    n_tabular_report_append_char(table->report, '|');
    n_tabular_report_append_repeat(table->report, '-', n_tabular_column_effective_width(&table->columns[column_i]));
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

static void n_tabular_table_header_emit(NexusTabularTable *table, const char *label_title) {
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

void nexus_tabular_table_header_defer(NexusTabularTable *table, const char *label_title) {
  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->header_written == FALSE);
  NEXUS_ASSERT_DEBUG(table->header_deferred == FALSE);

  table->header_deferred      = TRUE;
  table->deferred_label_title = label_title;
  if (label_title != NULL) {
    nexus_tabular_table_label_fit(table, label_title);
  }
}

void nexus_tabular_table_row_stage(NexusTabularTable *table, const char *row_label, const char *const *cell_values, uint32 cell_count) {
  NexusTabularBufferedRow *buffered_row;
  uint32                   column_i;

  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->session_tag == N_TABULAR_TABLE_SESSION_TAG);
  NEXUS_ASSERT_DEBUG(table->header_deferred != FALSE);
  NEXUS_ASSERT_DEBUG(table->header_written == FALSE);
  NEXUS_ASSERT_DEBUG(cell_count == table->column_count);

  if (row_label != NULL) {
    nexus_tabular_table_label_fit(table, row_label);
  }

  for (column_i = 0; column_i < cell_count; column_i++) {
    nexus_tabular_table_column_fit(table, column_i, cell_values[column_i]);
  }

  n_tabular_table_buffered_rows_ensure(table, table->buffered_row_count + 1U);

  buffered_row = &table->buffered_rows[table->buffered_row_count];
  if (row_label == NULL) {
    buffered_row->row_label[0] = '\0';
  } else {
    nexus_strings_string_copy(buffered_row->row_label, NEXUS_SIZEOF(buffered_row->row_label), row_label);
  }

  for (column_i = 0; column_i < cell_count; column_i++) {
    if (cell_values[column_i] == NULL) {
      buffered_row->cell_values[column_i][0] = '\0';
    } else {
      nexus_strings_string_copy(buffered_row->cell_values[column_i], NEXUS_SIZEOF(buffered_row->cell_values[column_i]), cell_values[column_i]);
    }
    buffered_row->cell_pointers[column_i] = buffered_row->cell_values[column_i];
  }

  buffered_row->cell_count = cell_count;
  table->buffered_row_count++;
}

void nexus_tabular_table_emit(NexusTabularTable *table) {
  uint32 row_i;

  NEXUS_ASSERT_DEBUG(table != NULL);

  if (table->header_deferred == FALSE) {
    return;
  }

  n_tabular_table_header_emit(table, table->deferred_label_title);

  for (row_i = 0; row_i < table->buffered_row_count; row_i++) {
    const NexusTabularBufferedRow *buffered_row;

    buffered_row = &table->buffered_rows[row_i];
    nexus_tabular_table_row_write(table, buffered_row->row_label, (const char *const *)buffered_row->cell_pointers, buffered_row->cell_count);
  }

  table->header_deferred      = FALSE;
  table->deferred_label_title = NULL;
  n_tabular_table_buffered_rows_release(table);
}

void nexus_tabular_table_header_write(NexusTabularTable *table, const char *label_title) {
  NEXUS_ASSERT_DEBUG(table != NULL);
  NEXUS_ASSERT_DEBUG(table->header_written == FALSE);
  n_tabular_table_header_emit(table, label_title);
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

  if (table->session_tag == N_TABULAR_TABLE_SESSION_TAG) {
    n_tabular_table_buffered_rows_release(table);
    table->session_tag = 0;
  }
}
