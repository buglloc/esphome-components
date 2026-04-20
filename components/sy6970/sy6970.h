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
  
  void set_state_led_enabled(bool enabled) { this->is_state_led_enabled_ = enabled; }
  void set_ilim_pin_enable(bool enabled) { this->ilim_pin_enable_ = enabled; }
  void set_batfet_enabled(bool enabled) { this->batfet_enabled_ = enabled; }

  void reset_default();

  void enable_state_led();
  void disable_state_led();

  void enable_ilim_pin();
  void disable_ilim_pin();

  void enable_batfet();
  void disable_batfet();
  
  void disable_watchdog();

 protected:
  i2c::ErrorCode get_register_bit(uint8_t reg, uint8_t bit, bool &out);
  i2c::ErrorCode set_register_bit(uint8_t reg, uint8_t bit);
  i2c::ErrorCode clear_register_bit(uint8_t reg, uint8_t bit);

 protected:
  bool is_state_led_enabled_ = true;
  bool ilim_pin_enable_ = true;
  bool batfet_enabled_ = true;
};

}  // namespace sy6970
}  // namespace esphome
