#pragma once

#include "p530_component.h"
#include "protocol.h"

#include <esphome/core/log.h>
#include <esphome/core/component.h>
#include <esphome/core/base_automation.h>
#include <esphome/core/automation.h>

namespace esphome {
namespace pkt_p530 {

// ============== Base Action ==============

template<typename... Ts> class PktAction : public Action<Ts...>, public Parented<P530Component> {
 public:
  void set_wait_for_complete(bool v) { this->wait_for_complete_ = v; }
  void set_send_timeout(uint32_t ms) { this->send_timeout_ms_ = ms; }

  void add_on_complete(std::initializer_list<Action<Ts...> *> actions) {
    this->on_complete_.add_actions(actions);
    this->on_complete_.add_action(new LambdaAction<Ts...>([this](Ts... x) { this->play_next_(x...); }));
  }

  void add_on_error(std::initializer_list<Action<Ts...> *> actions) {
    this->on_error_.add_actions(actions);
    this->on_error_.add_action(new LambdaAction<Ts...>([this](Ts... x) { this->play_next_(x...); }));
  }

  void play_complex(const Ts &...x) override {
    this->num_running_++;
    this->args_ = std::make_tuple(x...);
    this->stage_ = Stage::IDLE;

    ErrorCode e = this->do_action_(x...);
    if (e != ErrorCode::OK) {
      this->finish_(e);
      return;
    }

    if (!this->wait_for_complete_) {
      this->finish_(ErrorCode::OK);
    }
  }

  void play(const Ts &...) override {}  // not used, see play_complex

  void stop() override {
    this->on_complete_.stop();
    this->on_error_.stop();
  }

 protected:
  // Hooks

  virtual ErrorCode do_action_(const Ts &...) { return ErrorCode::NOT_IMPLEMENTED; }

  virtual ErrorCode handle_report_(std::span<const uint8_t>) { return ErrorCode::OK; }

 protected:
  enum class NextStep : uint8_t {
    STOP,         // stop chain
    NEXT_STEP,    // play next script action
    ON_COMPLETE,  // play on_complete
    ON_ERROR,     // play on_error
  };

  enum Stage : uint8_t {
    IDLE,
    WAIT_ACK,
    WAIT_REPORT,
    FINISHED,
  };

  ErrorCode send_cmd_nowait_(ReqType req, const uint8_t *payload, uint8_t len) {
    uint8_t seq = this->parent_->send(req, payload, len);
    if (seq == MAX_SEQ) {
      return ErrorCode::SEND_FAILED;
    }

    return ErrorCode::OK;
  }

  ErrorCode send_cmd_(ReqType req, const uint8_t *payload, uint8_t len, ReportType wait_rtype = ReportType::NONE,
                      uint32_t wait_timeout_ms = 0) {
    uint8_t seq = this->parent_->send(req, payload, len);
    if (seq == MAX_SEQ)
      return ErrorCode::SEND_FAILED;

    this->stage_ = Stage::WAIT_ACK;
    this->expected_report_timeout_ = wait_timeout_ms;
    this->expected_report_seq_ = seq;
    this->expected_report_type_ = 0;
    if (wait_rtype != ReportType::NONE) {
      this->expected_report_type_ = static_cast<uint8_t>(wait_rtype);
    }

    uint8_t req_type = static_cast<uint8_t>(req);
    this->parent_->add_report_waiter(
        req_type, seq, this->send_timeout_ms_,
        std::bind(&PktAction::on_packet_, this, std::placeholders::_1, std::placeholders::_2));

    return ErrorCode::OK;
  }

  bool on_packet_(ErrorCode err, const std::span<const uint8_t> payload) {
    switch (this->stage_) {
      case Stage::WAIT_ACK:
        if (err != ErrorCode::OK) {
          this->finish_(err);
          return true;
        }

        if (payload.size() != 1 || payload[0] != 0x01) {
          // not ACK
          return false;
        }

        if (this->expected_report_type_ == 0) {
          this->finish_(ErrorCode::OK);
          return true;
        }

        // Need to wait for a report
        this->stage_ = Stage::WAIT_REPORT;
        this->parent_->add_report_waiter(
            this->expected_report_type_, this->expected_report_seq_, this->expected_report_timeout_,
            std::bind(&PktAction::on_packet_, this, std::placeholders::_1, std::placeholders::_2));
        return true;

      case Stage::WAIT_REPORT:
        if (err != ErrorCode::OK) {
          this->finish_(err);
          return true;
        }

        err = this->handle_report_(payload);
        if (err == ErrorCode::NOT_IMPLEMENTED) {
          // not our packet
          return false;
        }

        this->finish_(err);
        return true;

      default:
        return false;
    }
  }

  void finish_(ErrorCode err) {
    if (this->stage_ == Stage::FINISHED) {
      return;
    }

    this->stage_ = Stage::FINISHED;

    bool handled = false;
    if (err == ErrorCode::OK) {
      handled = maybe_play(this->on_complete_);
    } else {
      handled = maybe_play(this->on_error_);
    }

    if (!handled) {
      this->trigger_next_();
    }
  }

  template<typename List> bool maybe_play(List &list) {
    if (list.empty()) {
      return false;
    }

    std::apply([&](const Ts &...args) { list.play(args...); }, this->args_);
    return true;
  }

  void trigger_next_() {
    std::apply([this](const Ts &...args) { this->play_next_(args...); }, this->args_);
  }

 protected:
  Stage stage_{Stage::IDLE};

  uint8_t expected_report_type_{0};
  uint8_t expected_report_seq_{0};
  uint32_t expected_report_timeout_{0};

  std::tuple<Ts...> args_;
  uint32_t send_timeout_ms_{1000};
  bool wait_for_complete_{true};

  ActionList<Ts...> on_complete_;
  ActionList<Ts...> on_error_;
};

// ============== LED/Beep Actions ==============

template<LedCtlTarget target, typename... Ts> class LedCtlAction : public PktAction<Ts...> {
 public:
  void set_on_ms(uint16_t v) { this->on_ms_ = v; }
  void set_off_ms(uint16_t v) { this->off_ms_ = v; }
  void set_count(uint16_t v) { this->count_ = v; }

 protected:
  ErrorCode do_action_(const Ts &...x) override {
    uint8_t payload[] = {
        static_cast<uint8_t>(target), uint8_t(this->on_ms_ >> 8), uint8_t(this->on_ms_), uint8_t(this->off_ms_ >> 8),
        uint8_t(this->off_ms_),       uint8_t(this->count_ >> 8), uint8_t(this->count_),
    };
    return this->send_cmd_(ReqType::LED_CTL, payload, sizeof(payload));
  }

 private:
  uint16_t on_ms_{0};
  uint16_t off_ms_{0};
  uint16_t count_{65535};
};

template<typename... Ts> using LedUpperAction = LedCtlAction<LedCtlTarget::UPPER_LED, Ts...>;
template<typename... Ts> using LedLowerAction = LedCtlAction<LedCtlTarget::LOWER_LED, Ts...>;
template<typename... Ts> using BeepAction = LedCtlAction<LedCtlTarget::BEEP, Ts...>;

// ============== Door Actions ==============

template<ReqType req, ReportType report, typename... Ts> class DoorCtlAction : public PktAction<Ts...> {
 public:
  void set_timeout(uint32_t ms) { this->timeout_ms_ = ms; }
  void set_duration(uint8_t v) { this->duration_ = v; }

 protected:
  ErrorCode do_action_(const Ts &...x) override {
    return this->send_cmd_(req, &this->duration_, sizeof(uint8_t), report, this->timeout_ms_);
  }

  ErrorCode handle_report_(const std::span<const uint8_t> payload) override {
    if (payload.empty()) {
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

template<typename... Ts> class DispenseAction : public PktAction<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t, portions)

 protected:
  ErrorCode do_action_(const Ts &...x) override {
    if (!this->parent_->has_food()) {
      return ErrorCode::NO_FOOD;
    }

    uint8_t portions = this->portions_.value(x...);
    uint8_t payload[] = {portions, 0x01, 0x01, 0x50};

    // 1 portion about 1.65s
    uint32_t timeout = portions * 3000;
    return this->send_cmd_(ReqType::DISPENSE, payload, sizeof(payload), ReportType::DISPENSE_DONE, timeout);
  }

  ErrorCode handle_report_(const std::span<const uint8_t> payload) override {
    if (payload.size() < 3 || payload[2] == 0x00) {
      return ErrorCode::NOT_IMPLEMENTED;  // not done yet
    }

    return ErrorCode::OK;
  }
};

// ============== Init Action ==============

template<typename... Ts> class InitAction : public PktAction<Ts...> {
 protected:
  ErrorCode do_action_(const Ts &...x) override {
    static const uint8_t MOTOR_CONFIG[] = {0x05, 0x7E};
    static const uint8_t SET_PARAMS_A[] = {0x00, 0x05, 0x00, 0x05};
    static const uint8_t SET_PARAM_A[] = {0x00, 0x05};
    static const uint8_t SET_PARAMS_B[] = {0x00, 0xFF, 0x00, 0xFF};
    static const uint8_t SET_PARAM_B[] = {0xFF, 0xFF};
    static const uint8_t MOTOR_PARAMS[] = {0x00, 0x3C, 0x01, 0x90, 0x0F, 0x01, 0x22, 0x22, 0x01, 0xF4, 0x0F, 0x01};

    this->send_cmd_nowait_(ReqType::GET_STATUS, nullptr, 0);
    this->send_cmd_nowait_(ReqType::MOTOR_CONFIG, MOTOR_CONFIG, sizeof(MOTOR_CONFIG));
    this->send_cmd_nowait_(ReqType::SET_PARAMS_A, SET_PARAMS_A, sizeof(SET_PARAMS_A));
    this->send_cmd_nowait_(ReqType::SET_PARAM_A, SET_PARAM_A, sizeof(SET_PARAM_A));
    this->send_cmd_nowait_(ReqType::SET_PARAMS_B, SET_PARAMS_B, sizeof(SET_PARAMS_B));
    this->send_cmd_nowait_(ReqType::SET_PARAM_B, SET_PARAM_B, sizeof(SET_PARAM_B));
    this->send_cmd_nowait_(ReqType::MOTOR_PARAMS, MOTOR_PARAMS, sizeof(MOTOR_PARAMS));

    return this->send_cmd_(ReqType::GET_STATUS, nullptr, 0);
  }
};

// ============== Conditions ==============

template<typename... Ts> class IsReadyCondition : public Condition<Ts...>, public Parented<P530Component> {
 public:
  bool check(const Ts &...x) override { return this->parent_->is_ready(); }
};

template<typename... Ts> class HasFoodCondition : public Condition<Ts...>, public Parented<P530Component> {
 public:
  bool check(const Ts &...x) override { return this->parent_->has_food(); }
};

}  // namespace pkt_p530
}  // namespace esphome
