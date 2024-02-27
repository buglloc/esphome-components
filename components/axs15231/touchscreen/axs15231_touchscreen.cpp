#include "axs15231_touchscreen.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace {
  static const char *const TAG = "axs15231.touchscreen";

  static const uint8_t AXS_READ_TOUCHPAD[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x8};

  static const uint8_t AXS_TOUCH_ONE_POINT_LEN = 0x06;
  static const uint8_t AXS_TOUCH_BUF_HEAD_LEN  = 0x02;

  static const uint8_t AXS_TOUCH_GESTURE_POS = 0x00;
  static const uint8_t AXS_TOUCH_POINT_NUM   = 0x01;
  static const uint8_t AXS_TOUCH_EVENT_POS   = 0x02;
  static const uint8_t AXS_TOUCH_XPOSH       = 0x02;
  static const uint8_t AXS_TOUCH_XPOSL       = 0x03;
  static const uint8_t AXS_TOUCH_YPOSH       = 0x04;
  static const uint8_t AXS_TOUCH_YPOSL       = 0x05;
  static const uint8_t AXS_TOUCH_WEIGHT_POS  = 0x06;
  static const uint8_t AXS_TOUCH_AREA_POS    = 0x07;
  static const uint8_t AXS_TOUCH_DATA_SIZE   = 0x08;

  #define AXS_GET_POINT_NUM(buf) buf[AXS_TOUCH_POINT_NUM]
  #define AXS_GET_GESTURE_TYPE(buf)  buf[AXS_TOUCH_GESTURE_POS]
  #define AXS_GET_POINT_X(buf,point_index) (((uint16_t)(buf[AXS_TOUCH_ONE_POINT_LEN*point_index+AXS_TOUCH_XPOSH] & 0x0F) <<8) + (uint16_t)buf[AXS_TOUCH_ONE_POINT_LEN*point_index+AXS_TOUCH_XPOSL])
  #define AXS_GET_POINT_Y(buf,point_index) (((uint16_t)(buf[AXS_TOUCH_ONE_POINT_LEN*point_index+AXS_TOUCH_YPOSH] & 0x0F) <<8) + (uint16_t)buf[AXS_TOUCH_ONE_POINT_LEN*point_index+AXS_TOUCH_YPOSL])
  #define AXS_GET_POINT_EVENT(buf,point_index) (buf[AXS_TOUCH_ONE_POINT_LEN*point_index+AXS_TOUCH_EVENT_POS] >> 6)
}

namespace esphome {
namespace axs15231 {

#define ERROR_CHECK(err) \
  if ((err) != i2c::ERROR_OK) { \
    ESP_LOGE(TAG, "Failed to communicate!"); \
    this->status_set_warning(); \
    return; \
  }

void AXS15231Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Setting up AXS15231 Touchscreen...");

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(2);
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(2);
  }

  this->x_raw_max_ = this->display_->get_width();
  this->y_raw_max_ = this->display_->get_height();
  ESP_LOGCONFIG(TAG, "AXS15231 Touchscreen setup complete");
}

void AXS15231Touchscreen::update_touches() {
  i2c::ErrorCode err;
  bool touched = false;
  uint8_t buff[AXS_TOUCH_DATA_SIZE];
  u_int16_t x, y;

  err = this->write(AXS_READ_TOUCHPAD, sizeof(AXS_READ_TOUCHPAD), false);
  ERROR_CHECK(err);

  err = this->read(buff, AXS_TOUCH_DATA_SIZE);
  ERROR_CHECK(err);

  x = AXS_GET_POINT_X(buff, 0);
  y = AXS_GET_POINT_Y(buff, 0);

  if ((x == 0 && y == 0) || AXS_GET_GESTURE_TYPE(buff) != 0) {
    return;
  }

  this->add_raw_touch_position_(0, x, y);
}

void AXS15231Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "AXS15231 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN(" Reset Pin: ", this->reset_pin_);
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
