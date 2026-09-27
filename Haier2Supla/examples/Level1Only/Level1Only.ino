// Library example for the Haier2Supla Arduino library.
//
// Scope:
// - demonstrates a fixed level 1 SUPLA integration for Haier SmartAir2 units,
// - creates only the essential HVAC, temperature/humidity, mode, fan, and
//   swing channels,
// - keeps the interface simple by defining hardware pins directly in the
//   sketch,
//
// This example is intended as a minimal starting point when only level 1
// channels are needed. 
// For a more complete integration with additional channels and features, see the ConfigurableLevels example.
// Full device code is in the Haier2Supla example.
// Note: This example is not intended for production use as-is, but it can be used as a base 
// for a custom implementation.


#include <Arduino.h>
#include <SuplaDevice.h>
#include <supla/network/esp_wifi.h>
#include <supla/device/status_led.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/status_led_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/control/button.h>
#include <supla/storage/eeprom.h>

#include <Haier2Supla.h>

constexpr uint8_t STATUS_LED_PIN = 15;
constexpr uint8_t CONFIG_BUTTON_PIN = 9;
constexpr uint8_t HAIER_UART_RX_PIN = 14;
constexpr uint8_t HAIER_UART_TX_PIN = 20;
constexpr uint32_t HAIER_UART_BAUD = 9600;
constexpr bool STATUS_LED_INVERTED = false;

Supla::Eeprom eeprom;
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;
Supla::Device::StatusLed *statusLed = nullptr;
Supla::Control::Button *buttonCfg = nullptr;

HardwareSerial haierSerial(1);

HaierSmartair2Controller haierController(
    haierSerial,
    HAIER_UART_RX_PIN,
    HAIER_UART_TX_PIN,
    HAIER_UART_BAUD,
    false);
HaierSuplaChannels haierSupla(&haierController);

static const HaierSuplaChannelDefinition kLevel1Channels[] = {
    {HaierSuplaChannelId::Hvac, "HVAC"},
    {HaierSuplaChannelId::TemperatureHumidity, "Room Temp & Humidity"},
    {HaierSuplaChannelId::ModeCoolRelay, "Mode Cool"},
    {HaierSuplaChannelId::ModeHeatRelay, "Mode Heat"},
    {HaierSuplaChannelId::ModeAutoRelay, "Mode Auto"},
    {HaierSuplaChannelId::FanAutoRelay, "Fan Auto"},
    {HaierSuplaChannelId::FanHighRelay, "Fan High"},
    {HaierSuplaChannelId::FanMidRelay, "Fan Mid"},
    {HaierSuplaChannelId::FanLowRelay, "Fan Low"},
    {HaierSuplaChannelId::SwingHorizontalRelay, "Swing Horizontal"},
    {HaierSuplaChannelId::SwingVerticalRelay, "Swing Vertical"},
};

static HaierSuplaChannelConfig buildHaierChannelConfig() {
  HaierSuplaChannelConfig config = makeHaierSuplaChannelConfig();

  // The same helper can assign any level number, but this simplified example
  // intentionally creates only level 1 channels.
  setHaierSuplaChannelLevel(
    config, 1, kLevel1Channels,
    sizeof(kLevel1Channels) / sizeof(kLevel1Channels[0]));

  return config;
}

static const HaierSuplaChannelConfig kHaierChannelConfig =
  buildHaierChannelConfig();

void setup() {
  Serial.begin(115200);
  delay(2000);

  Supla::Storage::Init();

  statusLed = new Supla::Device::StatusLed(STATUS_LED_PIN, STATUS_LED_INVERTED);
  buttonCfg = new Supla::Control::Button(CONFIG_BUTTON_PIN, true, true);
  buttonCfg->configureAsConfigButton(&SuplaDevice);

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;
  new Supla::Html::StatusLedParameters;

  SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);
  SuplaDevice.begin();

  haierSerial.begin(HAIER_UART_BAUD, SERIAL_8N1, HAIER_UART_RX_PIN,
                    HAIER_UART_TX_PIN);
  haierController.begin();
  haierSupla.begin(nullptr, 1, kHaierChannelConfig);
}

void loop() {
  haierController.loop();
  SuplaDevice.iterate();
  haierSupla.iterate();
}