#include "axs15231_display.h"
#include "esphome/core/log.h"

namespace esphome {
namespace axs15231 {

#ifdef USE_ESP_IDF

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

#endif
}  // namespace axs15231
}  // namespace esphome
