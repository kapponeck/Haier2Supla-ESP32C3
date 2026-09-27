#pragma once

#include <cstdint>

namespace Supla {
namespace haier {
namespace hon_protocol {

constexpr uint16_t kGetUserData = 0x4D01;
constexpr uint16_t kSetGroupParameters = 0x6001;

enum class ConditioningMode : uint8_t {
  AUTO = 0x00,
  COOL = 0x01,
  DRY = 0x02,
  HEALTHY_DRY = 0x03,
  HEAT = 0x04,
  ENERGY_SAVING = 0x05,
  FAN = 0x06,
};

enum class FanMode : uint8_t {
  FAN_HIGH = 0x01,
  FAN_MID = 0x02,
  FAN_LOW = 0x03,
  FAN_AUTO = 0x05,
};

enum class VerticalSwingMode : uint8_t {
  CENTER = 0x06,
  AUTO = 0x0C,
  AUTO_SPECIAL = 0x0E,
};

enum class HorizontalSwingMode : uint8_t {
  CENTER = 0x00,
  AUTO = 0x07,
};

struct __attribute__((packed)) HaierPacketControl {
  uint8_t set_point;
  uint8_t vertical_swing_mode : 4;
  uint8_t : 4;
  uint8_t fan_mode : 3;
  uint8_t special_mode : 2;
  uint8_t ac_mode : 3;
  uint8_t : 8;
  uint8_t ten_degree : 1;
  uint8_t display_status : 1;
  uint8_t half_degree : 1;
  uint8_t intelligence_status : 1;
  uint8_t pmv_status : 1;
  uint8_t use_fahrenheit : 1;
  uint8_t : 1;
  uint8_t steri_clean : 1;
  uint8_t ac_power : 1;
  uint8_t health_mode : 1;
  uint8_t electric_heating_status : 1;
  uint8_t fast_mode : 1;
  uint8_t quiet_mode : 1;
  uint8_t sleep_mode : 1;
  uint8_t lock_remote : 1;
  uint8_t beeper_status : 1;
  uint8_t target_humidity;
  uint8_t horizontal_swing_mode : 3;
  uint8_t : 3;
  uint8_t human_sensing_status : 2;
  uint8_t change_filter : 1;
  uint8_t : 7;
  uint8_t fresh_air_status : 1;
  uint8_t humidification_status : 1;
  uint8_t pm2p5_cleaning_status : 1;
  uint8_t ch2o_cleaning_status : 1;
  uint8_t self_cleaning_status : 1;
  uint8_t light_status : 1;
  uint8_t energy_saving_status : 1;
  uint8_t cleaning_time_status : 1;
};

struct __attribute__((packed)) HaierPacketSensors {
  uint8_t room_temperature;
  uint8_t room_humidity;
  uint8_t outdoor_temperature;
  uint8_t pm2p5_level : 2;
  uint8_t air_quality : 2;
  uint8_t human_sensing : 2;
  uint8_t : 1;
  uint8_t ac_type : 1;
  uint8_t error_status;
  uint8_t operation_source : 2;
  uint8_t operation_mode_hk : 2;
  uint8_t : 3;
  uint8_t err_confirmation : 1;
  uint16_t total_cleaning_time;
  uint16_t indoor_pm2p5_value;
  uint16_t outdoor_pm2p5_value;
  uint16_t ch2o_value;
  uint16_t voc_value;
  uint16_t co2_value;
};

static_assert(sizeof(HaierPacketControl) == 10,
              "Unexpected hOn control packet size");
static_assert(sizeof(HaierPacketSensors) == 18,
              "Unexpected hOn sensors packet size");

}  // namespace hon_protocol
}  // namespace haier
}  // namespace Supla
