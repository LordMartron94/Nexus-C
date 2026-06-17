#pragma once

#include "../nexus.h"

/*
Internal helpers shared across Nexus translation units.
*/

extern NError nexus_errors_from_errno(void);

#if NEXUS_PLATFORM_WINDOWS
extern NError nexus_errors_from_windows_error(unsigned long win32_error);
#endif
