#pragma once

#include <cstdint>

namespace Supla {
namespace haier {
namespace smartair2_protocol {

constexpr float kSetpointMinC = 16.0f;
constexpr float kSetpointMaxC = 30.0f;
constexpr uint8_t kSetpointOffsetC = 16;

enum class ConditioningMode : uint8_t {
  AUTO = 0x00,
  COOL = 0x01,
  HEAT = 0x02,
  FAN  = 0x03,
  DRY  = 0x04
};

enum class FanMode : uint8_t {
  FAN_HIGH = 0x00,
  FAN_MID  = 0x01,
  FAN_LOW  = 0x02,
  FAN_AUTO = 0x03
};

struct __attribute__((packed)) HaierPacketControl {
  uint8_t : 8;
  uint8_t room_temperature;
  uint8_t : 8;
  uint8_t room_humidity;
  uint8_t : 8;
  uint8_t cntrl;
  uint8_t : 8;
  uint8_t : 8;
  uint8_t : 8;
  uint8_t : 8;
  uint8_t : 8;
  uint8_t ac_mode;
  uint8_t : 8;
  uint8_t fan_mode;
  uint8_t : 8;
  uint8_t vertical_swing : 1;
  uint8_t horizontal_swing : 1;
  uint8_t : 0;
  uint8_t : 3;
  uint8_t use_fahrenheit : 1;
  uint8_t : 3;
  uint8_t lock_remote : 1;
  uint8_t ac_power : 1;
  uint8_t : 2;
  uint8_t health_mode : 1;
  uint8_t compressor : 1;
  uint8_t : 1;
  uint8_t ten_degree : 1;
  uint8_t : 0;
  uint8_t : 8;
  uint8_t use_swing_bits : 1;
  uint8_t turbo_mode : 1;
  uint8_t quiet_mode : 1;
  uint8_t : 2;
  uint8_t display_status : 1;
  uint8_t : 0;
  uint8_t : 8;
  uint8_t : 8;
  uint8_t : 8;
  uint8_t set_point;
};

struct __attribute__((packed)) HaierStatus {
  uint16_t subcommand;
  HaierPacketControl control;
};

inline float decodeSetPointC(uint8_t raw_set_point) {
  return static_cast<float>(raw_set_point + kSetpointOffsetC);
}

inline uint8_t encodeSetPointC(float set_point_c) {
  if (set_point_c < kSetpointMinC) {
    set_point_c = kSetpointMinC;
  } else if (set_point_c > kSetpointMaxC) {
    set_point_c = kSetpointMaxC;
  }

  int rounded = static_cast<int>(set_point_c + 0.5f);
  return static_cast<uint8_t>(rounded - kSetpointOffsetC);
}

}  // namespace smartair2_protocol
}  // namespace haier
}  // namespace Supla
