#pragma once

#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <functional>

#include <supla/control/virtual_relay.h>
#include <supla/log_wrapper.h>
#include <supla/sensor/virtual_binary.h>
#include <supla/sensor/virtual_therm_hygro_meter.h>
#include <supla/sensor/virtual_thermometer.h>

#include "HaierAcHvacChannel.h"
#include "HaierAcVirtualRelay.h"
#include "HaierFanSpeedChannel.h"
#include "HaierSmartair2Controller.h"
#include "HaierSuplaStateSync.h"

enum class HaierSuplaChannelId {
  Hvac,
  TemperatureHumidity,
  ModeCoolRelay,
  ModeHeatRelay,
  ModeDryRelay,
  ModeFanRelay,
  ModeAutoRelay,
  FanAutoRelay,
  FanHighRelay,
  FanMidRelay,
  FanLowRelay,
  FanSpeedChannel,
  SwingHorizontalRelay,
  SwingVerticalRelay,
  HealthRelay,
  PowerSavingRelay,
  DisplayTemperatureRelay,
  QuietRelay,
  DisplayRelay,
  TurboRelay,
  TenDegreeRelay,
  PowerStateSensor,
  SetTemperatureSensor,
  ModeCoolSensor,
  ModeHeatSensor,
  ModeDrySensor,
  ModeAutoSensor,
  ModeFanSensor,
  FanHighSensor,
  FanMidSensor,
  FanLowSensor,
  FanAutoSensor,
  TurboSensor,
  QuietSensor,
  DisplaySensor,
  LockRemoteSensor,
  HealthSensor,
  CompressorSensor,
  TenDegreeSensor,
  UseSwingBitsSensor,
  HorizontalSwingSensor,
  VerticalSwingSensor,
  TargetTemperatureStepRelay,
};

struct HaierSuplaChannelDefinition {
  HaierSuplaChannelId id;
  const char *caption;
};

struct HaierSuplaChannelLevelDefinition {
  const HaierSuplaChannelDefinition *channels = nullptr;
  size_t count = 0;
};

struct HaierSuplaConfiguredLevel {
  uint8_t level = 0;
  HaierSuplaChannelLevelDefinition definition;
};

constexpr size_t HAIER_SUPLA_MAX_CONFIGURED_LEVELS = 8;

struct HaierSuplaChannelConfig {
  HaierSuplaConfiguredLevel configuredLevels[HAIER_SUPLA_MAX_CONFIGURED_LEVELS];
  size_t configuredLevelCount = 0;
};

inline HaierSuplaChannelConfig makeHaierSuplaChannelConfig() {
  return {};
}

inline void setHaierSuplaChannelLevel(
    HaierSuplaChannelConfig &config,
    uint8_t level,
    const HaierSuplaChannelDefinition *channels,
    size_t count) {
  HaierSuplaChannelLevelDefinition definition;
  definition.channels = channels;
  definition.count = count;

  for (size_t index = 0; index < config.configuredLevelCount; ++index) {
    if (config.configuredLevels[index].level == level) {
      config.configuredLevels[index].definition = definition;
      return;
    }
  }

  if (config.configuredLevelCount < HAIER_SUPLA_MAX_CONFIGURED_LEVELS) {
    config.configuredLevels[config.configuredLevelCount].level = level;
    config.configuredLevels[config.configuredLevelCount].definition =
        definition;
    ++config.configuredLevelCount;
  }
}

class HaierSuplaChannels {
 public:
  explicit HaierSuplaChannels(HaierSmartair2Controller *controller)
      : controller_(controller), state_sync_(controller) {}

  void begin(const char *roomName,
             int interfaceLevel,
             const HaierSuplaChannelConfig &config) {
    if (initialized_ || controller_ == nullptr) {
      return;
    }

    interface_level_ = interfaceLevel;
    copyRoomName_(roomName);

    controller_->setTargetTemperatureChangedCallback(
        std::bind(&HaierSuplaChannels::onTargetTemperatureChanged_, this,
                  std::placeholders::_1));

    createConfiguredLevels_(config);

    finalizeConfiguration_();
    initialized_ = true;
  }

  void begin(const char *roomName,
             int interfaceLevel,
             bool enableDisplayTemperatureToggle = false) {
    begin(roomName, interfaceLevel,
          defaultConfig_(enableDisplayTemperatureToggle));
  }

  void iterate() {
    state_sync_.iterate();
  }

  HaierAcHvacChannel *hvacChannel() const {
    return hvac_;
  }

 private:
  static const HaierSuplaChannelConfig &defaultConfig_(
      bool enableDisplayTemperatureToggle) {
    static const HaierSuplaChannelDefinition kDefaultLevel1[] = {
        {HaierSuplaChannelId::Hvac, nullptr},
        {HaierSuplaChannelId::TemperatureHumidity, nullptr},
        {HaierSuplaChannelId::ModeCoolRelay, nullptr},
        {HaierSuplaChannelId::ModeHeatRelay, nullptr},
        {HaierSuplaChannelId::ModeAutoRelay, nullptr},
        {HaierSuplaChannelId::ModeDryRelay, nullptr},
        {HaierSuplaChannelId::ModeFanRelay, nullptr},
        {HaierSuplaChannelId::FanAutoRelay, nullptr},
        {HaierSuplaChannelId::FanHighRelay, nullptr},
        {HaierSuplaChannelId::FanMidRelay, nullptr},
        {HaierSuplaChannelId::FanLowRelay, nullptr},
        {HaierSuplaChannelId::SwingHorizontalRelay, nullptr},
        {HaierSuplaChannelId::SwingVerticalRelay, nullptr},
    };
    static const HaierSuplaChannelDefinition kDefaultLevel2[] = {
        {HaierSuplaChannelId::HealthRelay, nullptr},
        {HaierSuplaChannelId::PowerSavingRelay, nullptr},
        {HaierSuplaChannelId::QuietRelay, nullptr},
        {HaierSuplaChannelId::DisplayRelay, nullptr},
    };
    static const HaierSuplaChannelDefinition kDefaultLevel2WithDisplayToggle[] = {
        {HaierSuplaChannelId::HealthRelay, nullptr},
        {HaierSuplaChannelId::PowerSavingRelay, nullptr},
        {HaierSuplaChannelId::DisplayTemperatureRelay, nullptr},
        {HaierSuplaChannelId::QuietRelay, nullptr},
        {HaierSuplaChannelId::DisplayRelay, nullptr},
    };
    static const HaierSuplaChannelDefinition kDefaultLevel3[] = {
      {HaierSuplaChannelId::TurboRelay, nullptr},
        {HaierSuplaChannelId::PowerStateSensor, nullptr},
        {HaierSuplaChannelId::ModeCoolSensor, nullptr},
        {HaierSuplaChannelId::ModeHeatSensor, nullptr},
        {HaierSuplaChannelId::ModeDrySensor, nullptr},
        {HaierSuplaChannelId::ModeAutoSensor, nullptr},
        {HaierSuplaChannelId::ModeFanSensor, nullptr},
        {HaierSuplaChannelId::FanHighSensor, nullptr},
        {HaierSuplaChannelId::FanMidSensor, nullptr},
        {HaierSuplaChannelId::FanLowSensor, nullptr},
        {HaierSuplaChannelId::FanAutoSensor, nullptr},
        {HaierSuplaChannelId::TurboSensor, nullptr},
        {HaierSuplaChannelId::QuietSensor, nullptr},
        {HaierSuplaChannelId::DisplaySensor, nullptr},
        {HaierSuplaChannelId::LockRemoteSensor, nullptr},
        {HaierSuplaChannelId::HealthSensor, nullptr},
        {HaierSuplaChannelId::CompressorSensor, nullptr},
        {HaierSuplaChannelId::TenDegreeRelay, nullptr},
        {HaierSuplaChannelId::TenDegreeSensor, nullptr},
        {HaierSuplaChannelId::UseSwingBitsSensor, nullptr},
        {HaierSuplaChannelId::HorizontalSwingSensor, nullptr},
        {HaierSuplaChannelId::VerticalSwingSensor, nullptr},
        {HaierSuplaChannelId::SetTemperatureSensor, nullptr},
        {HaierSuplaChannelId::TargetTemperatureStepRelay, nullptr},
    };
    static const HaierSuplaChannelConfig kDefaultConfig = []() {
      HaierSuplaChannelConfig config = makeHaierSuplaChannelConfig();
      setHaierSuplaChannelLevel(
        config, 1, kDefaultLevel1,
        sizeof(kDefaultLevel1) / sizeof(kDefaultLevel1[0]));
      setHaierSuplaChannelLevel(
        config, 2, kDefaultLevel2,
        sizeof(kDefaultLevel2) / sizeof(kDefaultLevel2[0]));
      setHaierSuplaChannelLevel(
        config, 3, kDefaultLevel3,
        sizeof(kDefaultLevel3) / sizeof(kDefaultLevel3[0]));
      return config;
    }();
    static const HaierSuplaChannelConfig kDefaultConfigWithDisplayToggle = []() {
      HaierSuplaChannelConfig config = makeHaierSuplaChannelConfig();
      setHaierSuplaChannelLevel(
        config, 1, kDefaultLevel1,
        sizeof(kDefaultLevel1) / sizeof(kDefaultLevel1[0]));
      setHaierSuplaChannelLevel(
        config, 2, kDefaultLevel2WithDisplayToggle,
        sizeof(kDefaultLevel2WithDisplayToggle) /
          sizeof(kDefaultLevel2WithDisplayToggle[0]));
      setHaierSuplaChannelLevel(
        config, 3, kDefaultLevel3,
        sizeof(kDefaultLevel3) / sizeof(kDefaultLevel3[0]));
      return config;
    }();

    return enableDisplayTemperatureToggle ? kDefaultConfigWithDisplayToggle
                                          : kDefaultConfig;
  }

  void copyRoomName_(const char *roomName) {
    room_name_[0] = '\0';
    if (roomName != nullptr) {
      strncpy(room_name_, roomName, sizeof(room_name_) - 1);
      room_name_[sizeof(room_name_) - 1] = '\0';
    }
  }

  void createConfiguredLevel_(const HaierSuplaChannelLevelDefinition &level,
                              const char *label) {
    if (level.channels == nullptr || level.count == 0) {
      return;
    }

    SUPLA_LOG_DEBUG("Creating %s channels...", label);

    for (size_t index = 0; index < level.count; ++index) {
      createChannel_(level.channels[index]);
    }
  }

  void createConfiguredLevels_(const HaierSuplaChannelConfig &config) {
    for (uint8_t level = 1; level <= interface_level_; ++level) {
      const HaierSuplaChannelLevelDefinition *definition =
          findConfiguredLevel_(config, level);
      if (definition == nullptr) {
        continue;
      }

      char label[32] = {};
      snprintf(label, sizeof(label), "Level %u", level);
      createConfiguredLevel_(*definition, label);
    }
  }

  const HaierSuplaChannelLevelDefinition *findConfiguredLevel_(
      const HaierSuplaChannelConfig &config,
      uint8_t level) const {
    for (size_t index = 0; index < config.configuredLevelCount; ++index) {
      if (config.configuredLevels[index].level == level) {
        return &config.configuredLevels[index].definition;
      }
    }

    return nullptr;
  }

  template <typename T>
  void setCaption_(T *element, const char *caption, const char *defaultFormat) {
    if (element == nullptr) {
      return;
    }

    char resolvedCaption[100] = {};
    if (caption != nullptr && caption[0] != '\0') {
      if (room_name_[0] != '\0') {
        snprintf(resolvedCaption, sizeof(resolvedCaption), "%s-%s",
                 room_name_, caption);
      } else {
        snprintf(resolvedCaption, sizeof(resolvedCaption), "%s", caption);
      }
    } else if (defaultFormat != nullptr && defaultFormat[0] != '\0') {
      if (room_name_[0] == '\0' && strncmp(defaultFormat, "%s-", 3) == 0) {
        snprintf(resolvedCaption, sizeof(resolvedCaption), "%s",
                 defaultFormat + 3);
      } else {
        snprintf(resolvedCaption, sizeof(resolvedCaption), defaultFormat,
                 room_name_);
      }
    } else {
      return;
    }

    element->setInitialCaption(resolvedCaption);
  }

  void onTargetTemperatureChanged_(float targetTemp) {
    if (set_temperature_ != nullptr && isfinite(targetTemp)) {
      set_temperature_->setValue(targetTemp);
    }
  }

  void createChannel_(const HaierSuplaChannelDefinition &definition) {
    switch (definition.id) {
      case HaierSuplaChannelId::Hvac:
        if (hvac_ == nullptr) {
          hvac_ = new HaierAcHvacChannel(controller_);
          setCaption_(hvac_, definition.caption, "%s-Temp Setting");
        }
        break;

      case HaierSuplaChannelId::TemperatureHumidity:
        if (temperature_humidity_ == nullptr) {
          temperature_humidity_ = new Supla::Sensor::VirtualThermHygroMeter;
          setCaption_(temperature_humidity_, definition.caption,
                      "%s-Temp&Humi");
        }
        break;

      case HaierSuplaChannelId::ModeCoolRelay:
        if (cool_relay_ == nullptr) {
          cool_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::ConditioningMode>(
              std::bind(&HaierSmartair2Controller::setACMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getACMode, controller_),
              Supla::haier::smartair2_protocol::ConditioningMode::COOL);
          cool_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(cool_relay_, definition.caption, "%s-Mode:Cool");
        }
        break;

      case HaierSuplaChannelId::ModeHeatRelay:
        if (heat_relay_ == nullptr) {
          heat_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::ConditioningMode>(
              std::bind(&HaierSmartair2Controller::setACMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getACMode, controller_),
              Supla::haier::smartair2_protocol::ConditioningMode::HEAT);
          heat_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(heat_relay_, definition.caption, "%s-Mode:Heat");
        }
        break;

      case HaierSuplaChannelId::ModeDryRelay:
        if (dry_relay_ == nullptr) {
          dry_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::ConditioningMode>(
              std::bind(&HaierSmartair2Controller::setACMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getACMode, controller_),
              Supla::haier::smartair2_protocol::ConditioningMode::DRY);
          dry_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(dry_relay_, definition.caption, "%s-Mode:Dry");
        }
        break;

      case HaierSuplaChannelId::ModeFanRelay:
        if (fan_relay_ == nullptr) {
          fan_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::ConditioningMode>(
              std::bind(&HaierSmartair2Controller::setACMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getACMode, controller_),
              Supla::haier::smartair2_protocol::ConditioningMode::FAN);
          fan_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(fan_relay_, definition.caption, "%s-Mode:Fan");
        }
        break;

      case HaierSuplaChannelId::ModeAutoRelay:
        if (auto_relay_ == nullptr) {
          auto_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::ConditioningMode>(
              std::bind(&HaierSmartair2Controller::setACMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getACMode, controller_),
              Supla::haier::smartair2_protocol::ConditioningMode::AUTO);
          auto_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(auto_relay_, definition.caption, "%s-Mode:Auto");
        }
        break;

      case HaierSuplaChannelId::FanAutoRelay:
        if (fan_auto_relay_ == nullptr) {
          fan_auto_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::FanMode>(
              std::bind(&HaierSmartair2Controller::setFanMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getFanMode, controller_),
              Supla::haier::smartair2_protocol::FanMode::FAN_AUTO);
          fan_auto_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(fan_auto_relay_, definition.caption, "%s-Fan:Auto");
        }
        break;

      case HaierSuplaChannelId::FanHighRelay:
        if (fan_high_relay_ == nullptr) {
          fan_high_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::FanMode>(
              std::bind(&HaierSmartair2Controller::setFanMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getFanMode, controller_),
              Supla::haier::smartair2_protocol::FanMode::FAN_HIGH);
          fan_high_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(fan_high_relay_, definition.caption, "%s-Fan:High");
        }
        break;

      case HaierSuplaChannelId::FanMidRelay:
        if (fan_mid_relay_ == nullptr) {
          fan_mid_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::FanMode>(
              std::bind(&HaierSmartair2Controller::setFanMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getFanMode, controller_),
              Supla::haier::smartair2_protocol::FanMode::FAN_MID);
          fan_mid_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(fan_mid_relay_, definition.caption, "%s-Fan:Mid");
        }
        break;

      case HaierSuplaChannelId::FanLowRelay:
        if (fan_low_relay_ == nullptr) {
          fan_low_relay_ = new Supla::Control::HaierVirtualRelayWithArgOn<
              Supla::haier::smartair2_protocol::FanMode>(
              std::bind(&HaierSmartair2Controller::setFanMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getFanMode, controller_),
              Supla::haier::smartair2_protocol::FanMode::FAN_LOW);
          fan_low_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(fan_low_relay_, definition.caption, "%s-Fan:Low");
        }
        break;

      case HaierSuplaChannelId::FanSpeedChannel:
        if (fan_speed_channel_ == nullptr) {
          fan_speed_channel_ =
              new Supla::Control::HaierFanSpeedChannel(controller_);
          setCaption_(fan_speed_channel_, definition.caption,
                      "%s-Fan Speed");
        }
        break;

      case HaierSuplaChannelId::SwingHorizontalRelay:
        if (swing_horizontal_relay_ == nullptr) {
          swing_horizontal_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setHorizontalSwing,
                        controller_, std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getHorizontalSwing,
                        controller_));
          swing_horizontal_relay_->setDefaultFunction(
              SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(swing_horizontal_relay_, definition.caption,
                      "%s-Horizontal Swing");
        }
        break;

      case HaierSuplaChannelId::SwingVerticalRelay:
        if (swing_vertical_relay_ == nullptr) {
          swing_vertical_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setVerticalSwing,
                        controller_, std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getVerticalSwing,
                        controller_));
          swing_vertical_relay_->setDefaultFunction(
              SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(swing_vertical_relay_, definition.caption,
                      "%s-Vertical Swing");
        }
        break;

      case HaierSuplaChannelId::HealthRelay:
        if (health_relay_ == nullptr) {
          health_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setHealthMode, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getHealthMode,
                        controller_));
          health_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(health_relay_, definition.caption, "%s-Health Mode");
        }
        break;

      case HaierSuplaChannelId::PowerSavingRelay:
        if (power_saving_relay_ == nullptr) {
          power_saving_relay_ = new Supla::Control::HaierActionVirtualRelay(
              std::bind(&HaierSmartair2Controller::triggerPowerSavingModeToggleSequence,
                        controller_),
              std::bind(&HaierSmartair2Controller::isPowerSavingModeToggleSequenceActive,
                        controller_));
          power_saving_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(power_saving_relay_, definition.caption,
                      "%s-Power Saving Toggle");
        }
        break;

      case HaierSuplaChannelId::DisplayTemperatureRelay:
        if (display_temperature_relay_ == nullptr) {
          display_temperature_relay_ =
              new Supla::Control::HaierActionVirtualRelay(
                  std::bind(&HaierSmartair2Controller::triggerDisplayTemperatureToggleSequence,
                            controller_),
                  std::bind(&HaierSmartair2Controller::isDisplayTemperatureToggleSequenceActive,
                            controller_));
          display_temperature_relay_->setDefaultFunction(
              SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(display_temperature_relay_, definition.caption,
                      "%s-Display Temp Toggle");
        }
        break;

      case HaierSuplaChannelId::QuietRelay:
        if (quiet_relay_ == nullptr) {
          quiet_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setQuiet, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getQuiet, controller_));
          quiet_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(quiet_relay_, definition.caption, "%s-Quiet Mode");
        }
        break;

      case HaierSuplaChannelId::DisplayRelay:
        if (display_relay_ == nullptr) {
          display_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setDisplayStatus,
                        controller_, std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getDisplayStatus,
                        controller_));
          display_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(display_relay_, definition.caption,
                      "%s-Disable Display");
        }
        break;

      case HaierSuplaChannelId::TurboRelay:
        if (turbo_relay_ == nullptr) {
          turbo_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setTurbo, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getTurbo, controller_));
          turbo_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(turbo_relay_, definition.caption, "%s-Turbo Mode");
        }
        break;

      case HaierSuplaChannelId::TenDegreeRelay:
        if (ten_degree_relay_ == nullptr) {
          ten_degree_relay_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::setTenDegree, controller_,
                        std::placeholders::_1),
              std::bind(&HaierSmartair2Controller::getTenDegree,
                        controller_));
          ten_degree_relay_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(ten_degree_relay_, definition.caption,
                      "%s-10C Mode");
        }
        break;

      case HaierSuplaChannelId::PowerStateSensor:
        if (power_state_ == nullptr) {
          power_state_ = new Supla::Sensor::VirtualBinary;
          power_state_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(power_state_, definition.caption, "%s-Power");
        }
        break;

      case HaierSuplaChannelId::SetTemperatureSensor:
        if (set_temperature_ == nullptr) {
          set_temperature_ = new Supla::Sensor::VirtualThermometer;
          setCaption_(set_temperature_, definition.caption, "%s-Set Temp");
        }
        break;

      case HaierSuplaChannelId::ModeCoolSensor:
        if (modeCool_ == nullptr) {
          modeCool_ = new Supla::Sensor::VirtualBinary;
          modeCool_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(modeCool_, definition.caption, "%s-Mode:Cool");
        }
        break;

      case HaierSuplaChannelId::ModeHeatSensor:
        if (modeHeat_ == nullptr) {
          modeHeat_ = new Supla::Sensor::VirtualBinary;
          modeHeat_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(modeHeat_, definition.caption, "%s-Mode:Heat");
        }
        break;

      case HaierSuplaChannelId::ModeDrySensor:
        if (modeDry_ == nullptr) {
          modeDry_ = new Supla::Sensor::VirtualBinary;
          modeDry_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(modeDry_, definition.caption, "%s-Mode:Dry");
        }
        break;

      case HaierSuplaChannelId::ModeAutoSensor:
        if (modeAuto_ == nullptr) {
          modeAuto_ = new Supla::Sensor::VirtualBinary;
          modeAuto_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(modeAuto_, definition.caption, "%s-Mode:Auto");
        }
        break;

      case HaierSuplaChannelId::ModeFanSensor:
        if (modeFan_ == nullptr) {
          modeFan_ = new Supla::Sensor::VirtualBinary;
          modeFan_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(modeFan_, definition.caption, "%s-Mode:Fan");
        }
        break;

      case HaierSuplaChannelId::FanHighSensor:
        if (fanHigh_ == nullptr) {
          fanHigh_ = new Supla::Sensor::VirtualBinary;
          fanHigh_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(fanHigh_, definition.caption, "%s-Fan:High");
        }
        break;

      case HaierSuplaChannelId::FanMidSensor:
        if (fanMid_ == nullptr) {
          fanMid_ = new Supla::Sensor::VirtualBinary;
          fanMid_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(fanMid_, definition.caption, "%s-Fan:Mid");
        }
        break;

      case HaierSuplaChannelId::FanLowSensor:
        if (fanLow_ == nullptr) {
          fanLow_ = new Supla::Sensor::VirtualBinary;
          fanLow_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(fanLow_, definition.caption, "%s-Fan:Low");
        }
        break;

      case HaierSuplaChannelId::FanAutoSensor:
        if (fanAuto_ == nullptr) {
          fanAuto_ = new Supla::Sensor::VirtualBinary;
          fanAuto_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(fanAuto_, definition.caption, "%s-Fan:Auto");
        }
        break;

      case HaierSuplaChannelId::TurboSensor:
        if (turbo_ == nullptr) {
          turbo_ = new Supla::Sensor::VirtualBinary;
          turbo_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(turbo_, definition.caption, "%s-Turbo");
        }
        break;

      case HaierSuplaChannelId::QuietSensor:
        if (quiet_ == nullptr) {
          quiet_ = new Supla::Sensor::VirtualBinary;
          quiet_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(quiet_, definition.caption, "%s-Quiet");
        }
        break;

      case HaierSuplaChannelId::DisplaySensor:
        if (display_ == nullptr) {
          display_ = new Supla::Sensor::VirtualBinary;
          display_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(display_, definition.caption, "%s-Disable Display");
        }
        break;

      case HaierSuplaChannelId::LockRemoteSensor:
        if (lock_remote_ == nullptr) {
          lock_remote_ = new Supla::Sensor::VirtualBinary;
          lock_remote_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(lock_remote_, definition.caption, "%s-Lock Remote");
        }
        break;

      case HaierSuplaChannelId::HealthSensor:
        if (health_ == nullptr) {
          health_ = new Supla::Sensor::VirtualBinary;
          health_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(health_, definition.caption, "%s-Health Mode");
        }
        break;

      case HaierSuplaChannelId::CompressorSensor:
        if (compressor_ == nullptr) {
          compressor_ = new Supla::Sensor::VirtualBinary;
          compressor_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(compressor_, definition.caption, "%s-Compressor");
        }
        break;

      case HaierSuplaChannelId::TenDegreeSensor:
        if (ten_degree_ == nullptr) {
          ten_degree_ = new Supla::Sensor::VirtualBinary;
          ten_degree_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(ten_degree_, definition.caption, "%s-10°C Mode");
        }
        break;

      case HaierSuplaChannelId::UseSwingBitsSensor:
        if (use_swing_bits_ == nullptr) {
          use_swing_bits_ = new Supla::Sensor::VirtualBinary;
          use_swing_bits_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(use_swing_bits_, definition.caption,
                      "%s-Swing Enabled");
        }
        break;

      case HaierSuplaChannelId::HorizontalSwingSensor:
        if (horizontal_swing_ == nullptr) {
          horizontal_swing_ = new Supla::Sensor::VirtualBinary;
          horizontal_swing_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(horizontal_swing_, definition.caption,
                      "%s-Horizontal Swing");
        }
        break;

      case HaierSuplaChannelId::VerticalSwingSensor:
        if (vertical_swing_ == nullptr) {
          vertical_swing_ = new Supla::Sensor::VirtualBinary;
          vertical_swing_->setDefaultFunction(SUPLA_CHANNELFNC_BINARY_SENSOR);
          setCaption_(vertical_swing_, definition.caption,
                      "%s-Vertical Swing");
        }
        break;

      case HaierSuplaChannelId::TargetTemperatureStepRelay:
        if (temp_increase_ == nullptr) {
          temp_increase_ = new Supla::Control::HaierVirtualRelay(
              std::bind(&HaierSmartair2Controller::increaseTargetTemperatureC,
                        controller_, std::placeholders::_1),
              []() { return false; });
          temp_increase_->setDefaultFunction(SUPLA_CHANNELFNC_POWERSWITCH);
          setCaption_(temp_increase_, definition.caption,
                      " -1°C < %s Set Temp > +1°C");
        }
        break;
    }
  }

  void finalizeConfiguration_() {
    if (hvac_ != nullptr && temperature_humidity_ != nullptr &&
        temperature_humidity_->getChannel() != nullptr) {
      hvac_->setMainThermometerChannelNo(
          temperature_humidity_->getChannel()->getChannelNumber());
    }

    state_sync_.registerLevel1(cool_relay_, heat_relay_, dry_relay_, fan_relay_,
                               auto_relay_, fan_high_relay_, fan_mid_relay_,
                               fan_low_relay_, fan_auto_relay_,
                               swing_horizontal_relay_, swing_vertical_relay_,
                               temperature_humidity_);
    state_sync_.registerLevel2(health_relay_, power_saving_relay_,
                               display_temperature_relay_, display_relay_,
                               quiet_relay_);
    state_sync_.registerLevel3(turbo_relay_, ten_degree_relay_, power_state_,
                   set_temperature_, modeCool_, modeHeat_,
                   modeDry_, modeAuto_, modeFan_, fanHigh_,
                   fanMid_, fanLow_, fanAuto_, turbo_, quiet_,
                   display_, lock_remote_, health_, compressor_,
                   ten_degree_, use_swing_bits_,
                   horizontal_swing_, vertical_swing_);
  }

  HaierSmartair2Controller *controller_ = nullptr;
  HaierSuplaStateSync state_sync_;
  bool initialized_ = false;
  int interface_level_ = 1;
  char room_name_[32] = {};

  HaierAcHvacChannel *hvac_ = nullptr;
  Supla::Sensor::VirtualThermHygroMeter *temperature_humidity_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *cool_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *heat_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *dry_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *fan_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *auto_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fan_high_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fan_mid_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fan_low_relay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fan_auto_relay_ = nullptr;
  Supla::Control::HaierFanSpeedChannel *fan_speed_channel_ = nullptr;
  Supla::Control::HaierVirtualRelay *swing_horizontal_relay_ = nullptr;
  Supla::Control::HaierVirtualRelay *swing_vertical_relay_ = nullptr;
  Supla::Control::HaierVirtualRelay *health_relay_ = nullptr;
  Supla::Control::HaierActionVirtualRelay *power_saving_relay_ = nullptr;
  Supla::Control::HaierActionVirtualRelay *display_temperature_relay_ = nullptr;
  Supla::Control::HaierVirtualRelay *display_relay_ = nullptr;
  Supla::Control::HaierVirtualRelay *turbo_relay_ = nullptr;
  Supla::Control::HaierVirtualRelay *ten_degree_relay_ = nullptr;
  Supla::Control::HaierVirtualRelay *quiet_relay_ = nullptr;
  Supla::Sensor::VirtualBinary *power_state_ = nullptr;
  Supla::Sensor::VirtualThermometer *set_temperature_ = nullptr;
  Supla::Sensor::VirtualBinary *modeCool_ = nullptr;
  Supla::Sensor::VirtualBinary *modeHeat_ = nullptr;
  Supla::Sensor::VirtualBinary *modeDry_ = nullptr;
  Supla::Sensor::VirtualBinary *modeAuto_ = nullptr;
  Supla::Sensor::VirtualBinary *modeFan_ = nullptr;
  Supla::Sensor::VirtualBinary *fanHigh_ = nullptr;
  Supla::Sensor::VirtualBinary *fanMid_ = nullptr;
  Supla::Sensor::VirtualBinary *fanLow_ = nullptr;
  Supla::Sensor::VirtualBinary *fanAuto_ = nullptr;
  Supla::Sensor::VirtualBinary *turbo_ = nullptr;
  Supla::Sensor::VirtualBinary *quiet_ = nullptr;
  Supla::Sensor::VirtualBinary *display_ = nullptr;
  Supla::Sensor::VirtualBinary *lock_remote_ = nullptr;
  Supla::Sensor::VirtualBinary *health_ = nullptr;
  Supla::Sensor::VirtualBinary *compressor_ = nullptr;
  Supla::Sensor::VirtualBinary *ten_degree_ = nullptr;
  Supla::Sensor::VirtualBinary *use_swing_bits_ = nullptr;
  Supla::Sensor::VirtualBinary *horizontal_swing_ = nullptr;
  Supla::Sensor::VirtualBinary *vertical_swing_ = nullptr;
  Supla::Control::HaierVirtualRelay *temp_increase_ = nullptr;
};
