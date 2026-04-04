#pragma once

#include "heat_word_light_display.h"

#include "esphome/components/light/addressable_light.h"

namespace esphome::heat_word {

class HeatWordComponent;

enum class ColorPalette : uint8_t {
  DONE = 0,
  ERROR,
  HEAT,
  HOUR,
  IDLE,
  MINUTE,
  MISC,
  OFF,
  PAUSE,
  MAX_COLOR,
};

enum class PrintState : uint8_t {
  UNAVAILABLE = 0,
  IDLE,
  HEATING,
  PRINTING,
  PAUSED,
  RESUMING,
  ERROR,
  DONE,
};

struct Phrase {
  uint32_t mask;
  ColorPalette bg_color;
};

struct Word {
  const uint8_t *cells;
  uint8_t len;
  ColorPalette color;
  const char *name;

  void render(HeatWordComponent *comp, light::AddressableLight *strip) const;
};

Phrase build_phrase(PrintState state, uint32_t remaining_seconds);

void render_phrase(const Phrase &phrase, HeatWordComponent *comp, light::AddressableLight *strip);

}  // namespace esphome::heat_word
