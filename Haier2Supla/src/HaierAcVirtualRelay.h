#pragma once

#include <functional>
#include <supla/control/virtual_relay.h>

#include "HaierSmartair2Controller.h"

namespace Supla {
namespace Control {

class HaierAcVirtualRelay : public VirtualRelay {
 public:
  explicit HaierAcVirtualRelay(HaierSmartair2Controller *controller,
                               _supla_int_t functions =
                                   (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER));

  void turnOn(_supla_int_t duration = 0) override;
  void turnOff(_supla_int_t duration = 0) override;
  bool isOn() override;
  void syncState();

 private:
  HaierSmartair2Controller *controller_;
};

class HaierAcCoolVirtualRelay : public VirtualRelay {
 public:
  explicit HaierAcCoolVirtualRelay(HaierSmartair2Controller *controller,
                                   _supla_int_t functions =
                                       (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER));

  void turnOn(_supla_int_t duration = 0) override;
  void turnOff(_supla_int_t duration = 0) override;
  bool isOn() override;
  void syncState();

 private:
  HaierSmartair2Controller *controller_;
};

template <typename T>
class HaierVirtualRelayWithArg : public VirtualRelay {
 public:
  HaierVirtualRelayWithArg(std::function<void(T)> setter,
                          std::function<T()> getter,
                          T onValue,
                          T offValue,
                          _supla_int_t functions = (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER))
      : VirtualRelay(functions), setter_(std::move(setter)), getter_(std::move(getter)),
        onValue_(onValue), offValue_(offValue) {}

  void turnOn(_supla_int_t duration = 0) override {
    VirtualRelay::turnOn(duration);
    if (setter_) setter_(onValue_);
  }

  void turnOff(_supla_int_t duration = 0) override {
    VirtualRelay::turnOff(duration);
    if (setter_) setter_(offValue_);
  }

  bool isOn() override {
    if (getter_) return getter_() == onValue_;
    return VirtualRelay::isOn();
  }

  void syncState() {
    if (!getter_) return;
    bool on = getter_() == onValue_;
    if (on && !VirtualRelay::isOn()) {
      VirtualRelay::turnOn(0);
    } else if (!on && VirtualRelay::isOn()) {
      VirtualRelay::turnOff(0);
    }
  }

 private:
  std::function<void(T)> setter_;
  std::function<T()> getter_;
  T onValue_;
  T offValue_;
};

template <typename T>
class HaierVirtualRelayWithArgOn : public VirtualRelay {
 public:
  HaierVirtualRelayWithArgOn(std::function<void(T)> setter,
                            std::function<T()> getter,
                            T onValue,
                            _supla_int_t functions = (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER))
      : VirtualRelay(functions), setter_(std::move(setter)), getter_(std::move(getter)),
        onValue_(onValue) {}

  void turnOn(_supla_int_t duration = 0) override {
    VirtualRelay::turnOn(duration);
    if (setter_) setter_(onValue_);
  }

  void turnOff(_supla_int_t duration = 0) override {
    VirtualRelay::turnOff(duration);
  }

  bool isOn() override {
    if (getter_) return getter_() == onValue_;
    return VirtualRelay::isOn();
  }

  void syncState() {
    if (!getter_) return;
    bool on = getter_() == onValue_;
    if (on && !VirtualRelay::isOn()) {
      VirtualRelay::turnOn(0);
    } else if (!on && VirtualRelay::isOn()) {
      VirtualRelay::turnOff(0);
    }
  }

 private:
  std::function<void(T)> setter_;
  std::function<T()> getter_;
  T onValue_;
};

class HaierVirtualRelay : public VirtualRelay {
 public:
  HaierVirtualRelay(std::function<void(bool)> setter,
                    std::function<bool()> getter,
                    _supla_int_t functions = (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER));

  void turnOn(_supla_int_t duration = 0) override;
  void turnOff(_supla_int_t duration = 0) override;
  bool isOn() override;
  void syncState();

 private:
  std::function<void(bool)> setter_;
  std::function<bool()> getter_;
};

class HaierActionVirtualRelay : public VirtualRelay {
 public:
  HaierActionVirtualRelay(std::function<void()> action,
                          std::function<bool()> getter = nullptr,
                          _supla_int_t functions =
                              (0xFF ^ SUPLA_BIT_FUNC_CONTROLLINGTHEROLLERSHUTTER))
      : VirtualRelay(functions), action_(std::move(action)), getter_(std::move(getter)) {}

  void turnOn(_supla_int_t duration = 0) override {
    VirtualRelay::turnOn(duration);
    if (action_) action_();
  }

  void turnOff(_supla_int_t duration = 0) override {
    VirtualRelay::turnOff(duration);
  }

  bool isOn() override {
    if (getter_) return getter_();
    return VirtualRelay::isOn();
  }

  void syncState() {
    const bool on = isOn();
    if (on && !VirtualRelay::isOn()) {
      VirtualRelay::turnOn(0);
    } else if (!on && VirtualRelay::isOn()) {
      VirtualRelay::turnOff(0);
    }
  }

 private:
  std::function<void()> action_;
  std::function<bool()> getter_;
};

}  // namespace Control
}  // namespace Supla
