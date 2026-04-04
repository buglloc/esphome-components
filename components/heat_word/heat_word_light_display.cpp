#include "heat_word_light_display.h"

#include "esphome/core/log.h"

namespace esphome::heat_word {

static const char *const TAG = "heat_word.light_display";

void HeatWordLightDisplay::setup() {
  if (this->backend_state_ == nullptr) {
    ESP_LOGE(TAG, "Backend light not configured; set heat_word backend to an addressable light");
    this->status_set_error(LOG_STR("Backend light not set"));
    return;
  }

  this->backend_ = static_cast<light::AddressableLight *>(this->backend_state_->get_output());
  if (this->backend_ == nullptr) {
    ESP_LOGE(TAG, "Backend '%s' is not addressable", this->backend_state_->get_name().c_str());
    this->status_set_error(LOG_STR("Backend light output is not addressable"));
    return;
  }

  const size_t backend_size = this->backend_->size();
  if (backend_size < STRIP_SIZE) {
    ESP_LOGE(TAG, "Backend strip must have at least %d LEDs (has %d)", STRIP_SIZE, backend_size);
    this->status_set_error(LOG_STR("Backend strip too short"));
    return;
  }
}

void HeatWordLightDisplay::dump_config() {
  ESP_LOGCONFIG(TAG, "Heat Word light display:");

  if (this->backend_state_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Backend: %s", this->backend_state_->get_name().c_str());
  }
}

void HeatWordLightDisplay::clear_effect_data() {
  if (this->backend_ == nullptr) {
    return;
  }

  this->backend_->clear_effect_data();
}

light::LightTraits HeatWordLightDisplay::get_traits() {
  if (this->backend_ == nullptr) {
    return {};
  }

  return this->backend_->get_traits();
}

void HeatWordLightDisplay::update_state(light::LightState *state) {
  const auto values = state->current_values;
  const uint8_t brightness = light::to_uint8_scale(values.get_brightness() * values.get_state());

  this->correction_.set_local_brightness(brightness);
}

void HeatWordLightDisplay::write_state(light::LightState *state) {
  (void) state;

  if (this->backend_ != nullptr) {
    this->backend_->schedule_show();
  }

  this->mark_shown_();
}

int32_t HeatWordLightDisplay::to_physical_index_(int32_t logical_index) const {
  const int32_t x = logical_index % 16;
  const int32_t y = logical_index / 16;

  const bool is_left_panel = x < 8;
  const int32_t panel_x = is_left_panel ? x : (x - 8);

  // Logical: x 0..15 left→right, y 0..7 top→bottom.
  // Both panels: data in bottom-left, row serpentine upward, out top-left.
  // Bottom row (y=7) left→right; each row above alternates direction.
  const int32_t row_from_bottom = 7 - y;
  const int32_t column =
      (row_from_bottom % 2) == 0 ? panel_x : (7 - panel_x);
  const int32_t panel_offset = row_from_bottom * 8 + column;

  // Right matrix first in the chain (0..63), then left (64..127).
  return is_left_panel ? (panel_offset + 64) : panel_offset;
}

light::ESPColorView HeatWordLightDisplay::get_view_internal(int32_t index) const {
  if (this->backend_ == nullptr) {
    static uint8_t dummy_r = 0;
    static uint8_t dummy_g = 0;
    static uint8_t dummy_b = 0;
    static uint8_t dummy_w = 0;
    static uint8_t dummy_effect = 0;

    return light::ESPColorView(&dummy_r, &dummy_g, &dummy_b, &dummy_w, &dummy_effect, &this->correction_);
  }

  const int32_t physical_index = this->to_physical_index_(index);

  auto view = (*this->backend_)[physical_index];
  view.raw_set_color_correction(&this->correction_);

  return view;
}

}  // namespace esphome::heat_word
