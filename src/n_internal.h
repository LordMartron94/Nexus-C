#pragma once

#include "../nexus.h"

/*
Internal helpers shared across Nexus translation units.
*/

extern void nexus_errors_clear(void);

extern void nexus_errors_record_code(NexusErrorCode code);

extern void nexus_errors_record_errno(void);

#if NEXUS_PLATFORM_WINDOWS
extern void nexus_errors_record_windows_error(unsigned long win32_error);
#endif
