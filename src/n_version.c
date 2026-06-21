#include "../nexus.h"

NexusSemanticVersion nexus_version_pack(uint16 variant, uint16 major, uint16 minor, uint16 patch) {
  return NEXUS_VERSION_PACK(variant, major, minor, patch);
}

void nexus_version_unpack(NexusSemanticVersion version, uint16 *out_variant, uint16 *out_major, uint16 *out_minor, uint16 *out_patch) {
  NEXUS_ASSERT_DEBUG(out_variant != NULL);
  NEXUS_ASSERT_DEBUG(out_major != NULL);
  NEXUS_ASSERT_DEBUG(out_minor != NULL);
  NEXUS_ASSERT_DEBUG(out_patch != NULL);

  *out_variant = (uint16)((version >> 48) & 0xFFFFu);
  *out_major   = (uint16)((version >> 32) & 0xFFFFu);
  *out_minor   = (uint16)((version >> 16) & 0xFFFFu);
  *out_patch   = (uint16)(version & 0xFFFFu);
}

void nexus_version_format(NexusSemanticVersion version, char *out_buffer, uint_large out_buffer_size) {
  uint16 variant;
  uint16 major;
  uint16 minor;
  uint16 patch;

  nexus_version_unpack(version, &variant, &major, &minor, &patch);
  nexus_strings_string_format_with_truncation(out_buffer, out_buffer_size, "%u.%u.%u.%u", variant, major, minor, patch);
}
