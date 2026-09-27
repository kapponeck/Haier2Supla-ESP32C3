#pragma once

#include <Arduino.h>
#include <HaierProtocol.h>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <protocol/haier_protocol.h>
#include <string>
#include <utils/protocol_stream.h>
#include <vector>

#include "HaierSerialStream.h"
#include "hon_packet.h"
#include "smartair2_packet.h"

class HaierSmartair2Controller {
 public:
  HaierSmartair2Controller(HardwareSerial &serial,
                           uint8_t rx_pin,
                           uint8_t tx_pin,
                           uint32_t baud,
                           bool use_crc);

  void begin();
  void loop();

  void setPower(bool on);
  bool getPower() const { return power_state_; }
  bool hasStatus() const { return has_last_status_message_; }
  float getRoomTemperatureC() const;
  bool getModeCool() const;
  bool getModeHeat() const;
  bool getModeAuto() const;
  bool getModeFan() const;
  bool getModeDry() const;
  bool getFanHigh() const;
  bool getFanMid() const;
  bool getFanLow() const;
  bool getFanAuto() const;
  float getTargetTemperatureC() const;
  void setTargetTemperatureC(float temp);
  void increaseTargetTemperatureC(bool increase);
  void setACMode(Supla::haier::smartair2_protocol::ConditioningMode mode);
  void setFanMode(Supla::haier::smartair2_protocol::FanMode mode);
  void setUseSwingBits(bool use_bits);
  void setHorizontalSwing(bool h);
  void setVerticalSwing(bool v);
  void setTurbo(bool on);
  void setQuiet(bool on);
  void setDisplayStatus(bool on);
  void setUseFahrenheit(bool on);
  void setLockRemote(bool on);
  void setHealthMode(bool on);
  void setTenDegree(bool on);
  void triggerPowerSavingModeToggleSequence();
  bool isPowerSavingModeToggleSequenceActive() const;
  void triggerDisplayTemperatureToggleSequence();
  bool isDisplayTemperatureToggleSequenceActive() const;
  void setTargetTemperatureChangedCallback(std::function<void(float)> callback);

  haier_protocol::HaierMessage getControlMessage();
  Supla::haier::smartair2_protocol::FanMode getFanMode() const;
  Supla::haier::smartair2_protocol::ConditioningMode getACMode() const;
  uint8_t getRoomHumidity() const;
  bool getUseSwingBits() const;
  bool getHorizontalSwing() const;
  bool getVerticalSwing() const;
  bool getTurbo() const;
  bool getQuiet() const;
  bool getDisplayStatus() const;
  bool getUseFahrenheit() const;
  bool getLockRemote() const;
  bool getHealthMode() const;
  bool getCompressor() const;
  bool getTenDegree() const;

 private:
  enum class ProtocolPhase : uint8_t {
    START_DEVICE_VERSION,
    WAIT_DEVICE_VERSION,
    START_DEVICE_ID,
    WAIT_DEVICE_ID,
    START_FIRST_STATUS,
    WAIT_FIRST_STATUS,
    READY,
  };

  void sendDeviceVersionRequest_();
  void sendDeviceIdRequest_();
  void sendNetworkStatusReport_();
  void sendStatusRequest_(bool initial_request = false);

  haier_protocol::HandlerError handleDeviceVersionAnswer_(
      haier_protocol::FrameType request_type,
      haier_protocol::FrameType message_type,
      const uint8_t *data,
      size_t data_size);

  haier_protocol::HandlerError handleDeviceIdAnswer_(
      haier_protocol::FrameType request_type,
      haier_protocol::FrameType message_type,
      const uint8_t *data,
      size_t data_size);

  haier_protocol::HandlerError handleProtocolTimeout_(
      haier_protocol::FrameType request_type);

  haier_protocol::HandlerError handleNetworkStatusAnswer_(
      haier_protocol::FrameType request_type,
      haier_protocol::FrameType message_type,
      const uint8_t *data,
      size_t data_size);

  haier_protocol::HandlerError handleStatusAnswer_(
      haier_protocol::FrameType request_type,
      haier_protocol::FrameType message_type,
      const uint8_t *data,
      size_t data_size);

  uint8_t getWifiSignalStrength_() const;

  HardwareSerial &serial_;
  uint8_t rx_pin_;
  uint8_t tx_pin_;
  uint32_t baud_;
  bool use_crc_;
  bool hon_mode_ = false;

  HaierSerialStream stream_;
  haier_protocol::ProtocolHandler protocol_;
  ProtocolPhase protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
  std::chrono::steady_clock::time_point next_initialization_attempt_{};
  std::chrono::steady_clock::time_point last_network_status_report_;
  std::chrono::steady_clock::time_point last_status_request_;

  float room_temperature_c_ = NAN;
  bool power_state_ = false;
  Supla::haier::smartair2_protocol::ConditioningMode ac_mode_ =
      Supla::haier::smartair2_protocol::ConditioningMode::AUTO;
  float set_point_ = NAN;
  uint8_t room_humidity_ = 0;
  Supla::haier::smartair2_protocol::FanMode fan_mode_ =
      Supla::haier::smartair2_protocol::FanMode::FAN_AUTO;
  bool use_swing_bits_ = false;
  bool horizontal_swing_ = false;
  bool vertical_swing_ = false;
  bool turbo_mode_ = false;
  bool quiet_mode_ = false;
  bool display_status_ = false;
  bool use_fahrenheit_ = false;
  bool lock_remote_ = false;
  bool health_mode_ = false;
  bool compressor_ = false;
  bool ten_degree_ = false;

  std::array<uint8_t, sizeof(Supla::haier::smartair2_protocol::HaierPacketControl)> last_status_message_{};
  std::array<uint8_t, sizeof(Supla::haier::hon_protocol::HaierPacketControl)> last_hon_control_{};
  bool has_last_status_message_ = false;
  bool hon_energy_saving_ = false;
  std::vector<uint8_t> last_raw_packet_{};

  void logPacketDiff(const uint8_t *pkt, size_t len);
  void queueControlUpdate_();
  void notifyTargetTemperatureChanged_();
  void processPowerSavingModeToggleSequence_();
  void processDisplayTemperatureToggleSequence_();
  void sendDisplayToggleStep_(bool on);
  bool isAnyToggleSequenceActive_() const;

  struct PendingHvacSettings {
    bool valid = false;
    Supla::haier::smartair2_protocol::ConditioningMode ac_mode =
        Supla::haier::smartair2_protocol::ConditioningMode::AUTO;
    Supla::haier::smartair2_protocol::FanMode fan_mode =
        Supla::haier::smartair2_protocol::FanMode::FAN_AUTO;
    float set_point = NAN;
    bool use_swing_bits = false;
    bool horizontal_swing = false;
    bool vertical_swing = false;
    bool turbo_mode = false;
    bool quiet_mode = false;
    bool display_status = false;
    bool use_fahrenheit = false;
    bool lock_remote = false;
    bool health_mode = false;
    bool ten_degree = false;
  } pending_settings_;

  bool control_send_requested_ = false;
  std::function<void(float)> target_temperature_changed_callback_;

  struct PowerSavingModeToggleSequence {
    bool active = false;
    bool auto_mode_sent = false;
    bool next_health_mode = false;
    uint8_t toggles_remaining = 0;
    std::chrono::steady_clock::time_point next_step_at{};
  } power_saving_mode_toggle_sequence_;

  struct DisplayTemperatureToggleSequence {
    bool active = false;
    bool next_display_status = false;
    uint8_t toggles_remaining = 0;
    std::chrono::steady_clock::time_point next_step_at{};
  } display_temperature_toggle_sequence_;

  void sendControlNow();
  haier_protocol::HandlerError handleHonStatus_(const uint8_t *data,
                                                 size_t data_size);

  std::chrono::milliseconds control_retry_interval_{500};
  std::chrono::milliseconds network_status_report_interval_{5000};
  std::chrono::milliseconds power_saving_toggle_interval_{760};
  std::chrono::milliseconds power_saving_status_retry_interval_{250};
  std::chrono::milliseconds display_temperature_toggle_interval_{760};
  std::chrono::milliseconds display_temperature_status_retry_interval_{250};
};
