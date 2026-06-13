#include "../nexus.h"
#include <stdio.h>
#include <stdlib.h>

static ErrorMessageReportCallback *n_error_report = NULL;
static void                       *n_user_data    = NULL;

void nexus_assertions_error_callback_set(ErrorMessageReportCallback *callback, void *user_data) {
  n_error_report = callback;
  n_user_data    = user_data;
}

#define NEXUS_ASSERTION_FAILURE_REPORT_MAX 500

void nexus_assertions_failure_report(const char *expression, const char *message, const char *file, uint32 line) {
  const char *prefix;
  const char *middle;
  const char *truncation;
  const char *suffix;

  char   out_message[NEXUS_ASSERTION_FAILURE_REPORT_MAX];
  uint64 expression_length;
  uint64 message_length;
  uint64 fixed_length;
  uint64 available_message_length;
  uint64 i;
  uint64 out_i;

  prefix     = "Assertion Failure: ";
  middle     = ", message '";
  truncation = "...";
  suffix     = "'\n";

  if (n_error_report == NULL) {
    (void)fprintf(stderr, "FATAL: cannot report assertion failure without error reporter\n\t- Triggered by: file: %s, line %u\n", file, line);
    NEXUS_ASSERTIONS_DEBUG_TRAP();
  }

  expression_length = nexus_strings_string_length(expression);
  message_length    = nexus_strings_string_length(message);

  /*
    Fixed part excludes the actual message content.

    Layout:
      Assertion Failure: <expression>, message '<message>'\n\0
  */
  fixed_length = nexus_strings_string_length(prefix) + expression_length + nexus_strings_string_length(middle) + nexus_strings_string_length(suffix) +
                 1; /* null terminator */

  if (fixed_length >= NEXUS_ASSERTION_FAILURE_REPORT_MAX) {
    (void)nexus_strings_string_format_with_truncation(out_message, NEXUS_ASSERTION_FAILURE_REPORT_MAX, "Assertion Failure: %s, message ''\n",
                                                      expression);
  } else {
    available_message_length = NEXUS_ASSERTION_FAILURE_REPORT_MAX - fixed_length;

    if (message_length > available_message_length) {
      if (available_message_length > nexus_strings_string_length(truncation)) {
        available_message_length = available_message_length - nexus_strings_string_length(truncation);
      } else {
        available_message_length = 0;
        truncation               = "";
      }
    } else {
      truncation = "";
    }

    out_i = 0;

    for (i = 0; prefix[i] != '\0'; i++) {
      out_message[out_i++] = prefix[i];
    }

    for (i = 0; expression[i] != '\0'; i++) {
      out_message[out_i++] = expression[i];
    }

    for (i = 0; middle[i] != '\0'; i++) {
      out_message[out_i++] = middle[i];
    }

    for (i = 0; i < available_message_length && message[i] != '\0'; i++) {
      out_message[out_i++] = message[i];
    }

    for (i = 0; truncation[i] != '\0'; i++) {
      out_message[out_i++] = truncation[i];
    }

    for (i = 0; suffix[i] != '\0'; i++) {
      out_message[out_i++] = suffix[i];
    }

    out_message[out_i] = '\0';
  }

  n_error_report(n_user_data, out_message, file, line);
}
