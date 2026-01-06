#pragma once

#include "protocol.h"

#include <esphome/core/component.h>
#include <esphome/core/automation.h>
#include "esphome/components/sensor/sensor.h"
#include <esphome/components/binary_sensor/binary_sensor.h>
#include <esphome/components/uart/uart.h>

#include <functional>
#include <string_view>
#include <utility>
#include <vector>

namespace esphome {
namespace pkt_p530 {

enum class ErrorCode : uint8_t {
  OK = 0,
  TIMEOUT = 1,
  SEND_FAILED = 2,
  BOOT_FAILED = 3,
  NO_FOOD = 4,
  DOOR_BLOCKED = 7,
  NOT_IMPLEMENTED = 8,
};

using ReportCallback = std::function<bool(ErrorCode, std::basic_string_view<uint8_t>)>;

struct ReportWaiter {
  uint8_t type;
  uint8_t seq;
  uint32_t deadline_ms;
  ReportCallback callback;
};

class P530Component : public Component, public uart::UARTDevice {
 public:
  void loop() override;
  void dump_config() override;

  // Returns if ISD91230 is leave and ready
  // TODO: clear me by TTL
  bool is_ready() { return this->last_status_.is_ready(); };

  bool has_food() { return this->is_ready() && this->last_status_.has_food(); };

  void set_door_opened_sensor(binary_sensor::BinarySensor *s) { this->door_opened_sensor_ = s; }

  void set_door_issue_sensor(binary_sensor::BinarySensor *s) { this->door_issue_sensor_ = s; }

  void set_food_low_sensor(binary_sensor::BinarySensor *s) { this->food_low_sensor_ = s; }

  void set_last_feed_portions_sensor(sensor::Sensor *s) { this->last_feed_portions_sensor_ = s; }

  void set_total_portions_sensor(sensor::Sensor *s) { this->total_portions_sensor_ = s; }

  void add_on_error_callback(std::function<void(ErrorCode)> callback) {
    this->error_callback_.add(std::move(callback));
  }

  void add_on_door_blocked_callback(std::function<void()> callback) {
    this->door_blocked_callback_.add(std::move(callback));
  }

  void add_on_dispense_complete_callback(std::function<void(uint8_t)> callback) {
    this->dispense_complete_callback_.add(std::move(callback));
  }

  // Non-blocking send. Returns seq no
  uint8_t send(ReqType req, const uint8_t *payload, uint8_t len);

  // Add a waiter for a report matching type and seq
  void add_report_waiter(uint8_t type, uint8_t seq, uint32_t timeout_ms, ReportCallback callback);

 protected:
  void check_waiter_timeouts_();

  bool read_packet_();
  void handle_packet_(uint8_t type, uint8_t seq, const std::basic_string_view<uint8_t> payload);
  void handle_status_(const std::basic_string_view<uint8_t> payload);
  void handle_door_complete_(const std::basic_string_view<uint8_t> payload);
  void handle_dispense_complete_(const std::basic_string_view<uint8_t> payload);

  // State
  uint8_t rx_buffer_[PACKET_MAX_SIZE];
  uint8_t tx_seq_{0};
  StatusReport last_status_{};

  // Report waiters
  std::vector<ReportWaiter> waiters_;

  // Sensors
  binary_sensor::BinarySensor *door_opened_sensor_{nullptr};
  binary_sensor::BinarySensor *food_low_sensor_{nullptr};
  binary_sensor::BinarySensor *door_issue_sensor_{nullptr};
  sensor::Sensor *last_feed_portions_sensor_{nullptr};
  sensor::Sensor *total_portions_sensor_{nullptr};

  // Callbacks
  CallbackManager<void(ErrorCode)> error_callback_;
  CallbackManager<void()> door_blocked_callback_;
  CallbackManager<void(uint8_t)> dispense_complete_callback_;
};

}  // namespace pkt_p530
}  // namespace esphome
