#pragma once

#include "p530_component.h"
#include "protocol.h"

#include <esphome/core/log.h>
#include <esphome/core/component.h>
#include <esphome/core/base_automation.h>
#include <esphome/core/automation.h>

namespace esphome {
namespace pkt_p530 {

static const char *const TAG = "pkt_p530::actions";

// ============== Triggers ==============

class ErrorTrigger : public Trigger<ErrorCode> {
 public:
  explicit ErrorTrigger(P530Component *parent) {
    parent->add_on_error_callback([this](ErrorCode code) { this->trigger(code); });
  }
};

class DoorBlockedTrigger : public Trigger<> {
 public:
  explicit DoorBlockedTrigger(P530Component *parent) {
    parent->add_on_door_blocked_callback([this]() { this->trigger(); });
  }
};

class DispenseCompleteTrigger : public Trigger<uint8_t> {
 public:
  explicit DispenseCompleteTrigger(P530Component *parent) {
    parent->add_on_dispense_complete_callback([this](uint8_t portions) { this->trigger(portions); });
  }
};

// ============== Base Action ==============

using ActionCallback = std::function<void(ErrorCode)>;

template<typename... Ts> class BaseAction : public Action<Ts...>, public Parented<P530Component> {
 public:
  void set_wait_for_complete(bool wait) { this->wait_for_complete_ = wait; }
  void set_continue_on_error(bool v) { this->continue_on_error_ = v; }
  void set_send_timeout(uint32_t ms) { this->send_timeout_ms_ = ms; }

  void add_on_complete(const std::initializer_list<Action<Ts...> *> &actions) {
    this->on_complete_.add_actions(actions);
  }

  void add_on_error(const std::initializer_list<Action<Ts...> *> &actions) { this->on_error_.add_actions(actions); }

  void play_complex(const Ts &...x) override {
    this->num_running_++;
    this->stashed_args_ = std::make_tuple(x...);

    ErrorCode err = this->do_action();
    if (err != ErrorCode::OK) {
      this->finish_(err);
      return;
    }

    if (!this->wait_for_complete_) {
      this->play_next_tuple_();
    }
  }

  void play(const Ts &...x) override {}

  void stop() override {
    this->on_complete_.stop();
    this->on_error_.stop();
  }

 protected:
  virtual ErrorCode do_action() { return ErrorCode::NOT_IMPLEMENTED; }

  void finish_(ErrorCode err) {
    if (err == ErrorCode::OK) {
      this->on_complete_.empty() ? this->play_next_tuple_() : this->play_complete_();
      return;
    }

    if (!this->on_error_.empty()) {
      this->play_error_();
      return;
    }

    if (this->wait_for_complete_) {
      this->continue_on_error_ ? this->play_next_tuple_() : this->stop_complex();
    }
  }

  // Send command and wait for ACK, then call finish_()
  ErrorCode send_cmd(ReqType req, const uint8_t *payload, uint8_t len) {
    uint8_t seq = this->parent_->send(req, payload, len);
    if (seq == MAX_SEQ) {
      return ErrorCode::SEND_FAILED;
    }

    uint8_t req_type = static_cast<uint8_t>(req);
    this->parent_->add_report_waiter(
        req_type, seq, this->send_timeout_ms_,
        std::bind(&BaseAction::on_ack_, this, req_type, std::placeholders::_1, std::placeholders::_2));
    return ErrorCode::OK;
  }

  // Send command without waiting
  ErrorCode send_cmd_nowait(ReqType req, const uint8_t *payload, uint8_t len) {
    uint8_t seq = this->parent_->send(req, payload, len);
    return seq == MAX_SEQ ? ErrorCode::SEND_FAILED : ErrorCode::OK;
  }

  // Send command, wait for ACK, then wait for report
  ErrorCode send_cmd_with_report(ReqType req, const uint8_t *payload, uint8_t len, ReportType report_type,
                                 uint32_t report_timeout_ms) {
    uint8_t seq = this->parent_->send(req, payload, len);
    if (seq == MAX_SEQ) {
      return ErrorCode::SEND_FAILED;
    }

    this->pending_report_type_ = static_cast<uint8_t>(report_type);
    this->pending_report_timeout_ = report_timeout_ms;

    uint8_t req_type = static_cast<uint8_t>(req);
    this->parent_->add_report_waiter(
        req_type, seq, this->send_timeout_ms_,
        std::bind(&BaseAction::on_ack_then_report_, this, req_type, std::placeholders::_1, std::placeholders::_2));
    return ErrorCode::OK;
  }

  // Override in derived classes to handle report payload
  virtual ErrorCode handle_report(std::basic_string_view<uint8_t> payload) { return ErrorCode::OK; }

 private:
  bool on_ack_(uint8_t req_type, ErrorCode err, std::basic_string_view<uint8_t> payload) {
    if (err != ErrorCode::OK) {
      ESP_LOGW(TAG, "Command 0x%02X failed: %d", req_type, static_cast<uint8_t>(err));
      this->finish_(err);
      return true;
    }

    if (payload.size() != 1 || payload[0] != 0x01) {
      return false;  // not ACK
    }

    this->finish_(ErrorCode::OK);
    return true;
  }

  bool on_ack_then_report_(uint8_t req_type, ErrorCode err, std::basic_string_view<uint8_t> payload) {
    if (err != ErrorCode::OK) {
      ESP_LOGW(TAG, "Command 0x%02X failed: %d", req_type, static_cast<uint8_t>(err));
      this->finish_(err);
      return true;
    }

    if (payload.size() != 1 || payload[0] != 0x01) {
      return false;  // not ACK
    }

    // ACK received, now wait for report
    this->parent_->add_report_waiter(
        this->pending_report_type_, 0, this->pending_report_timeout_,
        std::bind(&BaseAction::on_report_, this, std::placeholders::_1, std::placeholders::_2));
    return true;
  }

  bool on_report_(ErrorCode err, std::basic_string_view<uint8_t> payload) {
    if (err != ErrorCode::OK) {
      this->finish_(err);
      return true;
    }

    ErrorCode result = this->handle_report(payload);
    if (result == ErrorCode::NOT_IMPLEMENTED) {
      return false;  // not our packet, keep waiting
    }

    this->finish_(result);
    return true;
  }

  void play_next_tuple_() {
    std::apply([this](const Ts &...args) { this->play_next_(args...); }, this->stashed_args_);
  }

  void play_complete_() {
    std::apply([this](const Ts &...args) { this->on_complete_.play(args...); }, this->stashed_args_);
  }

  void play_error_() {
    std::apply([this](const Ts &...args) { this->on_error_.play(args...); }, this->stashed_args_);
  }

 private:
  ActionList<Ts...> on_complete_;
  ActionList<Ts...> on_error_;
  std::tuple<Ts...> stashed_args_;
  uint32_t send_timeout_ms_{1000};
  uint8_t pending_report_type_{0};
  uint32_t pending_report_timeout_{0};
  bool wait_for_complete_{false};
  bool continue_on_error_{false};
};

// ============== LED/Beep Actions ==============

template<LedCtlTarget target, typename... Ts> class LedCtlAction : public BaseAction<Ts...> {
 public:
  void set_on_ms(uint16_t v) { this->on_ms_ = v; }
  void set_off_ms(uint16_t v) { this->off_ms_ = v; }
  void set_count(uint16_t v) { this->count_ = v; }

 protected:
  ErrorCode do_action() override {
    uint8_t payload[] = {
        static_cast<uint8_t>(target), uint8_t(this->on_ms_ >> 8), uint8_t(this->on_ms_), uint8_t(this->off_ms_ >> 8),
        uint8_t(this->off_ms_),       uint8_t(this->count_ >> 8), uint8_t(this->count_),
    };
    return this->send_cmd(ReqType::LED_CTL, payload, sizeof(payload));
  }

 private:
  uint16_t on_ms_{100};
  uint16_t off_ms_{100};
  uint16_t count_{1};
};

template<typename... Ts> using LedUpperAction = LedCtlAction<LedCtlTarget::UPPER_LED, Ts...>;
template<typename... Ts> using LedLowerAction = LedCtlAction<LedCtlTarget::LOWER_LED, Ts...>;
template<typename... Ts> using BeepAction = LedCtlAction<LedCtlTarget::BEEP, Ts...>;

// ============== Door Actions ==============

template<ReqType req, ReportType report, typename... Ts> class DoorCtlAction : public BaseAction<Ts...> {
 public:
  void set_timeout(uint32_t ms) { this->timeout_ms_ = ms; }
  void set_duration(uint8_t v) { this->duration_ = v; }

 protected:
  ErrorCode do_action() override {
    return this->send_cmd_with_report(req, &this->duration_, 1, report, this->timeout_ms_);
  }

  ErrorCode handle_report(std::basic_string_view<uint8_t> payload) override {
    if (payload.size() < 1) {
      return ErrorCode::NOT_IMPLEMENTED;  // not our packet
    }
    return payload[0] == 0x02 ? ErrorCode::OK : ErrorCode::DOOR_BLOCKED;
  }

 private:
  uint8_t duration_{0x1E};
  uint32_t timeout_ms_{10000};
};

template<typename... Ts> using DoorOpenAction = DoorCtlAction<ReqType::OPEN_DOOR, ReportType::DOOR_OPEN_DONE, Ts...>;
template<typename... Ts> using DoorCloseAction = DoorCtlAction<ReqType::CLOSE_DOOR, ReportType::DOOR_CLOSE_DONE, Ts...>;

// ============== Dispense Action ==============

template<typename... Ts> class DispenseAction : public BaseAction<Ts...> {
 public:
  void set_portions(uint8_t v) { this->portions_ = v; }
  void set_timeout(uint32_t ms) { this->timeout_ms_ = ms; }

 protected:
  ErrorCode do_action() override {
    if (!this->parent_->has_food()) {
      return ErrorCode::NO_FOOD;
    }

    uint8_t payload[] = {this->portions_, 0x01, 0x01, 0x50};
    uint32_t timeout = this->timeout_ms_ > 0 ? this->timeout_ms_ : this->portions_ * 3000;
    return this->send_cmd_with_report(ReqType::DISPENSE, payload, sizeof(payload), ReportType::DISPENSE_DONE, timeout);
  }

  ErrorCode handle_report(std::basic_string_view<uint8_t> payload) override {
    if (payload.size() < 3 || payload[2] != 0x01) {
      return ErrorCode::NOT_IMPLEMENTED;  // not done yet
    }
    return ErrorCode::OK;
  }

 private:
  uint8_t portions_{1};
  uint32_t timeout_ms_{0};
};

// ============== Init Action ==============

template<typename... Ts> class InitAction : public BaseAction<Ts...> {
 protected:
  ErrorCode do_action() override {
    static const uint8_t MOTOR_CONFIG[] = {0x05, 0x7E};
    static const uint8_t SET_PARAMS_A[] = {0x00, 0x05, 0x00, 0x05};
    static const uint8_t SET_PARAM_A[] = {0x00, 0x05};
    static const uint8_t SET_PARAMS_B[] = {0x00, 0xFF, 0x00, 0xFF};
    static const uint8_t SET_PARAM_B[] = {0xFF, 0xFF};
    static const uint8_t MOTOR_PARAMS[] = {0x00, 0x3C, 0x01, 0x90, 0x0F, 0x01, 0x22, 0x22, 0x01, 0xF4, 0x0F, 0x01};

    this->send_cmd_nowait(ReqType::GET_STATUS, nullptr, 0);
    this->send_cmd_nowait(ReqType::MOTOR_CONFIG, MOTOR_CONFIG, sizeof(MOTOR_CONFIG));
    this->send_cmd_nowait(ReqType::SET_PARAMS_A, SET_PARAMS_A, sizeof(SET_PARAMS_A));
    this->send_cmd_nowait(ReqType::SET_PARAM_A, SET_PARAM_A, sizeof(SET_PARAM_A));
    this->send_cmd_nowait(ReqType::SET_PARAMS_B, SET_PARAMS_B, sizeof(SET_PARAMS_B));
    this->send_cmd_nowait(ReqType::SET_PARAM_B, SET_PARAM_B, sizeof(SET_PARAM_B));
    this->send_cmd_nowait(ReqType::MOTOR_PARAMS, MOTOR_PARAMS, sizeof(MOTOR_PARAMS));

    return this->send_cmd(ReqType::GET_STATUS, nullptr, 0);
  }
};

// ============== Conditions ==============

template<typename... Ts> class IsReadyCondition : public Condition<Ts...>, public Parented<P530Component> {
 public:
  bool check(Ts... x) override { return this->parent_->is_ready(); }
};

template<typename... Ts> class HasFoodCondition : public Condition<Ts...>, public Parented<P530Component> {
 public:
  bool check(Ts... x) override { return this->parent_->has_food(); }
};

}  // namespace pkt_p530
}  // namespace esphome
