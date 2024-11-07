#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace sy6970 {

class SY6970 : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;

  void set_state_led_enabled(bool enabled) {
    this->is_state_led_enabled_ = enabled;
  }

  void reset_default();

  void enable_state_led();
  void disable_state_led();

  void disable_watchdog();

 protected:
  i2c::ErrorCode get_register_bit(uint8_t reg, uint8_t bit, bool &out);
  i2c::ErrorCode set_register_bit(uint8_t reg, uint8_t bit);
  i2c::ErrorCode clear_register_bit(uint8_t reg, uint8_t bit);

 protected:
  bool is_state_led_enabled_ = false;
};

}  // namespace sy6970
}  // namespace esphome
