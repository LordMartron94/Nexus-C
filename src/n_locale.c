#include <locale.h>
#include "../nexus.h"
#include "./n_internal.h"

#ifndef NEXUS_LOCALE_NUMERIC_BUFFER_LENGTH
#  define NEXUS_LOCALE_NUMERIC_BUFFER_LENGTH 64
#endif

void nexus_locale_numeric_c_push(NexusLocaleNumericScope *scope) {
  const char *previous;

  if (scope == NULL) {
    return;
  }

  scope->active               = FALSE;
  scope->previous_locale[0] = '\0';

  previous = setlocale(LC_NUMERIC, NULL);
  if (previous != NULL) {
    nexus_strings_string_copy(scope->previous_locale, (uint_large)NEXUS_SIZEOF(scope->previous_locale), previous);
  }

  if (setlocale(LC_NUMERIC, "C") != NULL) {
    scope->active = TRUE;
  }
}

void nexus_locale_numeric_c_pop(NexusLocaleNumericScope *scope) {
  if (scope == NULL || scope->active == FALSE) {
    return;
  }

  if (scope->previous_locale[0] != '\0') {
    (void)setlocale(LC_NUMERIC, scope->previous_locale);
  }

  scope->active = FALSE;
}
