#include "HaierAcVirtualRelay.h"

namespace Supla {
namespace Control {

HaierAcVirtualRelay::HaierAcVirtualRelay(HaierSmartair2Controller *controller,
                                         _supla_int_t functions)
    : VirtualRelay(functions), controller_(controller) {}

void HaierAcVirtualRelay::turnOn(_supla_int_t duration) {
  VirtualRelay::turnOn(duration);
  if (controller_) {
    controller_->setPower(true);
  }
}

void HaierAcVirtualRelay::turnOff(_supla_int_t duration) {
  VirtualRelay::turnOff(duration);
  if (controller_) {
    controller_->setPower(false);
  }
}

bool HaierAcVirtualRelay::isOn() {
  if (controller_) {
    return controller_->getPower();
  }
  return VirtualRelay::isOn();
}

void HaierAcVirtualRelay::syncState() {
  if (!controller_) return;
  bool p = controller_->getPower();
  if (p && !VirtualRelay::isOn()) {
    VirtualRelay::turnOn(0);
  } else if (!p && VirtualRelay::isOn()) {
    VirtualRelay::turnOff(0);
  }
}

HaierAcCoolVirtualRelay::HaierAcCoolVirtualRelay(HaierSmartair2Controller *controller,
                                                 _supla_int_t functions)
    : VirtualRelay(functions), controller_(controller) {}

void HaierAcCoolVirtualRelay::turnOn(_supla_int_t duration) {
  VirtualRelay::turnOn(duration);
  if (controller_) {
    controller_->setACMode(Supla::haier::smartair2_protocol::ConditioningMode::COOL);
  }
}

void HaierAcCoolVirtualRelay::turnOff(_supla_int_t duration) {
  VirtualRelay::turnOff(duration);
  if (controller_) {
    controller_->setACMode(Supla::haier::smartair2_protocol::ConditioningMode::AUTO);
  }
}

bool HaierAcCoolVirtualRelay::isOn() {
  if (controller_) {
    return controller_->getPower() && controller_->getModeCool();
  }
  return VirtualRelay::isOn();
}

void HaierAcCoolVirtualRelay::syncState() {
  if (!controller_) return;
  bool on = controller_->getPower() && controller_->getModeCool();
  if (on && !VirtualRelay::isOn()) {
    VirtualRelay::turnOn(0);
  } else if (!on && VirtualRelay::isOn()) {
    VirtualRelay::turnOff(0);
  }
}

}  // namespace Control
}  // namespace Supla

using namespace Supla::Control;

HaierVirtualRelay::HaierVirtualRelay(std::function<void(bool)> setter,
                                     std::function<bool()> getter,
                                     _supla_int_t functions)
    : VirtualRelay(functions), setter_(std::move(setter)), getter_(std::move(getter)) {}

void HaierVirtualRelay::turnOn(_supla_int_t duration) {
  VirtualRelay::turnOn(duration);
  if (setter_) setter_(true);
}

void HaierVirtualRelay::turnOff(_supla_int_t duration) {
  VirtualRelay::turnOff(duration);
  if (setter_) setter_(false);
}

bool HaierVirtualRelay::isOn() {
  if (getter_) return getter_();
  return VirtualRelay::isOn();
}

void HaierVirtualRelay::syncState() {
  if (!getter_) return;
  bool val = getter_();
  if (val && !VirtualRelay::isOn()) {
    VirtualRelay::turnOn(0);
  } else if (!val && VirtualRelay::isOn()) {
    VirtualRelay::turnOff(0);
  }
}
