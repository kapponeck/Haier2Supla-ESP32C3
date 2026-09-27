#include "HaierSmartair2Controller.h"

#include <WiFi.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <supla/log_wrapper.h>
#include <utils/haier_log.h>

using namespace haier_protocol;
using namespace Supla::haier::smartair2_protocol;

namespace {

constexpr uint8_t kInitializationRetries = 2;
constexpr std::chrono::milliseconds kInitializationRetryInterval(2000);

ConditioningMode fromHonMode(uint8_t mode) {
  using HonMode = Supla::haier::hon_protocol::ConditioningMode;
  switch (static_cast<HonMode>(mode)) {
    case HonMode::COOL: return ConditioningMode::COOL;
    case HonMode::HEAT: return ConditioningMode::HEAT;
    case HonMode::FAN: return ConditioningMode::FAN;
    case HonMode::DRY:
    case HonMode::HEALTHY_DRY: return ConditioningMode::DRY;
    default: return ConditioningMode::AUTO;
  }
}

uint8_t toHonMode(ConditioningMode mode) {
  using HonMode = Supla::haier::hon_protocol::ConditioningMode;
  switch (mode) {
    case ConditioningMode::COOL: return static_cast<uint8_t>(HonMode::COOL);
    case ConditioningMode::HEAT: return static_cast<uint8_t>(HonMode::HEAT);
    case ConditioningMode::FAN: return static_cast<uint8_t>(HonMode::FAN);
    case ConditioningMode::DRY: return static_cast<uint8_t>(HonMode::DRY);
    default: return static_cast<uint8_t>(HonMode::AUTO);
  }
}

FanMode fromHonFan(uint8_t fan) {
  using HonFan = Supla::haier::hon_protocol::FanMode;
  switch (static_cast<HonFan>(fan)) {
    case HonFan::FAN_HIGH: return FanMode::FAN_HIGH;
    case HonFan::FAN_MID: return FanMode::FAN_MID;
    case HonFan::FAN_LOW: return FanMode::FAN_LOW;
    default: return FanMode::FAN_AUTO;
  }
}

uint8_t toHonFan(FanMode fan) {
  using HonFan = Supla::haier::hon_protocol::FanMode;
  switch (fan) {
    case FanMode::FAN_HIGH: return static_cast<uint8_t>(HonFan::FAN_HIGH);
    case FanMode::FAN_MID: return static_cast<uint8_t>(HonFan::FAN_MID);
    case FanMode::FAN_LOW: return static_cast<uint8_t>(HonFan::FAN_LOW);
    default: return static_cast<uint8_t>(HonFan::FAN_AUTO);
  }
}

void forwardHaierProtocolLog(haier_protocol::HaierLogLevel level,
                             const char *tag,
                             const char *message) {
  const char *safe_tag = tag != nullptr ? tag : "haier.protocol";
  const char *safe_message = message != nullptr ? message : "";
  switch (level) {
    case haier_protocol::HaierLogLevel::LEVEL_ERROR:
      SUPLA_LOG_ERROR("[%s] %s", safe_tag, safe_message);
      break;
    case haier_protocol::HaierLogLevel::LEVEL_WARNING:
      SUPLA_LOG_WARNING("[%s] %s", safe_tag, safe_message);
      break;
    case haier_protocol::HaierLogLevel::LEVEL_INFO:
      SUPLA_LOG_INFO("[%s] %s", safe_tag, safe_message);
      break;
    default:
      SUPLA_LOG_DEBUG("[%s] %s", safe_tag, safe_message);
      break;
  }
}

}  // namespace

HaierSmartair2Controller::HaierSmartair2Controller(HardwareSerial &serial,
                                                   uint8_t rx_pin,
                                                   uint8_t tx_pin,
                                                   uint32_t baud,
                                                   bool use_crc)
    : serial_(serial),
      rx_pin_(rx_pin),
      tx_pin_(tx_pin),
      baud_(baud),
      use_crc_(use_crc),
      stream_(serial_),
      protocol_(stream_) {}

void HaierSmartair2Controller::begin() {
  SUPLA_LOG_DEBUG("Haier controller begin: baud=%u rx=%d tx=%d", (unsigned)baud_, rx_pin_, tx_pin_);

  haier_protocol::set_log_handler(forwardHaierProtocolLog);

  protocol_.set_answer_handler(
      FrameType::GET_DEVICE_VERSION,
      std::bind(&HaierSmartair2Controller::handleDeviceVersionAnswer_, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4));

  protocol_.set_answer_handler(
      FrameType::GET_DEVICE_ID,
      std::bind(&HaierSmartair2Controller::handleDeviceIdAnswer_, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4));

  protocol_.set_answer_handler(
      FrameType::CONTROL,
      std::bind(&HaierSmartair2Controller::handleStatusAnswer_, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4));
  protocol_.set_answer_handler(
      FrameType::REPORT_NETWORK_STATUS,
      std::bind(&HaierSmartair2Controller::handleNetworkStatusAnswer_, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4));
  protocol_.set_default_timeout_handler(
      std::bind(&HaierSmartair2Controller::handleProtocolTimeout_, this,
                std::placeholders::_1));

  const auto now = std::chrono::steady_clock::now();
  protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
  next_initialization_attempt_ = now;
  last_status_request_ = now;
  last_network_status_report_ = now;
}

void HaierSmartair2Controller::loop() {
  protocol_.loop();

  const auto now = std::chrono::steady_clock::now();
  const bool protocol_idle = !protocol_.is_waiting_for_answer() &&
                             protocol_.get_outgoing_queue_size() == 0;

  if (protocol_phase_ != ProtocolPhase::READY) {
    if (protocol_idle && now >= next_initialization_attempt_) {
      if (protocol_phase_ == ProtocolPhase::START_DEVICE_VERSION) {
        sendDeviceVersionRequest_();
        protocol_phase_ = ProtocolPhase::WAIT_DEVICE_VERSION;
      } else if (protocol_phase_ == ProtocolPhase::START_DEVICE_ID) {
        sendDeviceIdRequest_();
        protocol_phase_ = ProtocolPhase::WAIT_DEVICE_ID;
      } else if (protocol_phase_ == ProtocolPhase::START_FIRST_STATUS) {
        sendStatusRequest_(true);
        last_status_request_ = now;
        protocol_phase_ = ProtocolPhase::WAIT_FIRST_STATUS;
      }
    }
    return;
  }

  processPowerSavingModeToggleSequence_();
  processDisplayTemperatureToggleSequence_();

  const bool toggle_sequence_active = isAnyToggleSequenceActive_();

  const auto network_status_elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now - last_network_status_report_);
  if (!toggle_sequence_active &&
      network_status_elapsed >= network_status_report_interval_ &&
      !protocol_.is_waiting_for_answer() &&
      protocol_.get_outgoing_queue_size() == 0) {
    sendNetworkStatusReport_();
    last_network_status_report_ = now;
    return;
  }

  if (pending_settings_.valid && !control_send_requested_ &&
      !protocol_.is_waiting_for_answer() &&
      protocol_.get_outgoing_queue_size() == 0) {
    if (has_last_status_message_) {
      sendControlNow();
      control_send_requested_ = true;
    } else {
      sendStatusRequest_();
      last_status_request_ = std::chrono::steady_clock::now();
    }
  }

  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_status_request_);

  if (!toggle_sequence_active &&
      elapsed.count() > 1000 &&
      !protocol_.is_waiting_for_answer() &&
      protocol_.get_outgoing_queue_size() == 0) {
    sendStatusRequest_();
    last_status_request_ = now;
  }
}

void HaierSmartair2Controller::sendDeviceIdRequest_() {
  static const HaierMessage device_id_request(FrameType::GET_DEVICE_ID);
  SUPLA_LOG_INFO("Haier hOn: requesting device ID with CRC");
  protocol_.send_message(device_id_request, true,
                         kInitializationRetries,
                         kInitializationRetryInterval);
}

void HaierSmartair2Controller::sendDeviceVersionRequest_() {
  // SmartAir2 module capabilities, matching the initialization used by the
  // reference haier-esphome implementation.
  static const uint8_t module_capabilities[2] = {0x00, 0x07};
  static const HaierMessage device_version_request(
      FrameType::GET_DEVICE_VERSION,
      module_capabilities,
      sizeof(module_capabilities));
  SUPLA_LOG_INFO("Haier SmartAir2: requesting device version");
  protocol_.send_message(device_version_request, use_crc_,
                         kInitializationRetries,
                         kInitializationRetryInterval);
}

void HaierSmartair2Controller::sendNetworkStatusReport_() {
  const uint8_t payload[4] = {0x00, 0x00, 0x00, getWifiSignalStrength_()};
  const HaierMessage network_status(FrameType::REPORT_NETWORK_STATUS, payload,
                                    sizeof(payload));
  protocol_.send_message(network_status, use_crc_);
}

void HaierSmartair2Controller::sendStatusRequest_(bool initial_request) {
  static const HaierMessage STATUS_REQUEST(
      FrameType::CONTROL, Supla::haier::hon_protocol::kGetUserData);
  if (initial_request) {
    SUPLA_LOG_INFO("Haier %s: requesting first status",
                   hon_mode_ ? "hOn" : "SmartAir2");
    protocol_.send_message(STATUS_REQUEST, use_crc_, kInitializationRetries,
                           kInitializationRetryInterval);
  } else {
    protocol_.send_message(STATUS_REQUEST, use_crc_);
  }
}

HandlerError HaierSmartair2Controller::handleDeviceVersionAnswer_(
    FrameType request_type,
    FrameType message_type,
    const uint8_t *data,
    size_t data_size) {
  if (request_type != FrameType::GET_DEVICE_VERSION) {
    return HandlerError::UNEXPECTED_MESSAGE;
  }

  if (message_type != FrameType::GET_DEVICE_VERSION_RESPONSE &&
      message_type != FrameType::INVALID) {
    SUPLA_LOG_WARNING(
        "Haier SmartAir2: unexpected device version response type=0x%02X",
        static_cast<unsigned>(message_type));
    protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
    next_initialization_attempt_ =
        std::chrono::steady_clock::now() + kInitializationRetryInterval;
    return HandlerError::INVALID_ANSWER;
  }

  if (message_type == FrameType::GET_DEVICE_VERSION_RESPONSE &&
      data != nullptr && data_size >= 38 && (data[37] & 0x04) != 0) {
    hon_mode_ = true;
    use_crc_ = true;
    SUPLA_LOG_INFO("Haier: hOn protocol detected, CRC enabled");
  }

  SUPLA_LOG_INFO(
      "Haier SmartAir2: initialization response received type=0x%02X "
      "size=%u",
      static_cast<unsigned>(message_type), static_cast<unsigned>(data_size));
  protocol_phase_ = hon_mode_ ? ProtocolPhase::START_DEVICE_ID
                              : ProtocolPhase::START_FIRST_STATUS;
  next_initialization_attempt_ = std::chrono::steady_clock::now();
  return HandlerError::HANDLER_OK;
}

HandlerError HaierSmartair2Controller::handleDeviceIdAnswer_(
    FrameType request_type,
    FrameType message_type,
    const uint8_t *data,
    size_t data_size) {
  if (request_type != FrameType::GET_DEVICE_ID ||
      message_type != FrameType::GET_DEVICE_ID_RESPONSE) {
    SUPLA_LOG_WARNING(
        "Haier hOn: invalid device ID response type=0x%02X size=%u",
        static_cast<unsigned>(message_type), static_cast<unsigned>(data_size));
    protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
    next_initialization_attempt_ =
        std::chrono::steady_clock::now() + kInitializationRetryInterval;
    return HandlerError::INVALID_ANSWER;
  }

  SUPLA_LOG_INFO(
      "Haier hOn: device ID received size=%u; requesting status "
      "with CRC",
      static_cast<unsigned>(data_size));
  protocol_phase_ = ProtocolPhase::START_FIRST_STATUS;
  next_initialization_attempt_ = std::chrono::steady_clock::now();
  return HandlerError::HANDLER_OK;
}

HandlerError HaierSmartair2Controller::handleProtocolTimeout_(
    FrameType request_type) {
  if (request_type == FrameType::GET_DEVICE_VERSION &&
      protocol_phase_ == ProtocolPhase::WAIT_DEVICE_VERSION) {
    // Some SmartAir2 units do not return a useful version frame. The
    // reference implementation still proceeds with the first status request.
    SUPLA_LOG_WARNING(
        "Haier SmartAir2: device version timeout, continuing with status "
        "request");
    protocol_phase_ = ProtocolPhase::START_FIRST_STATUS;
    next_initialization_attempt_ = std::chrono::steady_clock::now();
    return HandlerError::HANDLER_OK;
  }

  if (request_type == FrameType::GET_DEVICE_ID &&
      protocol_phase_ == ProtocolPhase::WAIT_DEVICE_ID) {
    SUPLA_LOG_WARNING(
        "Haier hOn: device ID timeout, restarting initialization");
    protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
    next_initialization_attempt_ =
        std::chrono::steady_clock::now() + kInitializationRetryInterval;
    return HandlerError::HANDLER_OK;
  }

  if (request_type == FrameType::CONTROL &&
      protocol_phase_ == ProtocolPhase::WAIT_FIRST_STATUS) {
    SUPLA_LOG_WARNING(
        "Haier SmartAir2: first status timeout, restarting initialization");
    protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
    next_initialization_attempt_ =
        std::chrono::steady_clock::now() + kInitializationRetryInterval;
    return HandlerError::HANDLER_OK;
  }

  SUPLA_LOG_WARNING("Haier SmartAir2: response timeout for frame 0x%02X",
                    static_cast<unsigned>(request_type));
  return HandlerError::HANDLER_OK;
}

HandlerError HaierSmartair2Controller::handleNetworkStatusAnswer_(
    FrameType request_type,
    FrameType message_type,
    const uint8_t *data,
    size_t data_size) {
  if (request_type != FrameType::REPORT_NETWORK_STATUS) {
    return HandlerError::UNEXPECTED_MESSAGE;
  }
  if (message_type != FrameType::CONFIRM || data_size != 0) {
    return HandlerError::INVALID_ANSWER;
  }
  return HandlerError::HANDLER_OK;
}

void HaierSmartair2Controller::setPower(bool on) {
  power_state_ = on;
  if (hon_mode_) {
    queueControlUpdate_();
    return;
  }
  const uint16_t subcommand = on ? 0x4D02 : 0x4D03;
  HaierMessage msg(FrameType::CONTROL, subcommand);
  protocol_.send_message(msg, use_crc_);
}

float HaierSmartair2Controller::getRoomTemperatureC() const {
  return room_temperature_c_;
}

uint8_t HaierSmartair2Controller::getWifiSignalStrength_() const {
  const int32_t rssi = WiFi.RSSI();
  if (rssi >= 0) {
    return 0;
  }

  const int32_t magnitude = -rssi;
  if (magnitude > 0xFF) {
    return 0xFF;
  }
  return static_cast<uint8_t>(magnitude);
}

void HaierSmartair2Controller::setTargetTemperatureChangedCallback(
    std::function<void(float)> callback) {
  target_temperature_changed_callback_ = std::move(callback);
}

HandlerError HaierSmartair2Controller::handleStatusAnswer_(
    FrameType request_type,
    FrameType message_type,
    const uint8_t *data,
    size_t data_size) {
  if (hon_mode_ && request_type == FrameType::CONTROL &&
      message_type == FrameType::STATUS) {
    return handleHonStatus_(data, data_size);
  }

  if (request_type != FrameType::CONTROL ||
      message_type != FrameType::STATUS ||
      data == nullptr ||
      data_size < sizeof(HaierStatus)) {
    if (protocol_phase_ == ProtocolPhase::WAIT_FIRST_STATUS) {
      SUPLA_LOG_WARNING(
          "Haier SmartAir2: invalid first status response type=0x%02X "
          "size=%u",
          static_cast<unsigned>(message_type),
          static_cast<unsigned>(data_size));
      protocol_phase_ = ProtocolPhase::START_DEVICE_VERSION;
      next_initialization_attempt_ =
          std::chrono::steady_clock::now() + kInitializationRetryInterval;
    }
    return HandlerError::WRONG_MESSAGE_STRUCTURE;
  }

  if (protocol_phase_ != ProtocolPhase::READY) {
    SUPLA_LOG_INFO("Haier SmartAir2: first status received, protocol ready");
    protocol_phase_ = ProtocolPhase::READY;
  }

  HaierStatus status;
  memcpy(&status, data, sizeof(HaierStatus));

  room_temperature_c_ = static_cast<float>(status.control.room_temperature);
  if (room_temperature_c_ > 80.0f) {
    room_temperature_c_ /= 10.0f;
  }
  power_state_ = (status.control.ac_power != 0);
  compressor_ = (status.control.compressor != 0);
  room_humidity_ = status.control.room_humidity;
  if (!pending_settings_.valid) {
    const float previous_set_point = set_point_;
    ac_mode_ = static_cast<ConditioningMode>(status.control.ac_mode);
    set_point_ = decodeSetPointC(status.control.set_point);
    fan_mode_ = static_cast<FanMode>(status.control.fan_mode);
    use_swing_bits_ = (status.control.use_swing_bits != 0);
    horizontal_swing_ = (status.control.horizontal_swing != 0);
    vertical_swing_ = (status.control.vertical_swing != 0);
    turbo_mode_ = (status.control.turbo_mode != 0);
    quiet_mode_ = (status.control.quiet_mode != 0);
    display_status_ = (status.control.display_status != 0);
    use_fahrenheit_ = (status.control.use_fahrenheit != 0);
    lock_remote_ = (status.control.lock_remote != 0);
    health_mode_ = (status.control.health_mode != 0);
    ten_degree_ = (status.control.ten_degree != 0);
    if ((std::isnan(previous_set_point) && std::isfinite(set_point_)) ||
        (std::isfinite(previous_set_point) && std::isfinite(set_point_) &&
         std::fabs(previous_set_point - set_point_) > 0.25f)) {
      notifyTargetTemperatureChanged_();
    }
  }
  static_assert(sizeof(last_status_message_) == sizeof(status.control), "size mismatch");
  memcpy(last_status_message_.data(), &status.control, sizeof(status.control));
  has_last_status_message_ = true;

  logPacketDiff(data, data_size);

  if (pending_settings_.valid) {
    bool applied = true;
    if (status.control.ac_mode != static_cast<uint8_t>(ac_mode_)) applied = false;
    if (status.control.fan_mode != static_cast<uint8_t>(fan_mode_)) applied = false;
    if (((status.control.turbo_mode != 0) != turbo_mode_)) applied = false;
    if (((status.control.quiet_mode != 0) != quiet_mode_)) applied = false;
    if (((status.control.display_status != 0) != display_status_)) applied = false;
    if (((status.control.health_mode != 0) != health_mode_)) applied = false;
    if (((status.control.ten_degree != 0) != ten_degree_)) applied = false;
    if (((status.control.use_swing_bits != 0) != use_swing_bits_)) applied = false;
    if (((status.control.horizontal_swing != 0) != horizontal_swing_)) applied = false;
    if (((status.control.vertical_swing != 0) != vertical_swing_)) applied = false;

    float reported_set = decodeSetPointC(status.control.set_point);
    if (!std::isnan(set_point_)) {
      if (std::fabs(reported_set - set_point_) > 0.25f) applied = false;
    }

    if (applied) {
      pending_settings_.valid = false;
      control_send_requested_ = false;
    } else {
      control_send_requested_ = false;
    }
  }

  return HandlerError::HANDLER_OK;
}

HandlerError HaierSmartair2Controller::handleHonStatus_(const uint8_t *data,
                                                        size_t data_size) {
  using HonControl = Supla::haier::hon_protocol::HaierPacketControl;
  using HonSensors = Supla::haier::hon_protocol::HaierPacketSensors;
  constexpr size_t kMinimumStatusSize = 2 + sizeof(HonControl) + 3;
  if (data == nullptr || data_size < kMinimumStatusSize) {
    SUPLA_LOG_WARNING("Haier hOn: invalid status size=%u",
                      static_cast<unsigned>(data_size));
    return HandlerError::WRONG_MESSAGE_STRUCTURE;
  }

  const uint16_t subcommand =
      (static_cast<uint16_t>(data[0]) << 8) | data[1];
  const auto *control = reinterpret_cast<const HonControl *>(data + 2);
  const uint8_t *sensors_raw = data + 2 + sizeof(HonControl);
  const bool first_status = !has_last_status_message_;

  const float reported_room_temperature = sensors_raw[0] / 2.0f;
  const uint8_t reported_humidity = sensors_raw[1];
  const float reported_set_point =
      control->set_point + 16.0f + (control->half_degree ? 0.5f : 0.0f);
  const ConditioningMode reported_mode = fromHonMode(control->ac_mode);
  const FanMode reported_fan = fromHonFan(control->fan_mode);
  const bool reported_horizontal_swing =
      control->horizontal_swing_mode == static_cast<uint8_t>(
          Supla::haier::hon_protocol::HorizontalSwingMode::AUTO);
  const bool reported_vertical_swing =
      control->vertical_swing_mode == static_cast<uint8_t>(
          Supla::haier::hon_protocol::VerticalSwingMode::AUTO) ||
      control->vertical_swing_mode == static_cast<uint8_t>(
          Supla::haier::hon_protocol::VerticalSwingMode::AUTO_SPECIAL);

  room_temperature_c_ = reported_room_temperature;
  room_humidity_ = reported_humidity <= 100 ? reported_humidity : 0;
  compressor_ = false;
  memcpy(last_hon_control_.data(), control, sizeof(HonControl));
  has_last_status_message_ = true;
  logPacketDiff(data, data_size);

  if (first_status) {
    // SUPLA channels can briefly expose their defaults before the first HVAC
    // frame arrives. Never allow those startup values to overwrite the AC.
    pending_settings_.valid = false;
    control_send_requested_ = false;
  }

  const bool settings_applied =
      (power_state_ == (control->ac_power != 0)) &&
      (ac_mode_ == reported_mode) &&
      (fan_mode_ == reported_fan) &&
      (std::isnan(set_point_) ||
       std::fabs(set_point_ - reported_set_point) <= 0.25f) &&
      (horizontal_swing_ == reported_horizontal_swing) &&
      (vertical_swing_ == reported_vertical_swing) &&
      (turbo_mode_ == (control->fast_mode != 0)) &&
      (quiet_mode_ == (control->quiet_mode != 0)) &&
      (display_status_ == (control->display_status != 0)) &&
      (use_fahrenheit_ == (control->use_fahrenheit != 0)) &&
      (lock_remote_ == (control->lock_remote != 0)) &&
      (health_mode_ == (control->health_mode != 0)) &&
      (ten_degree_ == (control->ten_degree != 0));

  if (first_status || !pending_settings_.valid || settings_applied) {
    const float previous_set_point = set_point_;
    power_state_ = control->ac_power != 0;
    ac_mode_ = reported_mode;
    fan_mode_ = reported_fan;
    set_point_ = reported_set_point;
    horizontal_swing_ = reported_horizontal_swing;
    vertical_swing_ = reported_vertical_swing;
    use_swing_bits_ = horizontal_swing_ || vertical_swing_;
    turbo_mode_ = control->fast_mode != 0;
    quiet_mode_ = control->quiet_mode != 0;
    display_status_ = control->display_status != 0;
    use_fahrenheit_ = control->use_fahrenheit != 0;
    lock_remote_ = control->lock_remote != 0;
    health_mode_ = control->health_mode != 0;
    ten_degree_ = control->ten_degree != 0;
    hon_energy_saving_ = control->energy_saving_status != 0;
    if ((std::isnan(previous_set_point) && std::isfinite(set_point_)) ||
        (std::isfinite(previous_set_point) &&
         std::fabs(previous_set_point - set_point_) > 0.25f)) {
      notifyTargetTemperatureChanged_();
    }
  }

  if (pending_settings_.valid) {
    if (settings_applied) {
      pending_settings_.valid = false;
    }
    control_send_requested_ = false;
  }

  if (protocol_phase_ != ProtocolPhase::READY) {
    SUPLA_LOG_INFO(
        "Haier hOn ready: subtype=0x%04X room=%.1fC set=%.1fC power=%u "
        "mode=%u fan=%u",
        static_cast<unsigned>(subcommand), room_temperature_c_, set_point_,
        power_state_ ? 1U : 0U, static_cast<unsigned>(ac_mode_),
        static_cast<unsigned>(fan_mode_));
    protocol_phase_ = ProtocolPhase::READY;
  }

  if (data_size >= 2 + sizeof(HonControl) + sizeof(HonSensors)) {
    const auto *sensors =
        reinterpret_cast<const HonSensors *>(data + 2 + sizeof(HonControl));
    if (sensors->error_status != 0) {
      SUPLA_LOG_WARNING("Haier hOn: HVAC error code=0x%02X",
                        static_cast<unsigned>(sensors->error_status));
    }
  }
  return HandlerError::HANDLER_OK;
}

void HaierSmartair2Controller::queueControlUpdate_() {
  pending_settings_.valid = true;
  control_send_requested_ = false;
}

void HaierSmartair2Controller::notifyTargetTemperatureChanged_() {
  if (target_temperature_changed_callback_ && std::isfinite(set_point_)) {
    target_temperature_changed_callback_(set_point_);
  }
}

void HaierSmartair2Controller::triggerPowerSavingModeToggleSequence() {
  if (hon_mode_) {
    hon_energy_saving_ = !hon_energy_saving_;
    queueControlUpdate_();
    power_saving_mode_toggle_sequence_.active = false;
    return;
  }
  power_saving_mode_toggle_sequence_.active = true;
  power_saving_mode_toggle_sequence_.auto_mode_sent = false;
  power_saving_mode_toggle_sequence_.next_health_mode = !health_mode_;
  power_saving_mode_toggle_sequence_.toggles_remaining = 6;
  power_saving_mode_toggle_sequence_.next_step_at = std::chrono::steady_clock::now();
}

bool HaierSmartair2Controller::isPowerSavingModeToggleSequenceActive() const {
  return power_saving_mode_toggle_sequence_.active;
}

void HaierSmartair2Controller::triggerDisplayTemperatureToggleSequence() {
  if (hon_mode_) {
    SUPLA_LOG_WARNING(
        "Haier hOn: display-temperature toggle is not supported");
    display_temperature_toggle_sequence_.active = false;
    return;
  }
  display_temperature_toggle_sequence_.active = true;
  display_temperature_toggle_sequence_.next_display_status = !display_status_;
  display_temperature_toggle_sequence_.toggles_remaining = 10;
  display_temperature_toggle_sequence_.next_step_at = std::chrono::steady_clock::now();
}

bool HaierSmartair2Controller::isDisplayTemperatureToggleSequenceActive() const {
  return display_temperature_toggle_sequence_.active;
}

bool HaierSmartair2Controller::isAnyToggleSequenceActive_() const {
  return power_saving_mode_toggle_sequence_.active ||
         display_temperature_toggle_sequence_.active;
}

void HaierSmartair2Controller::sendDisplayToggleStep_(bool on) {
  if (!has_last_status_message_) {
    return;
  }

  setDisplayStatus(on);
}

void HaierSmartair2Controller::processPowerSavingModeToggleSequence_() {
  auto &sequence = power_saving_mode_toggle_sequence_;
  if (!sequence.active) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (now < sequence.next_step_at) {
    return;
  }

  if (pending_settings_.valid || control_send_requested_ ||
      protocol_.is_waiting_for_answer() ||
      protocol_.get_outgoing_queue_size() != 0) {
    return;
  }

  if (!has_last_status_message_) {
    sendStatusRequest_();
    last_status_request_ = now;
    sequence.next_step_at = now + power_saving_status_retry_interval_;
    return;
  }

  if (!sequence.auto_mode_sent) {
    setACMode(ConditioningMode::AUTO);
    sequence.auto_mode_sent = true;
    sequence.next_step_at = now + power_saving_toggle_interval_;
    return;
  }

  if (sequence.toggles_remaining > 0) {
    setHealthMode(sequence.next_health_mode);
    sequence.next_health_mode = !sequence.next_health_mode;
    --sequence.toggles_remaining;
    sequence.next_step_at =
        (sequence.toggles_remaining == 0) ? now : now + power_saving_toggle_interval_;
    return;
  }

  sequence.active = false;
}

void HaierSmartair2Controller::processDisplayTemperatureToggleSequence_() {
  auto &sequence = display_temperature_toggle_sequence_;
  if (!sequence.active) {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  if (now < sequence.next_step_at) {
    return;
  }

  if (pending_settings_.valid || control_send_requested_ ||
      protocol_.is_waiting_for_answer() ||
      protocol_.get_outgoing_queue_size() != 0) {
    return;
  }

  if (!has_last_status_message_) {
    sendStatusRequest_();
    last_status_request_ = now;
    sequence.next_step_at = now + display_temperature_status_retry_interval_;
    return;
  }

  if (sequence.toggles_remaining > 0) {
    sendDisplayToggleStep_(sequence.next_display_status);
    sequence.next_display_status = !sequence.next_display_status;
    --sequence.toggles_remaining;
    sequence.next_step_at =
        (sequence.toggles_remaining == 0) ? now
                                          : now + display_temperature_toggle_interval_;
    return;
  }

  sequence.active = false;
}

void HaierSmartair2Controller::setTargetTemperatureC(float temp) {
  if (temp < kSetpointMinC) {
    temp = kSetpointMinC;
  } else if (temp > kSetpointMaxC) {
    temp = kSetpointMaxC;
  }
  temp = roundf(temp);
  const bool changed = std::isnan(set_point_) || std::fabs(set_point_ - temp) > 0.25f;
  set_point_ = temp;
  if (changed) {
    notifyTargetTemperatureChanged_();
  }
  queueControlUpdate_();
}

void HaierSmartair2Controller::increaseTargetTemperatureC(bool increase) {
  float currentTemp = getTargetTemperatureC();
  float newTemp = increase ? currentTemp + 1.0f : currentTemp - 1.0f;
  setTargetTemperatureC(newTemp);
}

void HaierSmartair2Controller::setACMode(Supla::haier::smartair2_protocol::ConditioningMode mode) {
  ac_mode_ = mode;
  if (mode == Supla::haier::smartair2_protocol::ConditioningMode::AUTO) {
    fan_mode_ = Supla::haier::smartair2_protocol::FanMode::FAN_AUTO;
  } else if (mode == Supla::haier::smartair2_protocol::ConditioningMode::FAN &&
             fan_mode_ == Supla::haier::smartair2_protocol::FanMode::FAN_AUTO) {
    fan_mode_ = Supla::haier::smartair2_protocol::FanMode::FAN_LOW;
  }
  queueControlUpdate_();
}

void HaierSmartair2Controller::setFanMode(Supla::haier::smartair2_protocol::FanMode mode) {
  if (ac_mode_ == Supla::haier::smartair2_protocol::ConditioningMode::FAN &&
      mode == Supla::haier::smartair2_protocol::FanMode::FAN_AUTO) {
    return;
  }
  fan_mode_ = mode;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setUseSwingBits(bool use_bits) {
  use_swing_bits_ = use_bits;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setHorizontalSwing(bool h) {
  horizontal_swing_ = h;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setVerticalSwing(bool v) {
  vertical_swing_ = v;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setTurbo(bool on) {
  turbo_mode_ = on;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setQuiet(bool on) {
  quiet_mode_ = on;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setDisplayStatus(bool on) {
  display_status_ = on;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setUseFahrenheit(bool on) {
  use_fahrenheit_ = on;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setLockRemote(bool on) {
  lock_remote_ = on;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setHealthMode(bool on) {
  health_mode_ = on;
  queueControlUpdate_();
}

void HaierSmartair2Controller::setTenDegree(bool on) {
  ten_degree_ = on;
  queueControlUpdate_();
}

haier_protocol::HaierMessage HaierSmartair2Controller::getControlMessage() {
  if (!has_last_status_message_) {
    sendStatusRequest_();
    return HaierMessage(FrameType::CONTROL, 0x4D5F);
  }

  if (hon_mode_) {
    using HonControl = Supla::haier::hon_protocol::HaierPacketControl;
    uint8_t control_out_buffer[sizeof(HonControl)];
    memcpy(control_out_buffer, last_hon_control_.data(), sizeof(control_out_buffer));
    auto *out_data = reinterpret_cast<HonControl *>(control_out_buffer);

    control_out_buffer[4] = 0;
    out_data->ac_power = power_state_ ? 1 : 0;
    out_data->ac_mode = toHonMode(ac_mode_);
    out_data->fan_mode = toHonFan(fan_mode_);
    if (ac_mode_ == ConditioningMode::FAN && fan_mode_ == FanMode::FAN_AUTO) {
      out_data->fan_mode = toHonFan(FanMode::FAN_LOW);
    }

    if (!std::isnan(set_point_)) {
      const float clamped = std::max(kSetpointMinC,
                                     std::min(kSetpointMaxC, set_point_));
      const int whole = static_cast<int>(clamped);
      out_data->set_point = static_cast<uint8_t>(whole - kSetpointOffsetC);
      out_data->half_degree = (clamped - whole) >= 0.49f ? 1 : 0;
    }

    if (horizontal_swing_) {
      out_data->horizontal_swing_mode = static_cast<uint8_t>(
          Supla::haier::hon_protocol::HorizontalSwingMode::AUTO);
    } else if (out_data->horizontal_swing_mode == static_cast<uint8_t>(
                   Supla::haier::hon_protocol::HorizontalSwingMode::AUTO)) {
      out_data->horizontal_swing_mode = static_cast<uint8_t>(
          Supla::haier::hon_protocol::HorizontalSwingMode::CENTER);
    }
    if (vertical_swing_) {
      out_data->vertical_swing_mode = static_cast<uint8_t>(
          Supla::haier::hon_protocol::VerticalSwingMode::AUTO);
    } else if (out_data->vertical_swing_mode == static_cast<uint8_t>(
                   Supla::haier::hon_protocol::VerticalSwingMode::AUTO) ||
               out_data->vertical_swing_mode == static_cast<uint8_t>(
                   Supla::haier::hon_protocol::VerticalSwingMode::AUTO_SPECIAL)) {
      out_data->vertical_swing_mode = static_cast<uint8_t>(
          Supla::haier::hon_protocol::VerticalSwingMode::CENTER);
    }

    out_data->fast_mode = turbo_mode_ ? 1 : 0;
    out_data->quiet_mode = quiet_mode_ ? 1 : 0;
    out_data->ten_degree = ten_degree_ ? 1 : 0;
    out_data->display_status = display_status_ ? 1 : 0;
    out_data->use_fahrenheit = use_fahrenheit_ ? 1 : 0;
    out_data->lock_remote = lock_remote_ ? 1 : 0;
    out_data->health_mode = health_mode_ ? 1 : 0;
    out_data->energy_saving_status = hon_energy_saving_ ? 1 : 0;

    return HaierMessage(
        FrameType::CONTROL,
        Supla::haier::hon_protocol::kSetGroupParameters,
        control_out_buffer, sizeof(control_out_buffer));
  }

  uint8_t control_out_buffer[sizeof(Supla::haier::smartair2_protocol::HaierPacketControl)];
  memcpy(control_out_buffer, last_status_message_.data(), sizeof(control_out_buffer));
  auto *out_data = reinterpret_cast<Supla::haier::smartair2_protocol::HaierPacketControl *>(control_out_buffer);

  out_data->cntrl = 0x00;
  out_data->ac_power = power_state_ ? 1 : 0;
  out_data->ac_mode = static_cast<uint8_t>(ac_mode_);
  out_data->fan_mode = static_cast<uint8_t>(fan_mode_);
  out_data->use_swing_bits = use_swing_bits_ ? 1 : 0;
  out_data->horizontal_swing = horizontal_swing_ ? 1 : 0;
  out_data->vertical_swing = vertical_swing_ ? 1 : 0;

  if (!std::isnan(set_point_)) {
    out_data->set_point = encodeSetPointC(set_point_);
  }

  out_data->turbo_mode = turbo_mode_ ? 1 : 0;
  out_data->quiet_mode = quiet_mode_ ? 1 : 0;
  out_data->ten_degree = ten_degree_ ? 1 : 0;
  out_data->display_status = display_status_ ? 1 : 0;
  out_data->use_fahrenheit = use_fahrenheit_ ? 1 : 0;
  out_data->lock_remote = lock_remote_ ? 1 : 0;
  out_data->health_mode = health_mode_ ? 1 : 0;

  return HaierMessage(FrameType::CONTROL, 0x4D5F, control_out_buffer, sizeof(control_out_buffer));
}

void HaierSmartair2Controller::sendControlNow() {
  if (!has_last_status_message_) {
    return;
  }

  haier_protocol::HaierMessage msg = getControlMessage();
  if (pending_settings_.valid) {
    protocol_.send_message(msg, use_crc_, 2, control_retry_interval_);
  } else {
    protocol_.send_message(msg, use_crc_);
  }
}

bool HaierSmartair2Controller::getModeCool() const {
  return ac_mode_ == ConditioningMode::COOL;
}

bool HaierSmartair2Controller::getModeHeat() const {
  return ac_mode_ == ConditioningMode::HEAT;
}

bool HaierSmartair2Controller::getModeAuto() const {
  return ac_mode_ == ConditioningMode::AUTO;
}

bool HaierSmartair2Controller::getModeFan() const {
  return ac_mode_ == ConditioningMode::FAN;
}

bool HaierSmartair2Controller::getModeDry() const {
  return ac_mode_ == ConditioningMode::DRY;
}

bool HaierSmartair2Controller::getFanHigh() const {
  return fan_mode_ == FanMode::FAN_HIGH;
}

bool HaierSmartair2Controller::getFanMid() const {
  return fan_mode_ == FanMode::FAN_MID;
}

bool HaierSmartair2Controller::getFanLow() const {
  return fan_mode_ == FanMode::FAN_LOW;
}

bool HaierSmartair2Controller::getFanAuto() const {
  return fan_mode_ == FanMode::FAN_AUTO;
}

float HaierSmartair2Controller::getTargetTemperatureC() const {
  return set_point_;
}

Supla::haier::smartair2_protocol::FanMode HaierSmartair2Controller::getFanMode() const {
  return fan_mode_;
}

void HaierSmartair2Controller::logPacketDiff(const uint8_t *pkt, size_t len) {
  if (pkt == nullptr || len == 0) return;

  bool different = false;
  if (last_raw_packet_.size() != len) {
    different = true;
  } else {
    for (size_t i = 0; i < len; ++i) {
      if (last_raw_packet_[i] != pkt[i]) {
        different = true;
        break;
      }
    }
  }
  if (!different) return;

  std::string full;
  full.reserve(len * 3 + 32);
  char tmp[8];
  for (size_t i = 0; i < len; ++i) {
    snprintf(tmp, sizeof(tmp), "%02X ", pkt[i]);
    full.append(tmp);
  }
  SUPLA_LOG_DEBUG("HAIER  PKT: %s", full.c_str());

  std::string diff;
  diff.reserve(len * 3 + 32);
  for (size_t i = 0; i < len; ++i) {
    bool same = (i < last_raw_packet_.size() && last_raw_packet_[i] == pkt[i]);
    if (same) {
      diff.append(".. ");
    } else {
      snprintf(tmp, sizeof(tmp), "%02X ", pkt[i]);
      diff.append(tmp);
    }
  }
  SUPLA_LOG_DEBUG("HAIER DIFF: %s", diff.c_str());

  last_raw_packet_.assign(pkt, pkt + len);
}

Supla::haier::smartair2_protocol::ConditioningMode HaierSmartair2Controller::getACMode() const {
  return ac_mode_;
}

uint8_t HaierSmartair2Controller::getRoomHumidity() const { return room_humidity_; }

bool HaierSmartair2Controller::getUseSwingBits() const { return use_swing_bits_; }

bool HaierSmartair2Controller::getHorizontalSwing() const { return horizontal_swing_; }

bool HaierSmartair2Controller::getVerticalSwing() const { return vertical_swing_; }

bool HaierSmartair2Controller::getTurbo() const { return turbo_mode_; }

bool HaierSmartair2Controller::getQuiet() const { return quiet_mode_; }

bool HaierSmartair2Controller::getDisplayStatus() const { return display_status_; }

bool HaierSmartair2Controller::getUseFahrenheit() const { return use_fahrenheit_; }

bool HaierSmartair2Controller::getLockRemote() const { return lock_remote_; }

bool HaierSmartair2Controller::getHealthMode() const { return health_mode_; }

bool HaierSmartair2Controller::getCompressor() const { return compressor_; }

bool HaierSmartair2Controller::getTenDegree() const { return ten_degree_; }
