#pragma once

#include <Arduino.h>
#include <supla/sensor/virtual_binary.h>
#include <supla/sensor/virtual_therm_hygro_meter.h>
#include <supla/sensor/virtual_thermometer.h>

#include "HaierAcVirtualRelay.h"
#include "HaierSmartair2Controller.h"

class HaierSuplaStateSync {
 public:
  explicit HaierSuplaStateSync(HaierSmartair2Controller *controller)
      : controller_(controller) {}

  void registerLevel1(
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *coolRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *heatRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *dryRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *fanRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *autoRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanHighRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanMidRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanLowRelay,
      Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanAutoRelay,
      Supla::Control::HaierVirtualRelay *swingHorizontalRelay,
      Supla::Control::HaierVirtualRelay *swingVerticalRelay,
      Supla::Sensor::VirtualThermHygroMeter *temperatureHumidity) {
    coolRelay_ = coolRelay;
    heatRelay_ = heatRelay;
    dryRelay_ = dryRelay;
    fanRelay_ = fanRelay;
    autoRelay_ = autoRelay;
    fanHighRelay_ = fanHighRelay;
    fanMidRelay_ = fanMidRelay;
    fanLowRelay_ = fanLowRelay;
    fanAutoRelay_ = fanAutoRelay;
    swingHorizontalRelay_ = swingHorizontalRelay;
    swingVerticalRelay_ = swingVerticalRelay;
    temperatureHumidity_ = temperatureHumidity;
  }

  void registerLevel2(Supla::Control::HaierVirtualRelay *healthRelay,
                      Supla::Control::HaierActionVirtualRelay *powerSavingRelay,
                      Supla::Control::HaierActionVirtualRelay *displayTemperatureRelay,
                      Supla::Control::HaierVirtualRelay *displayRelay,
                      Supla::Control::HaierVirtualRelay *quietRelay) {
    healthRelay_ = healthRelay;
    powerSavingRelay_ = powerSavingRelay;
    displayTemperatureRelay_ = displayTemperatureRelay;
    displayRelay_ = displayRelay;
    quietRelay_ = quietRelay;
  }

  void registerLevel3(
      Supla::Control::HaierVirtualRelay *turboRelay,
      Supla::Control::HaierVirtualRelay *tenDegreeRelay,
      Supla::Sensor::VirtualBinary *powerState,
      Supla::Sensor::VirtualThermometer *setTemperature,
      Supla::Sensor::VirtualBinary *modeCool,
      Supla::Sensor::VirtualBinary *modeHeat,
      Supla::Sensor::VirtualBinary *modeDry,
      Supla::Sensor::VirtualBinary *modeAuto,
      Supla::Sensor::VirtualBinary *modeFan,
      Supla::Sensor::VirtualBinary *fanHigh,
      Supla::Sensor::VirtualBinary *fanMid,
      Supla::Sensor::VirtualBinary *fanLow,
      Supla::Sensor::VirtualBinary *fanAuto,
      Supla::Sensor::VirtualBinary *turbo,
      Supla::Sensor::VirtualBinary *quiet,
      Supla::Sensor::VirtualBinary *display,
      Supla::Sensor::VirtualBinary *lockRemote,
      Supla::Sensor::VirtualBinary *health,
      Supla::Sensor::VirtualBinary *compressor,
      Supla::Sensor::VirtualBinary *tenDegree,
      Supla::Sensor::VirtualBinary *useSwingBits,
      Supla::Sensor::VirtualBinary *horizontalSwing,
      Supla::Sensor::VirtualBinary *verticalSwing) {
    turboRelay_ = turboRelay;
    tenDegreeRelay_ = tenDegreeRelay;
    powerState_ = powerState;
    setTemperature_ = setTemperature;
    modeCool_ = modeCool;
    modeHeat_ = modeHeat;
    modeDry_ = modeDry;
    modeAuto_ = modeAuto;
    modeFan_ = modeFan;
    fanHigh_ = fanHigh;
    fanMid_ = fanMid;
    fanLow_ = fanLow;
    fanAuto_ = fanAuto;
    turbo_ = turbo;
    quiet_ = quiet;
    display_ = display;
    lockRemote_ = lockRemote;
    health_ = health;
    compressor_ = compressor;
    tenDegree_ = tenDegree;
    useSwingBits_ = useSwingBits;
    horizontalSwing_ = horizontalSwing;
    verticalSwing_ = verticalSwing;
  }

  void iterate() {
    if (controller_ == nullptr) {
      return;
    }

    if (coolRelay_ != nullptr) {
      coolRelay_->syncState();
    }
    if (heatRelay_ != nullptr) {
      heatRelay_->syncState();
    }
    if (dryRelay_ != nullptr) {
      dryRelay_->syncState();
    }
    if (fanRelay_ != nullptr) {
      fanRelay_->syncState();
    }
    if (autoRelay_ != nullptr) {
      autoRelay_->syncState();
    }
    if (fanHighRelay_ != nullptr) {
      fanHighRelay_->syncState();
    }
    if (fanMidRelay_ != nullptr) {
      fanMidRelay_->syncState();
    }
    if (fanLowRelay_ != nullptr) {
      fanLowRelay_->syncState();
    }
    if (fanAutoRelay_ != nullptr) {
      fanAutoRelay_->syncState();
    }
    if (displayRelay_ != nullptr) {
      displayRelay_->syncState();
    }
    if (quietRelay_ != nullptr) {
      quietRelay_->syncState();
    }
    if (swingHorizontalRelay_ != nullptr) {
      swingHorizontalRelay_->syncState();
    }
    if (swingVerticalRelay_ != nullptr) {
      swingVerticalRelay_->syncState();
    }
    if (healthRelay_ != nullptr) {
      healthRelay_->syncState();
    }
    if (powerSavingRelay_ != nullptr) {
      powerSavingRelay_->syncState();
    }
    if (displayTemperatureRelay_ != nullptr) {
      displayTemperatureRelay_->syncState();
    }
    if (turboRelay_ != nullptr) {
      turboRelay_->syncState();
    }
    if (tenDegreeRelay_ != nullptr) {
      tenDegreeRelay_->syncState();
    }

    setBinaryState_(powerState_, controller_->getPower());

    if (temperatureHumidity_ != nullptr) {
      float roomTemp = controller_->getRoomTemperatureC();
      if (isfinite(roomTemp)) {
        temperatureHumidity_->setTemp(roomTemp);
      }
      temperatureHumidity_->setHumi(controller_->getRoomHumidity());
    }

    if (setTemperature_ != nullptr) {
      float targetTemp = controller_->getTargetTemperatureC();
      if (isfinite(targetTemp)) {
        setTemperature_->setValue(targetTemp);
      }
    }

    setBinaryState_(modeCool_, controller_->getModeCool());
    setBinaryState_(modeHeat_, controller_->getModeHeat());
    setBinaryState_(modeDry_, controller_->getModeDry());
    setBinaryState_(modeAuto_, controller_->getModeAuto());
    setBinaryState_(modeFan_, controller_->getModeFan());
    setBinaryState_(fanHigh_, controller_->getFanHigh());
    setBinaryState_(fanMid_, controller_->getFanMid());
    setBinaryState_(fanLow_, controller_->getFanLow());
    setBinaryState_(fanAuto_, controller_->getFanAuto());
    setBinaryState_(turbo_, controller_->getTurbo());
    setBinaryState_(quiet_, controller_->getQuiet());
    setBinaryState_(display_, controller_->getDisplayStatus());
    setBinaryState_(lockRemote_, controller_->getLockRemote());
    setBinaryState_(health_, controller_->getHealthMode());
    setBinaryState_(compressor_, controller_->getCompressor());
    setBinaryState_(tenDegree_, controller_->getTenDegree());
    setBinaryState_(useSwingBits_, controller_->getUseSwingBits());
    setBinaryState_(horizontalSwing_, controller_->getHorizontalSwing());
    setBinaryState_(verticalSwing_, controller_->getVerticalSwing());
  }

 private:
  static void setBinaryState_(Supla::Sensor::VirtualBinary *sensor, bool value) {
    if (sensor == nullptr) {
      return;
    }

    value ? sensor->set() : sensor->clear();
  }

  HaierSmartair2Controller *controller_ = nullptr;

  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *coolRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *heatRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *dryRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *fanRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::ConditioningMode> *autoRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanHighRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanMidRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanLowRelay_ = nullptr;
  Supla::Control::HaierVirtualRelayWithArgOn<Supla::haier::smartair2_protocol::FanMode> *fanAutoRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *swingHorizontalRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *swingVerticalRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *healthRelay_ = nullptr;
  Supla::Control::HaierActionVirtualRelay *powerSavingRelay_ = nullptr;
  Supla::Control::HaierActionVirtualRelay *displayTemperatureRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *displayRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *turboRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *tenDegreeRelay_ = nullptr;
  Supla::Control::HaierVirtualRelay *quietRelay_ = nullptr;

  Supla::Sensor::VirtualBinary *powerState_ = nullptr;
  Supla::Sensor::VirtualThermHygroMeter *temperatureHumidity_ = nullptr;
  Supla::Sensor::VirtualThermometer *setTemperature_ = nullptr;
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
  Supla::Sensor::VirtualBinary *lockRemote_ = nullptr;
  Supla::Sensor::VirtualBinary *health_ = nullptr;
  Supla::Sensor::VirtualBinary *compressor_ = nullptr;
  Supla::Sensor::VirtualBinary *tenDegree_ = nullptr;
  Supla::Sensor::VirtualBinary *useSwingBits_ = nullptr;
  Supla::Sensor::VirtualBinary *horizontalSwing_ = nullptr;
  Supla::Sensor::VirtualBinary *verticalSwing_ = nullptr;
};