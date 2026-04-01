#pragma once

#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_state.h"

namespace esphome::heat_word {

constexpr int32_t STRIP_SIZE = 128;

class HeatWordLightDisplay : public light::AddressableLight {
 public:
  void set_backend_state(light::LightState *backend_state) { this->backend_state_ = backend_state; }
  light::LightState *get_backend_state() const { return this->backend_state_; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR - 1; }

  int32_t size() const override { return STRIP_SIZE; }
  void clear_effect_data() override;
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;
  void update_state(light::LightState *state) override;

 protected:
  light::ESPColorView get_view_internal(int32_t index) const override;
  int32_t to_physical_index_(int32_t index) const;

  light::LightState *backend_state_{nullptr};
  light::AddressableLight *backend_{nullptr};
};

}  // namespace esphome::heat_word
