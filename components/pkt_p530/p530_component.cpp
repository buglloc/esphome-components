#include "p530_component.h"

#include <esphome/core/log.h>
#include <esphome/core/helpers.h>

namespace esphome {
namespace pkt_p530 {

namespace {
static const char *const TAG = "pkt_p530";
}  // namespace

void P530Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Petkit P530:");
  this->check_uart_settings(115200, 1, uart::UART_CONFIG_PARITY_NONE, 8);
  LOG_BINARY_SENSOR("  ", "Door Opened Sensor", this->door_opened_sensor_);
  LOG_BINARY_SENSOR("  ", "Door Issue Sensor", this->door_issue_sensor_);
  LOG_BINARY_SENSOR("  ", "Low Food Sensor", this->food_low_sensor_);
  LOG_SENSOR("  ", "Dispensed Food Portions Sensor", this->dispensed_portions_sensor_);
}

void P530Component::loop() {
  this->check_waiter_timeouts_();
  while (this->read_packet_()) {
  }
}

void P530Component::check_waiter_timeouts_() {
  if (this->waiters_.empty()) {
    return;
  }

  uint32_t now = millis();
  std::vector<ReportCallback> expired;

  for (auto it = this->waiters_.begin(); it != this->waiters_.end();) {
    if (it->deadline_ms != 0 && static_cast<int32_t>(now - it->deadline_ms) >= 0) {
      ESP_LOGD(TAG, "Waiter timeout: type=0x%02X seq=0x%02X", it->type, it->seq);
      expired.push_back(std::move(it->callback));
      it = this->waiters_.erase(it);
      continue;
    }

    ++it;
  }

  for (auto &cb : expired) {
    cb(ErrorCode::TIMEOUT, {});
  }
}

void P530Component::add_report_waiter(uint8_t type, uint8_t seq, uint32_t timeout_ms, ReportCallback callback) {
  uint32_t deadline = 0;
  if (timeout_ms > 0) {
    deadline = millis() + timeout_ms;
  }

  ESP_LOGD(TAG, "Add waiter: type=0x%02X seq=0x%02X timeout=%ums", type, seq, timeout_ms);
  this->waiters_.push_back({type, seq, deadline, std::move(callback)});
}

bool P530Component::read_packet_() {
  if (this->available() < PACKET_MIN_SIZE) {
    return false;
  }

  uint8_t b;
  while (this->available() >= PACKET_MIN_SIZE) {
    if (!this->read_byte(&b) || b != 0xAA) {
      ESP_LOGW(TAG, "Unpexpected first byte in packet: 0x%02X != 0xAA", b);
      continue;
    }

    if (!this->read_byte(&b) || b != 0xAA) {
      ESP_LOGW(TAG, "Unpexpected first byte in packet: 0x%02X != 0xAA", b);
      continue;
    }

    uint8_t len;
    if (!this->read_byte(&len) || len < PACKET_MIN_SIZE || len > PACKET_MAX_SIZE) {
      ESP_LOGE(TAG, "Invalid packet len: %d", len);
      continue;
    }

    this->rx_buffer_[0] = 0xAA;
    this->rx_buffer_[1] = 0xAA;
    this->rx_buffer_[2] = len;

    if (!this->read_array(&this->rx_buffer_[3], len - 3)) {
      ESP_LOGE(TAG, "Unable to read %d bytes", len - 3);
      return false;
    }

    uint16_t rx_crc = (this->rx_buffer_[len - 2] << 8) | this->rx_buffer_[len - 1];
    uint16_t calc_crc = crc16be(this->rx_buffer_, len - PACKET_CRC_SIZE, 0xFFFF);

    if (rx_crc != calc_crc) {
      ESP_LOGE(TAG, "CRC mismatch");
      continue;
    }

    uint8_t type = this->rx_buffer_[3];
    uint8_t seq = this->rx_buffer_[4];
    uint8_t payload_len = len - PACKET_HEADER_SIZE - PACKET_CRC_SIZE;
    std::basic_string_view<uint8_t> payload(&this->rx_buffer_[PACKET_HEADER_SIZE], payload_len);

    ESP_LOGD(TAG, "RX: type=0x%02X seq=0x%02X len=%u", type, seq, payload_len);
    this->handle_packet_(type, seq, payload);
    return true;
  }

  return false;
}

void P530Component::handle_packet_(uint8_t type, uint8_t seq, const std::basic_string_view<uint8_t> payload) {
  // Check interesting reports
  switch (static_cast<ReportType>(type)) {
    case ReportType::STATUS:
      this->handle_status_(payload);
      break;

    case ReportType::DOOR_OPEN_DONE:
      [[fallthrough]];
    case ReportType::DOOR_CLOSE_DONE:
      this->handle_door_complete_(payload);
      break;

    case ReportType::DISPENSE_DONE:
      this->handle_dispense_complete_(payload);
      break;

    default:
      // pass
      break;
  }

  if (this->waiters_.empty()) {
    return;
  }

  std::vector<ReportWaiter> matched;
  for (auto it = this->waiters_.begin(); it != this->waiters_.end();) {
    if (it->type != type) {
      ++it;
      continue;
    }

    if (it->seq != 0 && it->seq != seq) {
      ++it;
      continue;
    }

    matched.push_back(std::move(*it));
    it = this->waiters_.erase(it);
  }

  for (auto &waiter : matched) {
    if (!waiter.callback(ErrorCode::OK, payload)) {
      // Callback rejected the payload, re-register the waiter
      ESP_LOGV(TAG, "Waiter rejected packet, re-registering: type=0x%02X seq=0x%02X", waiter.type, waiter.seq);
      this->waiters_.push_back(std::move(waiter));
      continue;
    }

    ESP_LOGD(TAG, "Waiter matched: type=0x%02X seq=0x%02X", waiter.type, waiter.seq);
  }
}

void P530Component::handle_status_(const std::basic_string_view<uint8_t> payload) {
  if (payload.size() < sizeof(StatusReport)) {
    ESP_LOGE(TAG, "unexpected status report payload size: %zu", payload.size());
    return;
  }

  memcpy(&this->last_status_, payload.data(), sizeof(StatusReport));

  ESP_LOGD(TAG, "Got status report: food=%s, door_opened=%s", YESNO(this->last_status_.has_food()),
           YESNO(this->last_status_.is_door_open()));

  if (this->food_low_sensor_) {
    this->food_low_sensor_->publish_state(!this->last_status_.has_food());
  }

  if (this->door_opened_sensor_) {
    this->door_opened_sensor_->publish_state(this->last_status_.is_door_open());
  }
}

void P530Component::handle_door_complete_(const std::basic_string_view<uint8_t> payload) {
  if (payload.size() < 1) {
    // ????
    ESP_LOGE(TAG, "unexpected door complete payload size: %zu", payload.size());
    return;
  }

  bool ok = payload[0] == 0x02;
  if (this->door_issue_sensor_) {
    this->door_issue_sensor_->publish_state(!ok);
  }

  if (payload[0] == 0x02) {
    ESP_LOGI(TAG, "Got door report: opened/closed");
    return;
  }

  ESP_LOGW(TAG, "Got door report: door blocked, status=0x%02X", payload[0]);
}

void P530Component::handle_dispense_complete_(const std::basic_string_view<uint8_t> payload) {
  if (payload.size() < 3) {
    // ????
    ESP_LOGE(TAG, "unexpected dispense complete payload size: %zu", payload.size());
    return;
  }

  if (payload[2] == 0x00) {
    ESP_LOGI(TAG, "Got dispense complete report: in progress");
    return;
  }

  uint8_t portions = static_cast<uint8_t>(payload[0]);
  ESP_LOGI(TAG, "Got dispense complete report: portions=%d", portions);
  if (this->dispensed_portions_sensor_ != nullptr) {
    this->dispensed_portions_sensor_->publish_state(portions);
  }
}

uint8_t P530Component::send(ReqType req, const uint8_t *payload, uint8_t len) {
  if (this->tx_seq_ < MAX_SEQ) {
    this->tx_seq_++;
  } else {
    this->tx_seq_ = 1;
  }

  uint8_t pkt[PACKET_MAX_SIZE];
  uint8_t pkt_len = PACKET_HEADER_SIZE + len + PACKET_CRC_SIZE;

  pkt[0] = 0xAA;
  pkt[1] = 0xAA;
  pkt[2] = pkt_len;
  pkt[3] = static_cast<uint8_t>(req);
  pkt[4] = this->tx_seq_;

  if (len > 0 && payload) {
    memcpy(&pkt[PACKET_HEADER_SIZE], payload, len);
  }

  uint16_t crc = crc16be(pkt, pkt_len - PACKET_CRC_SIZE, 0xFFFF);
  pkt[pkt_len - 2] = crc >> 8;
  pkt[pkt_len - 1] = crc & 0xFF;

  ESP_LOGD(TAG, "TX: type=0x%02X seq=0x%02X len=%u", static_cast<uint8_t>(req), this->tx_seq_, len);
  this->write_array(pkt, pkt_len);
  return this->tx_seq_;
}

}  // namespace pkt_p530
}  // namespace esphome
