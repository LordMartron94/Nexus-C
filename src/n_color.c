#include "../nexus.h"

extern NexusColorRGBA8 nexus_color_rgba8_create(uint8 red, uint8 green, uint8 blue, uint8 alpha) {
  NexusColorRGBA8 color;
  color.red   = red;
  color.green = green;
  color.blue  = blue;
  color.alpha = alpha;

  return color;
}

extern NexusColorRGBA8 nexus_color_rgba8_create_rgb(uint8 red, uint8 green, uint8 blue) {
  NexusColorRGBA8 color;
  color.red   = red;
  color.green = green;
  color.blue  = blue;
  color.alpha = 255;

  return color;
}
