#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#include "pinky_parser.h"


#ifdef USE_ESP32

namespace esphome {
namespace pinky_winky {

/// Main PinkyWinky component class.
class PinkyWinky : public esphome::Component,
                   public esphome::EntityBase,
                   public esp32_ble_tracker::ESPBTDeviceListener {
public:
  void set_address(uint64_t address) { this->address_ = address; };
  void set_secret(const std::string &secret) { this->parser_.set_secret(secret); };
  void set_max_ts_drift(uint32_t max_ts_drift) { this->max_ts_drift_ = max_ts_drift; };

  void reset();

  void set_time(time::RealTimeClock *time) { time_ = time; }
  time::RealTimeClock *get_time() const { return time_; }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool update_ts(uint32_t ts);
#ifdef USE_BINARY_SENSOR
  void set_button(binary_sensor::BinarySensor *button) { this->button_ = button; }
#endif

#ifdef USE_SENSOR
  void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }
#endif

protected:
  PinkyParser parser_;
  ESPPreferenceObject pref_;
  SemaphoreHandle_t lock_;
  time::RealTimeClock *time_;
  uint32_t max_ts_drift_;
  int32_t ts_delta_;
  uint32_t last_ts_;
  uint64_t address_;

  // PinkyWinky often gets into a state where it spams loads of battery update
  // notifications. Here we will limit to no more than every 60m.
  uint32_t last_battery_update_;

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *button_{nullptr};
#endif
#ifdef USE_SENSOR
  sensor::Sensor *battery_level_{nullptr};
#endif
};

/// Action to reset PinkyWinky.
template<typename... Ts> class PinkyWinkyResetAction : public Action<Ts...>, public Parented<PinkyWinky> {
 public:
  void play(Ts... x) override { this->parent_->reset(); }
};

}  // namespace pinky_winky
}  // namespace esphome

#endif
