#pragma once

#include "heat_word.h"
#include "esphome/core/automation.h"

namespace esphome::heat_word {

template<typename... Ts> class HeatWordSetBrightnessAction : public Action<Ts...> {
 public:
  explicit HeatWordSetBrightnessAction(HeatWordComponent *component) : component_(component) {}

  TEMPLATABLE_VALUE(float, brightness)

  void play(Ts... x) override { this->component_->set_brightness(this->brightness_.value(x...)); }

 protected:
  HeatWordComponent *component_;
};

}  // namespace esphome::heat_word
