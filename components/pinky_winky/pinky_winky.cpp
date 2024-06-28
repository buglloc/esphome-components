#include "pinky_winky.h"

#include <cinttypes>

#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace pinky_winky {

namespace {
  const char *const TAG = "pinky_winky";
  constexpr uint32_t kBatteryUpdateThrottleMs = 3600 << 10;
}

void PinkyWinky::setup() {
  this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
  if (!this->pref_.load(&this->ts_delta_)) {
    this->ts_delta_ = 0;
    ESP_LOGI(TAG, "restored ts delta");
  }

  this->lock_ = xSemaphoreCreateMutex();
  this->parser_.setup();
  this->last_ts_ = 0;
  this->last_battery_update_ = 0;
}

void PinkyWinky::dump_config() {
  ESP_LOGCONFIG(TAG, "PinkyWinky");
  ESP_LOGCONFIG(TAG, "Timestamp delta: %" PRIu32, this->ts_delta_);
  ESP_LOGCONFIG(TAG, "  Secret: %s", this->parser_.have_secret() ? "yes" : "no");
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Button", this->button_);
#endif
#ifdef USE_SENSOR
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
#endif
}

void PinkyWinky::reset() {
  if (xSemaphoreTake(this->lock_, portMAX_DELAY)) {
    this->ts_delta_ = 0;
    this->last_ts_ = 0;
    this->pref_.save(&this->ts_delta_);
    xSemaphoreGive(this->lock_);

    ESP_LOGI(TAG, "state was reseted");
    return;
  }

  ESP_LOGE(TAG, "unable to get lock");
}

bool PinkyWinky::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device: unknown MAC address (%s): %s", device.get_name().c_str(), device.address_str().c_str());
    return false;
  }

  if (!xSemaphoreTake(this->lock_, 0)) {
    return false;
  }

  bool ok = false;
  for (auto &it : device.get_manufacturer_datas()) {
    if (!this->parser_.is_pinky_uuid(it.uuid)) {
      ESP_LOGW(TAG, "parse_device: invalid uuid: %s", it.uuid.to_string().c_str());
      continue;
    }

    optional<PinkyState> state = this->parser_.parse_state(it.data, this->last_ts_);
    if (!state.has_value()) {
      break;
    }

    if (!this->update_ts(state->ts)) {
      break;
    }

#ifdef USE_SENSOR
    if (this->battery_level_ != nullptr) {
      uint32_t now = millis();
      if (this->last_battery_update_ == 0 || now - this->last_battery_update_ > kBatteryUpdateThrottleMs) {
        this->last_battery_update_ = now;
        this->battery_level_->publish_state(state->battery_level);
      }
    }
#endif

#ifdef USE_BINARY_SENSOR
    if (this->button_ != nullptr) {
      this->button_->publish_state(state->pressed);
    }
#endif

    ok = true;
    break;
  }

  xSemaphoreGive(this->lock_);
  return ok;
}

bool PinkyWinky::update_ts(uint32_t ts) {
  if (ts == 0) {
    ESP_LOGW(TAG, "update_ts: zero timestamp ignored");
    return false;
  }

  if (this->time_ != nullptr) {
    ESPTime time = this->time_->now();
    if (!time.is_valid()) {
      ESP_LOGW(TAG, "update_ts: skip beacon ts check: time is not valid yet");
    } else if (this->ts_delta_ == 0) {
      this->ts_delta_ = time.timestamp - ts;
      ESP_LOGW(TAG, "update_ts: updated ts delta: %" PRIu32 " (ts) -> %" PRId32 " (delta)", ts, this->ts_delta_);
    } else {
      uint32_t expected_ts = time.timestamp - this->ts_delta_;
      if (expected_ts < ts - this->max_ts_drift_ || expected_ts > ts + this->max_ts_drift_) {
        ESP_LOGE(TAG, "update_ts: invalid ts: %" PRIu32 " (actual) > %" PRId32 " (expected)", ts, expected_ts);
        return false;
      }
    }
  }

  this->last_ts_ = ts;
  return true;
}

}  // namespace pinky_winky
}  // namespace esphome

#endif
