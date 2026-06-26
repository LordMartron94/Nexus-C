#include "../nexus.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(NEXUS_PLATFORM_WINDOWS)
#  include <io.h>
#elif defined(NEXUS_PLATFORM_POSIX)
#  include <unistd.h>
#else
#  error "Unsupported platform"
#endif

#define NEXUS_ENV_NO_COLOR    "NO_COLOR"
#define NEXUS_ENV_FORCE_COLOR "FORCE_COLOR"
#define NEXUS_ENV_COLORTERM   "COLORTERM"
#define NEXUS_ENV_TERM        "TERM"

static boolean nexus_terminal_environment_variable_truthy(const char *name) {
  char   value[64];
  NError environment_error;

  environment_error = nexus_environment_variable_get(name, value, (uint_large)NEXUS_SIZEOF(value));
  if (environment_error != NEXUS_ERROR_NONE) {
    return FALSE;
  }

  if (value[0] == '\0' || value[0] == '0') {
    return FALSE;
  }

  return TRUE;
}

static boolean nexus_terminal_string_contains_ignore_case(const char *haystack, const char *needle) {
  uint_large haystack_index;
  uint_large needle_index;
  uint_large needle_length;

  if (haystack == NULL || needle == NULL) {
    return FALSE;
  }

  needle_length = nexus_strings_string_length(needle);
  if (needle_length == 0) {
    return TRUE;
  }

  for (haystack_index = 0; haystack[haystack_index] != '\0'; haystack_index++) {
    for (needle_index = 0; needle_index < needle_length; needle_index++) {
      char haystack_character;
      char needle_character;

      haystack_character = haystack[haystack_index + needle_index];
      needle_character   = needle[needle_index];
      if (haystack_character == '\0') {
        break;
      }

      if (haystack_character >= 'A' && haystack_character <= 'Z') {
        haystack_character = (char)(haystack_character - 'A' + 'a');
      }
      if (needle_character >= 'A' && needle_character <= 'Z') {
        needle_character = (char)(needle_character - 'A' + 'a');
      }

      if (haystack_character != needle_character) {
        break;
      }
    }

    if (needle_index == needle_length) {
      return TRUE;
    }
  }

  return FALSE;
}

static boolean nexus_terminal_term_is_smart(const char *term_value) {
  if (term_value == NULL || term_value[0] == '\0') {
    return FALSE;
  }

  if (nexus_terminal_string_contains_ignore_case(term_value, "256color") == TRUE) {
    return TRUE;
  }

  if (nexus_terminal_string_contains_ignore_case(term_value, "-color") == TRUE) {
    return TRUE;
  }

  if (nexus_strings_string_equals(term_value, "xterm") == TRUE) {
    return TRUE;
  }

  if (nexus_strings_string_equals(term_value, "xterm-kitty") == TRUE) {
    return TRUE;
  }
  if (nexus_strings_string_equals(term_value, "alacritty") == TRUE) {
    return TRUE;
  }
  if (nexus_strings_string_equals(term_value, "foot") == TRUE) {
    return TRUE;
  }
  if (nexus_strings_string_equals(term_value, "wezterm") == TRUE) {
    return TRUE;
  }
  if (nexus_strings_string_equals(term_value, "screen-256color") == TRUE) {
    return TRUE;
  }
  if (nexus_strings_string_equals(term_value, "tmux-256color") == TRUE) {
    return TRUE;
  }

  return FALSE;
}

static boolean nexus_terminal_colordetect_is_smart(void) {
  char   colorterm_value[64];
  NError environment_error;

  environment_error = nexus_environment_variable_get(NEXUS_ENV_COLORTERM, colorterm_value, (uint_large)NEXUS_SIZEOF(colorterm_value));
  if (environment_error == NEXUS_ERROR_NONE) {
    if (nexus_terminal_string_contains_ignore_case(colorterm_value, "truecolor") == TRUE) {
      return TRUE;
    }
    if (nexus_terminal_string_contains_ignore_case(colorterm_value, "24bit") == TRUE) {
      return TRUE;
    }
    if (nexus_terminal_string_contains_ignore_case(colorterm_value, "rgb") == TRUE) {
      return TRUE;
    }
  }

  environment_error = nexus_environment_variable_get(NEXUS_ENV_TERM, colorterm_value, (uint_large)NEXUS_SIZEOF(colorterm_value));
  if (environment_error == NEXUS_ERROR_NONE) {
    return nexus_terminal_term_is_smart(colorterm_value);
  }

  return FALSE;
}

NError nexus_terminal_output_capabilities_get(int file_descriptor, NexusTerminalOutputCapabilities *capabilities) {
  if (capabilities == NULL) {
    return NEXUS_ERROR_INVALID_ARGUMENT;
  }

  capabilities->is_terminal      = FALSE;
  capabilities->is_smart_terminal = FALSE;
  capabilities->color_disabled = FALSE;
  capabilities->color_forced     = FALSE;

#if defined(NEXUS_PLATFORM_WINDOWS)
  capabilities->is_terminal = _isatty(file_descriptor) != 0;
#elif defined(NEXUS_PLATFORM_POSIX)
  capabilities->is_terminal = isatty(file_descriptor) != 0;
#endif

  capabilities->color_disabled = nexus_terminal_environment_variable_truthy(NEXUS_ENV_NO_COLOR);
  capabilities->color_forced   = nexus_terminal_environment_variable_truthy(NEXUS_ENV_FORCE_COLOR);

  if (capabilities->is_terminal == TRUE || capabilities->color_forced == TRUE) {
    capabilities->is_smart_terminal = nexus_terminal_colordetect_is_smart();
  }

  return NEXUS_ERROR_NONE;
}

NError nexus_terminal_stdout_capabilities_get(NexusTerminalOutputCapabilities *capabilities) {
#if defined(NEXUS_PLATFORM_WINDOWS)
  return nexus_terminal_output_capabilities_get(_fileno(stdout), capabilities);
#else
  return nexus_terminal_output_capabilities_get(STDOUT_FILENO, capabilities);
#endif
}

NexusColorOutputCapability nexus_terminal_color_output_capability_get(const NexusTerminalOutputCapabilities *capabilities) {
  if (capabilities == NULL) {
    return NCOC_NONE;
  }

  if (capabilities->color_disabled == TRUE && capabilities->color_forced == FALSE) {
    return NCOC_NONE;
  }

  if (capabilities->is_terminal == FALSE && capabilities->color_forced == FALSE) {
    return NCOC_NONE;
  }

  if (capabilities->is_smart_terminal == TRUE || capabilities->color_forced == TRUE) {
    return NCOC_TRUECOLOR;
  }

  return NCOC_ANSI;
}
