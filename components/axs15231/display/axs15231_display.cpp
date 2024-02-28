#include "axs15231_display.h"
#include "esphome/core/log.h"

#ifdef USE_ESP_IDF

namespace {
  constexpr static const char *const TAG = "axs15231.display";

  typedef struct
  {
      uint8_t cmd;
      uint8_t data[36];
      uint8_t len;
  } lcd_cmd_t;

  const static lcd_cmd_t AXS_QSPI_INIT_NEW[] = {
    {0x28, {0x00}, 0x40},
    {0x10, {0x00}, 0x80},
    {0xbb, {0x00,0x00,0x00,0x00,0x00,0x00,0x5a,0xa5}, 0x08},
    {0xa0, {0x00,0x30,0x00,0x02,0x00,0x00,0x05,0x3f,0x30,0x05,0x3f,0x3f,0x00,0x00,0x00,0x00,0x00}, 0x11},
    {0xa2, {0x30,0x04,0x14,0x50,0x80,0x30,0x85,0x80,0xb4,0x28,0xff,0xff,0xff,0x20,0x50,0x10,0x02,0x06,0x20,0xd0,0xc0,0x01,0x12,0xa0,0x91,0xc0,0x20,0x7f,0xff,0x00,0x06}, 0x1F},
    {0xd0, {0x80,0xb4,0x21,0x24,0x08,0x05,0x10,0x01,0xf2,0x02,0xc2,0x02,0x22,0x22,0xaa,0x03,0x10,0x12,0xc0,0x10,0x10,0x40,0x04,0x00,0x30,0x10,0x00,0x03,0x0d,0x12}, 0x1E},
    {0xa3, {0xa0,0x06,0xaa,0x00,0x08,0x02,0x0a,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x00,0x55,0x55}, 0x16},
    {0xc1, {0x33,0x04,0x02,0x02,0x71,0x05,0x24,0x55,0x02,0x00,0x01,0x01,0x53,0xff,0xff,0xff,0x4f,0x52,0x00,0x4f,0x52,0x00,0x45,0x3b,0x0b,0x04,0x0d,0x00,0xff,0x42}, 0x1E},
    {0xc4, {0x00,0x24,0x33,0x80,0x66,0xea,0x64,0x32,0xc8,0x64,0xc8,0x32,0x90,0x90,0x11,0x06,0xdc,0xfa,0x00,0x00,0x80,0xfe,0x10,0x10,0x00,0x0a,0x0a,0x44,0x50}, 0x1D},
    {0xc5, {0x18,0x00,0x00,0x03,0xfe,0xe8,0x3b,0x20,0x30,0x10,0x88,0xde,0x0d,0x08,0x0f,0x0f,0x01,0xe8,0x3b,0x20,0x10,0x10,0x00}, 0x17},
    {0xc6, {0x05,0x0a,0x05,0x0a,0x00,0xe0,0x2e,0x0b,0x12,0x22,0x12,0x22,0x01,0x03,0x00,0x02,0x6a,0x18,0xc8,0x22}, 0x14},
    {0xc7, {0x50,0x36,0x28,0x00,0xa2,0x80,0x8f,0x00,0x80,0xff,0x07,0x11,0x9c,0x6f,0xff,0x24,0x0c,0x0d,0x0e,0x0f,0x01,0x01,0x01,0x01,0x3f,0x07,0x00}, 0x1B},
    {0xc9, {0x33,0x44,0x44,0x01}, 0x04},
    {0xcf, {0x2c,0x1e,0x88,0x58,0x13,0x18,0x56,0x18,0x1e,0x68,0xf7,0x00,0x66,0x0d,0x22,0xc4,0x0c,0x77,0x22,0x44,0xaa,0x55,0x04,0x04,0x12,0xa0,0x08}, 0x1B},
    {0xd5, {0x30,0x30,0x8a,0x00,0x44,0x04,0x4a,0xe5,0x02,0x4a,0xe5,0x02,0x04,0xd9,0x02,0x47,0x03,0x03,0x03,0x03,0x83,0x00,0x00,0x00,0x80,0x52,0x53,0x50,0x50,0x00}, 0x1E},
    {0xd6, {0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe,0x34,0x02,0x01,0x83,0xff,0x00,0x20,0x50,0x00,0x30,0x03,0x03,0x50,0x13,0x00,0x00,0x00,0x04,0x50,0x20,0x01,0x00}, 0x1E},
    {0xd7, {0x03,0x01,0x09,0x0b,0x0d,0x0f,0x1e,0x1f,0x18,0x1d,0x1f,0x19,0x30,0x30,0x04,0x00,0x20,0x20,0x1f}, 0x13},
    {0xd8, {0x02,0x00,0x08,0x0a,0x0c,0x0e,0x1e,0x1f,0x18,0x1d,0x1f,0x19}, 0x0C},
    {0xdf, {0x44,0x33,0x4b,0x69,0x00,0x0a,0x02,0x90}, 0x06},
    {0xe0, {0x1f,0x20,0x10,0x17,0x0d,0x09,0x12,0x2a,0x44,0x25,0x0c,0x15,0x13,0x31,0x36,0x2f,0x02}, 0x11},
    {0xe1, {0x3f,0x20,0x10,0x16,0x0c,0x08,0x12,0x29,0x43,0x25,0x0c,0x15,0x13,0x32,0x36,0x2f,0x27}, 0x11},
    {0xe2, {0x3b,0x07,0x12,0x18,0x0e,0x0d,0x17,0x35,0x44,0x32,0x0c,0x14,0x14,0x36,0x3a,0x2f,0x0d}, 0x11},
    {0xe3, {0x37,0x07,0x12,0x18,0x0e,0x0d,0x17,0x35,0x44,0x32,0x0c,0x14,0x14,0x36,0x32,0x2f,0x0f}, 0x11},
    {0xe4, {0x3b,0x07,0x12,0x18,0x0e,0x0d,0x17,0x39,0x44,0x2e,0x0c,0x14,0x14,0x36,0x3a,0x2f,0x0d}, 0x11},
    {0xe5, {0x37,0x07,0x12,0x18,0x0e,0x0d,0x17,0x39,0x44,0x2e,0x0c,0x14,0x14,0x36,0x3a,0x2f,0x0f}, 0x11},
    {0xbb, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 0x06},
    {0x28, {0x00}, 0x40},
    {0x10, {0x00}, 0x80},
    {0x11, {0x00}, 0x80},
    {0x29, {0x00}, 0x00},
  };


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
}  // namespace

namespace esphome {
namespace axs15231 {

void AXS15231Display::update() {
  if (this->prossing_update_) {
    this->need_update_ = true;
    return;
  }

  this->prossing_update_ = true;
  do {
    this->need_update_ = false;
    this->do_update_();
  } while (this->need_update_);
  this->prossing_update_ = false;
  this->display_();
}

float AXS15231Display::get_setup_priority() const {
  return setup_priority::HARDWARE;
}

void AXS15231Display::setup() {
  ESP_LOGCONFIG(TAG, "setting up axs15231");

  ESP_LOGI(TAG, "setup pins");
  this->setup_pins_();
  ESP_LOGI(TAG, "setup lcd");
  this->init_lcd_();

  ESP_LOGI(TAG, "set madctl");
  this->set_madctl_();

  this->x_low_ = this->width_;
  this->y_low_ = this->height_;
  this->x_high_ = 0;
  this->y_high_ = 0;

  ESP_LOGI(TAG, "init internal buffer");
  this->init_internal_(this->get_buffer_length_());
  if (this->buffer_ == nullptr) {
    this->mark_failed();
  }

  ESP_LOGI(TAG, "set brightness");
  this->write_command_(BRIGHTNESS, &this->brightness_, 1);

  this->setup_complete_ = true;
  ESP_LOGCONFIG(TAG, "axs15231 setup complete");
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

void AXS15231Display::fill(Color color) {
  uint16_t new_color = 0;
  this->x_low_ = 0;
  this->y_low_ = 0;
  this->x_high_ = this->get_width_internal() - 1;
  this->y_high_ = this->get_height_internal() - 1;

  new_color = display::ColorUtil::color_to_565(color);
  if (((uint8_t) (new_color >> 8)) == ((uint8_t) new_color)) {
    // Upper and lower is equal can use quicker memset operation
    memset(this->buffer_, (uint8_t) new_color, this->get_buffer_length_());
    return;
  }

  // Slower set of both buffers
  for (uint32_t i = 0; i < this->get_buffer_length_(); i = i + 2) {
    this->buffer_[i] = (uint8_t) (new_color >> 8);
    this->buffer_[i + 1] = (uint8_t) new_color;
  }
}

display::DisplayType AXS15231Display::get_display_type() {
  return display::DisplayType::DISPLAY_TYPE_COLOR;
}

uint32_t AXS15231Display::get_buffer_length_() {
  return this->get_width_internal() * this->get_height_internal() * 2;
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

void AXS15231Display::set_backlight_pin(GPIOPin *backlight_pin) {
  this->backlight_pin_ = backlight_pin;
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
}

void AXS15231Display::set_mirror_y(bool mirror_y) {
  this->mirror_y_ = mirror_y;
}

void AXS15231Display::set_swap_xy(bool swap_xy) {
  this->swap_xy_ = swap_xy;
}

void AXS15231Display::set_brightness(uint8_t brightness) {
  this->brightness_ = brightness;

  if (this->setup_complete_) {
    this->write_command_(BRIGHTNESS, &this->brightness_, 1);
  }
}

void AXS15231Display::set_offsets(int16_t offset_x, int16_t offset_y) {
  this->offset_x_ = offset_x;
  this->offset_y_ = offset_y;
}

void AXS15231Display::setup_pins_() {
  if (this->backlight_pin_ != nullptr) {
    this->backlight_pin_->setup();
    this->backlight_pin_->digital_write(true);
  }

  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
  }

  this->spi_setup();

  this->reset_();
}

void AXS15231Display::set_madctl_() {
// custom x/y transform and color order
  uint8_t mad = MADCTL_RGB;
  // TODO(buglloc): MADCTL_MV is broken
  // if (this->swap_xy_)
  //   mad |= MADCTL_MV;
  if (this->mirror_x_)
    mad |= MADCTL_MX;
  if (this->mirror_y_)
    mad |= MADCTL_MY;

  this->write_command_(MADCTL_CMD, &mad, 1);
  ESP_LOGD(TAG, "wrote MADCTL 0x%02X", mad);
}

void AXS15231Display::init_lcd_() {
  const lcd_cmd_t *lcd_init = AXS_QSPI_INIT_NEW;
  for (int i = 0; i < sizeof(AXS_QSPI_INIT_NEW) / sizeof(lcd_cmd_t); ++i) {
    this->write_command_(lcd_init[i].cmd, (uint8_t *)lcd_init[i].data, lcd_init[i].len & 0x3f);
    if (lcd_init[i].len & 0x80)
        delay(150);
    if (lcd_init[i].len & 0x40)
        delay(20);
  }
}

void AXS15231Display::reset_() {
  if (this->reset_pin_  == nullptr) {
    return;
  }

  this->reset_pin_->digital_write(true);
  delay(20);
  this->reset_pin_->digital_write(false);
  delay(20);
  this->reset_pin_->digital_write(true);
  delay(20);
}

void AXS15231Display::write_command_(uint8_t cmd, const uint8_t *bytes, size_t len) {
  this->enable();
  this->write_cmd_addr_data(8, 0x02, 24, cmd << 8, bytes, len);
  this->disable();
}

void AXS15231Display::write_command_(uint8_t cmd, uint8_t data) { this->write_command_(cmd, &data, 1); }

void AXS15231Display::write_command_(uint8_t cmd) { this->write_command_(cmd, &cmd, 0); }

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

void AXS15231Display::display_() {
  if ((this->x_high_ < this->x_low_) || (this->y_high_ < this->y_low_)) {
    return;
  }

  // we will only update the changed rows to the display
  size_t const w = this->x_high_ - this->x_low_ + 1;
  size_t const h = this->y_high_ - this->y_low_ + 1;

  this->set_addr_window_(0, this->y_low_, this->width_ - 1, this->y_high_);
  this->enable();
  this->write_cmd_addr_data(8, 0x32, 24, 0x2C00, this->buffer_, w * h * 2, 4);
  // x_ and y_offset are offsets into the source buffer, unrelated to our own offsets into the display.
  // if (x_offset == 0 && x_pad == 0 && y_offset == 0) {
  //   // we could deal here with a non-zero y_offset, but if x_offset is zero, y_offset probably will be so don't bother
  //   this->write_cmd_addr_data(8, 0x32, 24, 0x2C00, ptr, w * h * 2, 4);
  // } else {
  //   this->write_cmd_addr_data(8, 0x32, 24, 0x2C00, nullptr, 0, 4);
  //   auto stride = x_offset + w + x_pad;
  //   for (int y = 0; y != h; y++) {
  //     this->write_cmd_addr_data(0, 0, 0, 0, ptr + ((y + y_offset) * stride + x_offset) * 2, w * 2, 4);
  //   }
  // }

  this->disable();

  // invalidate watermarks
  this->x_low_ = this->width_;
  this->y_low_ = this->height_;
  this->x_high_ = 0;
  this->y_high_ = 0;
}

void AXS15231Display::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x < 0 || x >= this->get_width_internal() || y < 0 || y >= this->get_height_internal()) {
    ESP_LOGW(TAG, "tring to draw invalid pixel: x(0 <= %d < %d) && y(0 <= %d < %d)", x, this->get_width_internal(), y,
               this->get_height_internal());
    return;
  }

  uint32_t pos = (y * width_) + x;
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

#endif
}  // namespace axs15231
}  // namespace esphome
