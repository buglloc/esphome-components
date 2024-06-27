#pragma once

#include "esphome/core/defines.h"

#ifdef USE_ESP32
#include "esphome/components/esp32_ble/ble.h"
#include "esphome/components/esp32_ble/ble_uuid.h"

namespace esphome {
namespace pinky_winky {

struct PinkyState {
  uint8_t battery_level;
  bool pressed;
  uint32_t ts;
};

class PinkyParser {

public:
  void setup();
  bool is_pinky_uuid(const esp32_ble::ESPBTUUID uuid);
  void set_secret(const std::string &secret);
  bool have_secret();

  optional<PinkyState> parse_state(const std::vector<uint8_t> &mfg_data, const uint32_t last_ts);

protected:
  std::string secret_;
  esp32_ble::ESPBTUUID company_uuid_;

};

}  // namespace pinky_winky
}  // namespace esphome

#endif
