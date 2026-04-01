#pragma once

#include "heat_word_light_display.h"

#include "esphome/components/light/addressable_light.h"

namespace esphome::heat_word {

class HeatWordComponent;
using Phrase = uint32_t;

enum class WordKind : uint8_t {
  HOUR = 0,
  MINUTE,
  MISC,
  DONE,
  HEAT,
  ERRORS,
  BACKGROUND,
  MAX_KIND,
};

enum class PrintState : uint8_t {
  IDLE = 0,
  HEATING,
  PRINTING,
  PAUSED,
  ERROR,
  DONE,
};

struct Word {
  const uint8_t *cells;
  uint8_t len;
  WordKind category;
  const char *name;

  void render(HeatWordComponent *comp, light::AddressableLight *strip) const;
};

Phrase build_phrase(PrintState state, uint32_t remaining_seconds, uint32_t total_seconds);

void render_phrase(Phrase phrase, HeatWordComponent *comp, light::AddressableLight *strip);

}  // namespace esphome::heat_word
