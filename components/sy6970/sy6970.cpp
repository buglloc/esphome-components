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

  this->batfet_enabled_ ? this->enable_batfet() : this->disable_batfet();
  this->ilim_pin_enable_ ? this->enable_ilim_pin() : this->disable_ilim_pin();

  // LED config doesn't work when BATFET is disabled
  if (this->batfet_enabled_) {
    this->is_state_led_enabled_ ? this->enable_state_led() : this->disable_state_led();
  } else if (this->is_state_led_enabled_) {
    ESP_LOGW(TAG, "BATFET is disabled; overriding State LED to OFF");
    this->is_state_led_enabled_ = false;
  }

  this->dump_config();
  ESP_LOGCONFIG(TAG, "SY6970 PMU setup complete");
}

void SY6970::dump_config() {
  ESP_LOGCONFIG(TAG, "SY6970 PMU:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  State LED: %s", ONOFF(this->is_state_led_enabled_));
  ESP_LOGCONFIG(TAG, "  ILIM Pin: %s", ONOFF(this->ilim_pin_enable_));
  ESP_LOGCONFIG(TAG, "  BATFET: %s", ONOFF(this->batfet_enabled_));
}

void SY6970::reset_default() {
  i2c::ErrorCode err = this->set_register_bit(POWERS_PPM_REG_14H, 7);
  ERROR_CHECK(err);
}

void SY6970::enable_state_led() {
  i2c::ErrorCode err = this->clear_register_bit(POWERS_PPM_REG_07H, 6);
  ERROR_CHECK(err);
}

void SY6970::disable_state_led() {
  i2c::ErrorCode err = this->set_register_bit(POWERS_PPM_REG_07H, 6);
  ERROR_CHECK(err);
}

void SY6970::enable_ilim_pin() {
  i2c::ErrorCode err = this->set_register_bit(POWERS_PPM_REG_00H, 6);
  ERROR_CHECK(err);
}

void SY6970::disable_ilim_pin() {
  i2c::ErrorCode err = this->clear_register_bit(POWERS_PPM_REG_00H, 6);
  ERROR_CHECK(err);
}

void SY6970::enable_batfet() {
  i2c::ErrorCode err = this->clear_register_bit(POWERS_PPM_REG_09H, 5);
  ERROR_CHECK(err);
}

void SY6970::disable_batfet() {
  i2c::ErrorCode err = this->set_register_bit(POWERS_PPM_REG_09H, 5);
  ERROR_CHECK(err);
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
