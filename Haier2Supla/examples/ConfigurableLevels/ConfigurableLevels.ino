// Library example for the Haier2Supla Arduino library.
//
// Scope:
// - demonstrates SUPLA integration for Haier SmartAir2 units with selectable
//   interface levels 1-3,
// - exposes a broader channel set than the minimal examples,
// - keeps hardware pins fixed in the sketch while allowing interface level
//   selection from the web configurator.
//
// This example is intended as a reference configuration between the minimal
// Level1Only sketch and the larger full Haier2Supla example. It is useful when the device
// pinout is fixed in code but the SUPLA channel scope should remain selectable.
// Note: Not all supported channels are included.

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
#include <supla/network/html/button_update.h>
#include <supla/network/html/select_input_parameter.h>
#include <supla/control/button.h>
#include <supla/storage/eeprom.h>
#include <supla/events.h>

#include <Haier2Supla.h>

constexpr uint8_t STATUS_LED_PIN = 15;
constexpr uint8_t CONFIG_BUTTON_PIN = 9;
constexpr uint8_t HAIER_UART_RX_PIN = 14;
constexpr uint8_t HAIER_UART_TX_PIN = 20;
constexpr bool STATUS_LED_INVERTED = false;

Supla::Eeprom eeprom;
Supla::ESPWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;
Supla::Device::StatusLed *statusLed = nullptr;
Supla::Control::Button *buttonCfg = nullptr;

#define CFG_TAG_INTERFACE_LEVEL "iface_level"

HardwareSerial haierSerial(1);

constexpr int32_t DEFAULT_INTERFACE_LEVEL = 1;
constexpr uint32_t HAIER_UART_BAUD = 9600;

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

static const HaierSuplaChannelDefinition kLevel2Channels[] = {
    {HaierSuplaChannelId::ModeDryRelay, "Mode Dry"},
    {HaierSuplaChannelId::ModeFanRelay, "Mode Fan"},
};

static const HaierSuplaChannelDefinition kLevel3Channels[] = {
    {HaierSuplaChannelId::PowerStateSensor, "Power State"},
    {HaierSuplaChannelId::ModeCoolSensor, "State Cool"},
    {HaierSuplaChannelId::ModeHeatSensor, "State Heat"},
    {HaierSuplaChannelId::ModeDrySensor, "State Dry"},
    {HaierSuplaChannelId::ModeAutoSensor, "State Auto"},
    {HaierSuplaChannelId::ModeFanSensor, "State Fan"},
};

static HaierSuplaChannelConfig buildHaierChannelConfig() {
  HaierSuplaChannelConfig config = makeHaierSuplaChannelConfig();

  // The helper supports additional levels as well, but this example exposes
  // only levels 1-3 through the web configuration selector below.
  setHaierSuplaChannelLevel(
    config, 1, kLevel1Channels,
    sizeof(kLevel1Channels) / sizeof(kLevel1Channels[0]));
  setHaierSuplaChannelLevel(
    config, 2, kLevel2Channels,
    sizeof(kLevel2Channels) / sizeof(kLevel2Channels[0]));
  setHaierSuplaChannelLevel(
    config, 3, kLevel3Channels,
    sizeof(kLevel3Channels) / sizeof(kLevel3Channels[0]));
  // Example for extra library-only levels:
  // setHaierSuplaChannelLevel(config, 4, kLevel4Channels,
  //                          sizeof(kLevel4Channels) / sizeof(kLevel4Channels[0]));

  return config;
}

static const HaierSuplaChannelConfig kHaierChannelConfig =
  buildHaierChannelConfig();

void setup() {
  Serial.begin(115200);
  delay(2000);

  Supla::Storage::Init();
  auto cfg = Supla::Storage::ConfigInstance();

  int32_t interfaceLvl = DEFAULT_INTERFACE_LEVEL;
  if (cfg) {
    if (!cfg->getInt32(CFG_TAG_INTERFACE_LEVEL, &interfaceLvl)) {
      cfg->setInt32(CFG_TAG_INTERFACE_LEVEL, interfaceLvl);
      cfg->commit();
    }
  }

  statusLed = new Supla::Device::StatusLed(STATUS_LED_PIN, STATUS_LED_INVERTED);
  buttonCfg = new Supla::Control::Button(CONFIG_BUTTON_PIN, true, true);
  buttonCfg->configureAsConfigButton(&SuplaDevice);

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;
  new Supla::Html::StatusLedParameters;
  new Supla::Html::ButtonUpdate(&suplaServer);

  auto interfaceLevel = new Supla::Html::SelectInputParameter(
      CFG_TAG_INTERFACE_LEVEL,
      "Interface Level (changing this requires removing and re-adding the device in Supla Cloud)");
  interfaceLevel->registerValue("Minimal", 1);
  interfaceLevel->registerValue("Standard", 2);
  interfaceLevel->registerValue("Debug", 3);

  SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);
  SuplaDevice.begin();

  haierSerial.begin(HAIER_UART_BAUD, SERIAL_8N1, HAIER_UART_RX_PIN,
                    HAIER_UART_TX_PIN);
  haierController.begin();
  haierSupla.begin(nullptr, interfaceLvl, kHaierChannelConfig);
}

void loop() {
  haierController.loop();
  SuplaDevice.iterate();
  haierSupla.iterate();
}