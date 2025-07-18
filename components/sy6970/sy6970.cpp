#include "sy6970.h"
#include "pmu.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sy6970 {

#define _BV(b) (1ULL << (uint64_t)(b))
#define ERROR_CHECK(err) \
  if ((err) != i2c::ERROR_OK) { \
    this->status_set_warning("Failed to communicate"); \
    return; \
  }

#define ERROR_CHECK_RET(err) \
  if ((err) != i2c::ERROR_OK) { \
    this->status_set_warning("Failed to communicate"); \
    return err; \
  }

namespace {
  constexpr static const char *const TAG = "sy6970";
} // anonymous namespace

void SY6970::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SY6970 PMU...");

  this->disable_watchdog();
  this->is_state_led_enabled_ ? this->enable_state_led() : this->disable_state_led();
  
  
  //Fix for phantom touch input when turning off backlight. https://github.com/Xinyuan-LilyGO/T-Display-S3-Long/issues/30
  //Disable the ILIM pin and set the input current limit to maximum.
  uint8_t ILIMPIN_byte = 0x3F;
  this->write_register(0x00, &ILIMPIN_byte, 1); 
  
  //Turn off the BATFET without using the battery.
  uint8_t BATFET_byte = 0x64;
  this->write_register(0x09, &BATFET_byte, 1);

  ESP_LOGCONFIG(TAG, "SY6970 PMU setup complete");
}

void SY6970::dump_config() {
  ESP_LOGCONFIG(TAG, "SY6970 PMU:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  State LED: %s", ONOFF(this->is_state_led_enabled_));
}

void SY6970::reset_default() {
  i2c::ErrorCode err = this->set_register_bit(POWERS_PPM_REG_14H, 7);
  ERROR_CHECK(err);
}

void SY6970::enable_state_led() {
  this->clear_register_bit(POWERS_PPM_REG_07H, 6);
}

void SY6970::disable_state_led() {
  this->set_register_bit(POWERS_PPM_REG_07H, 6);
}

void SY6970::disable_watchdog() {
  uint8_t val = 0;
  i2c::ErrorCode err = this->read_register(POWERS_PPM_REG_07H, &val, 1);
  ERROR_CHECK(err);

  val &= 0xCF;
  err = this->write_register(POWERS_PPM_REG_07H, &val, 1);
  ERROR_CHECK(err);
}

i2c::ErrorCode SY6970::get_register_bit(uint8_t reg, uint8_t bit, bool &out) {
  uint8_t val = 0;
  i2c::ErrorCode err = this->read_register(reg, &val, 1);
  ERROR_CHECK_RET(err);

  out = val & _BV(bit);
  return i2c::ERROR_OK;
}

i2c::ErrorCode SY6970::set_register_bit(uint8_t reg, uint8_t bit) {
  uint8_t val = 0;
  i2c::ErrorCode err = this->read_register(reg, &val, 1);
  ERROR_CHECK_RET(err);

  val |= _BV(bit);
  return this->write_register(reg, &val, 1);
}

i2c::ErrorCode SY6970::clear_register_bit(uint8_t reg, uint8_t bit) {
  uint8_t val = 0;
  i2c::ErrorCode err = this->read_register(reg, &val, 1);
  ERROR_CHECK_RET(err);

  val &= ~_BV(bit);
  return this->write_register(reg, &val, 1);
}

}  // namespace sy6970
}  // namespace esphome
