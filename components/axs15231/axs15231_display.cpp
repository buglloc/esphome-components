#include "axs15231_display.h"
#include "esphome/core/log.h"

namespace esphome {
namespace axs15231 {

#ifdef USE_ESP_IDF

namespace {
  constexpr static const char *const TAG = "display.axs15231";

  static const uint8_t SW_RESET_CMD = 0x01;
  static const uint8_t SLEEP_OUT = 0x11;
  static const uint8_t INVERT_OFF = 0x20;
  static const uint8_t INVERT_ON = 0x21;
  static const uint8_t ALL_ON = 0x23;
  static const uint8_t WRAM = 0x24;
  static const uint8_t MIPI = 0x26;
  static const uint8_t DISPLAY_ON = 0x29;
  static const uint8_t RASET = 0x2B;
  static const uint8_t CASET = 0x2A;
  static const uint8_t WDATA = 0x2C;
  static const uint8_t TEON = 0x35;
  static const uint8_t MADCTL_CMD = 0x36;
  static const uint8_t PIXFMT = 0x3A;
  static const uint8_t BRIGHTNESS = 0x51;
  static const uint8_t SWIRE1 = 0x5A;
  static const uint8_t SWIRE2 = 0x5B;
  static const uint8_t PAGESEL = 0xFE;

  static const uint8_t MADCTL_MY = 0x80;   ///< Bit 7 Bottom to top
  static const uint8_t MADCTL_MX = 0x40;   ///< Bit 6 Right to left
  static const uint8_t MADCTL_MV = 0x20;   ///< Bit 5 Reverse Mode
  static const uint8_t MADCTL_RGB = 0x00;  ///< Bit 3 Red-Green-Blue pixel order

  // store a 16 bit value in a buffer, big endian.
  static inline void put16_be(uint8_t *buf, uint16_t value) {
    buf[0] = value >> 8;
    buf[1] = value;
  }
}

void AXS15231Display::update() {
  const uint32_t ms = millis();
  this->do_update_();
  int w = this->x_high_ - this->x_low_ + 1;
  int h = this->y_high_ - this->y_low_ + 1;


  esph_log_w(TAG, "cc: %d", millis() - ms);


  this->draw_pixels_at(this->x_low_, this->y_low_, w, h, this->buffer_, display::COLOR_ORDER_RGB, display::COLOR_BITNESS_565,
                        true, this->x_low_, this->y_low_, this->get_width_internal() - w - this->x_low_);
  // invalidate watermarks
  this->x_low_ = this->width_;
  this->y_low_ = this->height_;
  this->x_high_ = 0;
  this->y_high_ = 0;


  esph_log_w(TAG, "bb: %d", millis() - ms);
}

void AXS15231Display::setup() {
  esph_log_config(TAG, "Setting up axs15231");
  this->spi_setup();

  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(true);
  }

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(5);
    this->reset_pin_->digital_write(false);
    delay(5);
    this->reset_pin_->digital_write(true);
  }

  this->set_timeout(120, [this] {
    this->write_command_(SLEEP_OUT);
  });
  this->set_timeout(240, [this] {
    this->write_init_sequence_();
  });
}

bool AXS15231Display::can_proceed() {
  return this->setup_complete_;
}

void AXS15231Display::dump_config() {
  ESP_LOGCONFIG("", "AXS15231 Display");
  ESP_LOGCONFIG(TAG, "  Height: %u", this->height_);
  ESP_LOGCONFIG(TAG, "  Width: %u", this->width_);
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  SPI Data rate: %dMHz", (unsigned) (this->data_rate_ / 1000000));
#ifdef USE_POWER_SUPPLY
  ESP_LOGCONFIG(TAG, "  Power Supply Configured: yes");
#endif
}

// Fill the entire screen with the given color.
// void AXS15231Display::fill(Color color) {
//   uint8_t rgb[] = {color.r, color.g, color.b};
//   this->write_command_(0x2f, rgb, 3);
// }

int AXS15231Display::get_width() {
  return this->width_;
}

int AXS15231Display::get_height() {
  return this->height_;
}

display::DisplayType AXS15231Display::get_display_type() {
  return display::DisplayType::DISPLAY_TYPE_COLOR;
}

int AXS15231Display::get_width_internal() {
  return this->width_;
}

int AXS15231Display::get_height_internal() {
  return this->height_;
}

void AXS15231Display::set_reset_pin(GPIOPin *reset_pin) {
  this->reset_pin_ = reset_pin;
}

void AXS15231Display::set_enable_pin(GPIOPin *enable_pin) {
  this->enable_pin_ = enable_pin;
}

void AXS15231Display::set_width(uint16_t width) {
  this->width_ = width;
}

void AXS15231Display::set_dimensions(uint16_t width, uint16_t height) {
  this->width_ = width;
  this->height_ = height;
}

void AXS15231Display::set_mirror_x(bool mirror_x) {
  this->mirror_x_ = mirror_x;
  this->reset_params_();
}

void AXS15231Display::set_mirror_y(bool mirror_y) {
  this->mirror_y_ = mirror_y;
  this->reset_params_();
}

void AXS15231Display::set_swap_xy(bool swap_xy) {
  this->swap_xy_ = swap_xy;
  this->reset_params_();
}

void AXS15231Display::set_brightness(uint8_t brightness) {
  this->brightness_ = brightness;
  this->reset_params_();
}

void AXS15231Display::set_offsets(int16_t offset_x, int16_t offset_y) {
  this->offset_x_ = offset_x;
  this->offset_y_ = offset_y;
}

void AXS15231Display::write_command_(uint8_t cmd, const uint8_t *bytes, size_t len) {
  this->enable();
  this->write_cmd_addr_data(8, 0x02, 24, cmd << 8, bytes, len);

  // (size_t cmd_bits, uint32_t cmd, size_t addr_bits, uint32_t address,
    //                              const uint8_t *data, size_t length, uint8_t bus_width)
  this->disable();
}

void AXS15231Display::write_command_(uint8_t cmd, uint8_t data) {
  this->write_command_(cmd, &data, 1);
}

void AXS15231Display::write_command_(uint8_t cmd) {
  this->write_command_(cmd, &cmd, 0);
}

void AXS15231Display::reset_params_(bool ready) {
  if (!ready && !this->is_ready()) {
    return;
  }

  // custom x/y transform and
  uint8_t mad = MADCTL_RGB;
  if (this->swap_xy_)
    mad |= MADCTL_MV;
  if (this->mirror_x_)
    mad |= MADCTL_MX;
  if (this->mirror_y_)
    mad |= MADCTL_MY;
  this->write_command_(MADCTL_CMD, &mad, 1);
  this->write_command_(BRIGHTNESS, &this->brightness_, 1);
}

void AXS15231Display::write_init_sequence_() {
  // if (this->model_ == RM690B0) {
  //   this->write_command_(PAGESEL, 0x20);
  //   this->write_command_(MIPI, 0x0A);
  //   this->write_command_(WRAM, 0x80);
  //   this->write_command_(SWIRE1, 0x51);
  //   this->write_command_(SWIRE2, 0x2E);
  //   this->write_command_(PAGESEL, 0x00);
  //   this->write_command_(0xC2, 0x00);
  //   delay(10);
  //   this->write_command_(TEON, 0x00);
  // }


  // this->write_command_(PIXFMT, 0x55);
  this->write_command_(BRIGHTNESS, 0);

  this->write_command_(0x28);
    delay(20);
  //  const uint8_t data = {0x00};
  this->write_command_(0x10);
  this->write_command_(0x11);
    delay(100);

  this->write_command_(PIXFMT, 0x55);
  this->write_command_(DISPLAY_ON);

  this->reset_params_(true);
  this->setup_complete_ = true;
  esph_log_config(TAG, "axs15231 setup complete");
}

void AXS15231Display::set_addr_window_(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint8_t buf[4];
  x1 += this->offset_x_;
  x2 += this->offset_x_;
  y1 += this->offset_y_;
  y2 += this->offset_y_;
  put16_be(buf, x1);
  put16_be(buf + 2, x2);
  this->write_command_(CASET, buf, sizeof buf);
  put16_be(buf, y1);
  put16_be(buf + 2, y2);
  this->write_command_(RASET, buf, sizeof buf);
}

void AXS15231Display::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= this->get_width_internal() || y < 0 || y >= this->get_height_internal()) {
    esph_log_w(TAG,
      "tring to draw invalid pixel: x(0 <= %d < %d) && y(0 <= %d < %d)",
      x, this->get_width_internal(),
      y, this->get_height_internal()
    );
    return;
  }

  if (this->buffer_ == nullptr) {
    this->init_internal_(this->width_ * this->height_ * 2);
  }

  if (this->is_failed()) {
    esph_log_w(TAG, "unable to draw pixel in failed state");
    return;
  }

  uint32_t pos = (y * this->width_) + x;
  uint16_t new_color;
  bool updated = false;
  pos = pos * 2;
  new_color = display::ColorUtil::color_to_565(color, display::ColorOrder::COLOR_ORDER_RGB);
  if (this->buffer_[pos] != (uint8_t) (new_color >> 8)) {
    this->buffer_[pos] = (uint8_t) (new_color >> 8);
    updated = true;
  }
  pos = pos + 1;
  new_color = new_color & 0xFF;

  if (this->buffer_[pos] != new_color) {
    this->buffer_[pos] = new_color;
    updated = true;
  }

  if (updated) {
    // low and high watermark may speed up drawing from buffer
    if (x < this->x_low_)
      this->x_low_ = x;
    if (y < this->y_low_)
      this->y_low_ = y;
    if (x > this->x_high_)
      this->x_high_ = x;
    if (y > this->y_high_)
      this->y_high_ = y;
  }
}






void AXS15231Display::draw_pixels_at(int x_start, int y_start, int w, int h, const uint8_t *ptr, display::ColorOrder order, display::ColorBitness bitness, bool big_endian, int x_offset, int y_offset, int x_pad){
  if (!this->setup_complete_ || this->is_failed()) {
    esph_log_w(TAG, "setup isn't complete");
    return;
  }

  if (w <= 0 || h <= 0) {
    esph_log_w(TAG, "invalid WxH: %dx%d", w, h);
    return;
  }

  if (bitness != display::COLOR_BITNESS_565 || big_endian != (this->bit_order_ == spi::BIT_ORDER_MSB_FIRST)) {
    return display::Display::draw_pixels_at(x_start, y_start, w, h, ptr, order, bitness, big_endian, x_offset, y_offset, x_pad);
  }



    // esph_log_w(TAG, "aa");

  this->set_addr_window_(x_start, y_start, x_start + w - 1, y_start + h - 1);
  this->enable();
  // x_ and y_offset are offsets into the source buffer, unrelated to our own offsets into the display.
  if (x_offset == 0 && x_pad == 0 && y_offset == 0) {
    // we could deal here with a non-zero y_offset, but if x_offset is zero, y_offset probably will be so don't bother
    this->write_cmd_addr_data(8, 0x32, 24, 0x2C00, ptr, w * h * 2, 4);
  } else {
    this->write_cmd_addr_data(8, 0x32, 24, 0x2C00, nullptr, 0, 4);
    auto stride = x_offset + w + x_pad;
    for (int y = 0; y != h; y++) {
      this->write_cmd_addr_data(0, 0, 0, 0, ptr + ((y + y_offset) * stride + x_offset) * 2, w * 2, 4);
    }
  }
  this->disable();
}

#endif
}  // namespace axs15231
}  // namespace esphome
