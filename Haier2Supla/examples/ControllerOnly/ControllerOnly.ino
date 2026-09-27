// Library example for the Haier2Supla Arduino library.
//
// Scope:
// - demonstrates the minimal Haier SmartAir2 controller setup over UART,
// - initializes only the protocol controller,
// - does not create SUPLA channels, web configuration etc.
//
// This example is intended for basic controller bring-up, protocol testing,
// and custom integrations that do not need the full SUPLA device stack.

#include <Arduino.h>
#include <SuplaDevice.h>
#include <supla/log_wrapper.h>
#include <HaierSmartair2Controller.h>

HardwareSerial haierSerial(1);
HaierSmartair2Controller controller(
    haierSerial,
    14,
    20,
    9600,
    false);

void setup() {
  Serial.begin(115200);
  haierSerial.begin(9600, SERIAL_8N1, 14, 20);
  controller.begin();
}

void loop() {
  controller.loop();
}
