#include "axs15231_touchscreen.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"


#define I2C_ERROR_CHECK(err) \
  if ((err) != i2c::ERROR_OK) { \
    this->status_set_warning("I2C communication failed"); \
    return; \
  }

namespace esphome {
namespace axs15231 {

namespace {
  const char *const TAG = "axs15231.touchscreen";

  constexpr uint8_t AXS_TOUCH_POINT_LEN     = 0x06;
  constexpr uint8_t AXS_TOUCH_BUF_HEAD_LEN  = 0x02;

  constexpr uint8_t AXS_TOUCH_EVENT_TOUCH   = 0x08;
  constexpr uint8_t AXS_TOUCH_EVENT_LEAVE   = 0x04;

  constexpr const uint8_t AXS_READ_TOUCHPAD[11] = { 0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 };
} // anonymous namespace

void AXS15231Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Setting up AXS15231 Touchscreen...");

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(false);
    delay(5);
    this->reset_pin_->digital_write(true);
    delay(10);
  }

  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->pin_mode(gpio::FLAG_INPUT);
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_FALLING_EDGE);
  }

  this->x_raw_max_ = this->display_->get_native_width();
  this->y_raw_max_ = this->display_->get_native_height();
  ESP_LOGCONFIG(TAG, "AXS15231 Touchscreen setup complete");
}

void AXS15231Touchscreen::update_touches() {
  i2c::ErrorCode err; 
  uint8_t data[AXS_TOUCH_BUF_HEAD_LEN + AXS_TOUCH_POINT_LEN] = {0};

  err = this->write(AXS_READ_TOUCHPAD, sizeof(AXS_READ_TOUCHPAD), false);
  I2C_ERROR_CHECK(err);

  err = this->read(data, sizeof(data));
  I2C_ERROR_CHECK(err);

  this->status_clear_warning();

  /*
  Read 8-bit touch information:
    0 NULL
    1 Number of fingers touched
    2 [High 4 bits: Event]+[Low 4 bits: High 4 bits of Y-coordinate]
    3 Low 4 bits of Y-coordinate
    4 [High 4 bits: NULL]+[Low 4 bits: High 4 bits of X-coordinate]
    5 Low 4 bits of X-coordinate
    6 NULL
    7 NULL
  */
  uint8_t fingers = data[1];
  if (fingers == 0) {
    return;
  }

  uint8_t touchEvent = data[2] >> 4;
  if (touchEvent != AXS_TOUCH_EVENT_TOUCH) {
    return;
  }

  //Fix for phantom touch input when turning off backlight. https://github.com/Xinyuan-LilyGO/T-Display-S3-Long/issues/30
  if (fingers == 1) {
    uint16_t x = encode_uint16(data[4] & 0xF, data[5]);
    uint16_t y = this->y_raw_max_ - encode_uint16(data[2] & 0xF, data[3]);
    this->add_raw_touch_position_(0, x, y);
  } else {
    return;
  }
}

void AXS15231Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "AXS15231 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN(" Reset Pin: ", this->reset_pin_);
  LOG_PIN(" Interrupt Pin: ", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  X min: %d", this->x_raw_min_);
  ESP_LOGCONFIG(TAG, "  X max: %d", this->x_raw_max_);
  ESP_LOGCONFIG(TAG, "  Y min: %d", this->y_raw_min_);
  ESP_LOGCONFIG(TAG, "  Y max: %d", this->y_raw_max_);

  ESP_LOGCONFIG(TAG, "  Swap X/Y: %s", YESNO(this->swap_x_y_));
  ESP_LOGCONFIG(TAG, "  Invert X: %s", YESNO(this->invert_x_));
  ESP_LOGCONFIG(TAG, "  Invert Y: %s", YESNO(this->invert_y_));
}

}  // namespace axs15231
}  // namespace esphome
