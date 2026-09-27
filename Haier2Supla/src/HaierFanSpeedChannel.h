#pragma once

#include <supla/channel_element.h>

#include "HaierSmartair2Controller.h"

namespace Supla {
namespace Control {

// A compact fan-speed control exposed as SUPLA's motor-speed slider.
// The AC supports four discrete values, so arbitrary slider positions are
// snapped to Auto, Low, Mid or High.
class HaierFanSpeedChannel : public Supla::ChannelElement {
 public:
  explicit HaierFanSpeedChannel(HaierSmartair2Controller *controller)
      : controller_(controller) {
    channel.setType(SUPLA_CHANNELTYPE_DIMMER);
    channel.setFuncList(0);
    // Function 600 (legacy motor-speed control) is defined in the wire
    // protocol, but current SUPLA Cloud rejects it for a DIMMER channel.
    // A standard dimmer provides the same 0-100 slider UI and is supported by
    // both Cloud and mobile clients.
    channel.setDefaultFunction(SUPLA_CHANNELFNC_DIMMER);
  }

  void onInit() override {
    syncState();
  }

  void iterateAlways() override {
    syncState();
  }

  int32_t handleNewValueFromServer(
      TSD_SuplaChannelNewValue *newValue) override {
    if (controller_ == nullptr || newValue == nullptr) {
      return 0;
    }

    const uint8_t requested =
        static_cast<uint8_t>(newValue->value[0]) > 100
            ? 100
            : static_cast<uint8_t>(newValue->value[0]);
    const FanMode mode = sliderToMode_(requested);
    controller_->setFanMode(mode);

    // Immediately snap the UI to the discrete value. The next AC status frame
    // confirms it and keeps the slider synchronized with remote-control use.
    publish_(mode);
    return -1;
  }

  void syncState() {
    if (controller_ == nullptr || !controller_->hasStatus()) {
      return;
    }
    publish_(controller_->getFanMode());
  }

 private:
  using FanMode = Supla::haier::smartair2_protocol::FanMode;

  static FanMode sliderToMode_(uint8_t value) {
    if (value <= 16) {
      return FanMode::FAN_AUTO;
    }
    if (value <= 49) {
      return FanMode::FAN_LOW;
    }
    if (value <= 83) {
      return FanMode::FAN_MID;
    }
    return FanMode::FAN_HIGH;
  }

  static uint8_t modeToSlider_(FanMode mode) {
    switch (mode) {
      case FanMode::FAN_LOW:
        return 33;
      case FanMode::FAN_MID:
        return 66;
      case FanMode::FAN_HIGH:
        return 100;
      case FanMode::FAN_AUTO:
      default:
        // Keep Auto distinguishable from an unavailable/off dimmer channel.
        return 1;
    }
  }

  void publish_(FanMode mode) {
    const uint8_t value = modeToSlider_(mode);
    if (channel.getValueBrightness() != value) {
      channel.setNewValue(0, 0, 0, 0, value, 0);
    }
  }

  HaierSmartair2Controller *controller_ = nullptr;
};

}  // namespace Control
}  // namespace Supla
