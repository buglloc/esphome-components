#pragma once

#include "heat_word_phrase.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/esp_hsv_color.h"
#include "esphome/components/light/light_state.h"
#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <array>
#include <cstdint>

#ifndef HEAT_WORD_MAX_PRINT_STATES
#define HEAT_WORD_MAX_PRINT_STATES 8
#endif

namespace esphome::heat_word {

class HeatWordComponent : public PollingComponent, public api::CustomAPIDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR - 2; }

  void set_light_state(light::LightState *light_state) { this->light_state_ = light_state; }

  void set_color_hsv(ColorPalette palette, uint8_t hue, uint8_t saturation, uint8_t value) {
    this->color_palette_[static_cast<size_t>(palette)] = light::ESPHSVColor(hue, saturation, value).to_rgb();
  }

  void set_print_status_entity(const char *entity_id) { this->print_status_entity_ = entity_id; }
  void set_remain_time_entity(const char *entity_id) { this->remain_time_entity_ = entity_id; }
  void set_elapsed_time_entity(const char *entity_id) { this->elapsed_time_entity_ = entity_id; }
  void set_done_timeout_ms(uint32_t ms) { this->done_timeout_ms_ = ms; }
  void set_brightness(float brightness) {
    if (this->light_state_ == nullptr)
      return;
    this->light_state_->make_call().set_brightness(clamp(brightness, 0.0f, 1.0f)).perform();
  }
  void add_print_status(const char *ha_status, PrintState state);

  void on_print_state(PrintState state);
  void on_remaining_seconds(uint32_t sec);
  void redraw();
  Color lookup_color(ColorPalette palette) const;

 protected:
  light::LightState *light_state_{nullptr};

  std::array<Color, static_cast<size_t>(ColorPalette::MAX_COLOR)> color_palette_{};

  PrintState state_{PrintState::IDLE};
  uint32_t remaining_seconds_{0};
  uint32_t elapsed_seconds_{0};
  uint32_t done_timeout_ms_{0};

  const char *print_status_entity_{nullptr};
  const char *remain_time_entity_{nullptr};
  const char *elapsed_time_entity_{nullptr};

  struct PrintStateMapping {
    const char *ha_status;
    PrintState state;
  };
  StaticVector<PrintStateMapping, HEAT_WORD_MAX_PRINT_STATES> state_mappings_;

  void on_ha_print_status_(StringRef state);
  void on_ha_remain_time_(StringRef state);
  void on_ha_elapsed_time_(StringRef state);
  void on_ha_total_time_(StringRef state);
};

}  // namespace esphome::heat_word
