#pragma once

#include <stdint.h>

namespace esphome {
namespace pkt_p530 {

// Protocol constants
static const uint8_t PACKET_HEADER_SIZE = 5;  // magic(2) + len(1) + type(1) + seq(1)
static const uint8_t PACKET_CRC_SIZE = 2;
static const uint8_t PACKET_MIN_SIZE = PACKET_HEADER_SIZE + PACKET_CRC_SIZE;
static const uint8_t PACKET_MAX_SIZE = 0xFF;
static const uint8_t PACKET_PAYLOAD_MAX_SIZE = PACKET_MAX_SIZE - PACKET_MIN_SIZE;
static const uint8_t MAX_SEQ = 0xFF;  // 0xFF seq is for status report

// LED/Beep command target
enum class LedCtlTarget : uint8_t {
  UPPER_LED = 0x01,
  LOWER_LED = 0x02,
  BEEP = 0x03,
};

// Command types (ESP8266 -> ISD91230)
enum class ReqType : uint8_t {
  NONE = 0x00,
  GET_STATUS = 0x01,
  SET_PARAMS_A = 0x03,
  SET_PARAMS_B = 0x04,
  SET_PARAM_A = 0x05,
  SET_PARAM_B = 0x06,
  OPEN_DOOR = 0x07,
  CLOSE_DOOR = 0x09,
  DISPENSE = 0x0B,
  MOTOR_PARAMS = 0x0D,
  LED_CTL = 0x0E,
  MOTOR_CONFIG = 0x13,
};

// Response types (ISD91230 -> ESP8266)
enum class ReportType : uint8_t {
  NONE = 0x00,
  STATUS = 0x02,
  DOOR_OPEN_DONE = 0x08,
  DOOR_CLOSE_DONE = 0x0A,
  DISPENSE_DONE = 0x0C,
  MOTOR_READY = 0x14,
};

// Status packet payload (11 bytes)
struct StatusReport {
  uint8_t door_open{0};   // 0x00=closed, 0x01=opened
  uint8_t food_level{0};  // 0x00=low, 0x01=ok
  uint8_t ready{0};       // 0x00=not ready, 0x01=ok
  uint8_t _reserved[8];

  bool is_door_open() const { return door_open == 0x01; }
  bool has_food() const { return food_level == 0x01; }
  bool is_ready() const { return ready == 0x01; }
} __attribute__((packed));

}  // namespace pkt_p530
}  // namespace esphome
