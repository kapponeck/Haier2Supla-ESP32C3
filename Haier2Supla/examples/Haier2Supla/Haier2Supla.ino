// Main reference example for the Haier2Supla Arduino library.
//
// Scope:
// - demonstrates full SUPLA integration for Haier SmartAir2 units,
// - exposes interface levels 1-3 with a broad set of control and state
//   channels,
// - supports web configuration for Wi-Fi, protocol settings, room name,
//   hardware pins, interface level, and OTA update.
//
// This sketch is intended as the most complete example in the library. Use it
// as the baseline for full-featured deployments or as a starting point for
// custom variants derived from the complete SUPLA integration.

#define AC_ROOM "Haier"
#define FW_VERSION "0.9.0"
#define FW_BUILD_INFO __DATE__ " " __TIME__

#include <Arduino.h>
#include <cstring>
#if defined(ESP32) || defined(SUPLA_DEVICE_ESP32)
#include <esp_mac.h>
#endif
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
#include <supla/network/html/custom_text_parameter.h>
#include <supla/network/html/custom_parameter.h>
#include <supla/network/html/select_input_parameter.h>
#include <supla/control/button.h>
#include <supla/control/virtual_relay.h>
#include <supla/storage/eeprom.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/html/text_cmd_input_parameter.h>
#include <supla/network/html/select_cmd_input_parameter.h>
#include <supla/events.h>

#include <supla/sensor/virtual_thermometer.h>
#include <supla/sensor/virtual_therm_hygro_meter.h>
#include <supla/sensor/virtual_binary.h>
#include <Haier2Supla.h>

// ESP32-C3 uses one radio for the configuration AP and Wi-Fi scans. Scanning
// while the AP is active can delay DHCP long enough for clients to assign
// themselves a 169.254.x.x address. Keep provisioning connectivity stable;
// the target SSID can still be entered manually in the configuration page.
class StableConfigEspWifi : public Supla::ESPWifi {
 public:
  void setup() override {
    Supla::ESPWifi::setup();
    if (mode == Supla::DEVICE_MODE_CONFIG) {
      // AP+STA is only needed for scanning. Keeping STA enabled can make the
      // provisioning AP follow station-side radio transitions.
      WiFi.mode(WIFI_MODE_AP);
      ensureConfigAp();
    }
  }

  bool iterate() override {
    const bool result = Supla::ESPWifi::iterate();
    if (mode == Supla::DEVICE_MODE_CONFIG) {
      const uint32_t now = millis();
      if (now - lastApCheckMs >= 5000) {
        lastApCheckMs = now;
        ensureConfigAp();
      }
    }
    return result;
  }

  void startConfigModeScan() override {
  }

 private:
  void ensureConfigAp() {
    const bool wrongMode = WiFi.getMode() != WIFI_MODE_AP;
    const bool wrongSsid = WiFi.softAPSSID() != hostname;
    if (!wrongMode && !wrongSsid) {
      return;
    }

    SUPLA_LOG_WARNING("WiFi: recovering configuration AP");
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(hostname, nullptr, 6);
  }

  uint32_t lastApCheckMs = 0;
};

// Global SUPLA runtime objects used by the device, storage, and web UI.
// SUPLA components
Supla::Eeprom eeprom;
StableConfigEspWifi wifi;
Supla::EspWebServer suplaServer;
Supla::LittleFsConfig configSupla;
Supla::Device::StatusLed *statusLed = nullptr;
Supla::Control::Button *buttonCfg = nullptr;

// Keys used to store custom configuration values in persistent storage.
// Configuration tags for custom parameters
#define CFG_TAG_ROOM_NAME "room_name"
#define CFG_TAG_RX_PIN "uart_rx_pin"
#define CFG_TAG_TX_PIN "uart_tx_pin"
#define CFG_TAG_INTERFACE_LEVEL "iface_level"
#define CFG_TAG_LED_PIN "status_led_pin"
#define CFG_TAG_BUTTON_PIN "cfg_button_pin"


// Haier SmartAir2 over UART1
HardwareSerial HaierSerial(1);

// Default hardware settings for the Tenstar Robot ESP32-C3 Super Mini.
// UART pin values can be overridden from web config.
constexpr uint8_t DEFAULT_STATUS_LED_PIN = 8;
constexpr bool DEFAULT_STATUS_LED_INVERTED = true;
constexpr uint8_t DEFAULT_BUTTON_CFG_PIN = 9;
constexpr uint8_t DEFAULT_HAIER_UART_RX_PIN = 20;
constexpr uint8_t DEFAULT_HAIER_UART_TX_PIN = 21;
constexpr int32_t DEFAULT_INTERFACE_LEVEL = 1;
constexpr uint32_t HAIER_UART_BAUD = 9600;

// Global variables for configurable pins (will be initialized from config)
uint8_t statusLedPin = DEFAULT_STATUS_LED_PIN;
uint8_t buttonCfgPin = DEFAULT_BUTTON_CFG_PIN;
uint8_t haierUartRxPin = DEFAULT_HAIER_UART_RX_PIN;
uint8_t haierUartTxPin = DEFAULT_HAIER_UART_TX_PIN;

// Core Haier controller and SUPLA channel bridge instances.
HaierSmartair2Controller haierController(
    HaierSerial,
    DEFAULT_HAIER_UART_RX_PIN,  // Will be updated in setup()
    DEFAULT_HAIER_UART_TX_PIN,  // Will be updated in setup()
    HAIER_UART_BAUD,
    false  // use_crc = false - CRC is not used by SmartAir2, and enabling it would require changes in the protocol handler to not expect CRC bytes
);
HaierSuplaChannels haierSupla(&haierController);

// Captions below are suffixes. When roomName is set, the final caption becomes
// <roomName>-<caption>.
// Level 1 exposes a compact UI: the main HVAC channel handles heat, cool and
// auto; dry and fan-only remain separate because SUPLA's HvacBase does not yet
// expose these modes in its standard UI. Fan speed uses one slider.
static const HaierSuplaChannelDefinition kLevel1Channels[] = {
    {HaierSuplaChannelId::Hvac, "HVAC"},
    {HaierSuplaChannelId::TemperatureHumidity, "Room Temp & Humidity"},
    {HaierSuplaChannelId::ModeDryRelay, "Mode Dry"},
    {HaierSuplaChannelId::ModeFanRelay, "Fan Only"},
    {HaierSuplaChannelId::FanSpeedChannel, "Fan Speed"},
    {HaierSuplaChannelId::SwingHorizontalRelay, "Swing Horizontal"},
    {HaierSuplaChannelId::SwingVerticalRelay, "Swing Vertical"},
  };

// Level 2 adds secondary control features that are useful but not essential.
  static const HaierSuplaChannelDefinition kLevel2Channels[] = {
    {HaierSuplaChannelId::HealthRelay, "Health Mode"},
    {HaierSuplaChannelId::PowerSavingRelay, "Power Saving Toggle"},
    {HaierSuplaChannelId::QuietRelay, "Quiet Mode"},
    {HaierSuplaChannelId::DisplayRelay, "Display Disable"},
  };

// Level 3 adds diagnostic/state channels and advanced control relays.
  static const HaierSuplaChannelDefinition kLevel3Channels[] = {
    {HaierSuplaChannelId::TurboRelay, "Turbo Mode"},
    {HaierSuplaChannelId::PowerStateSensor, "Power State"},
    {HaierSuplaChannelId::ModeCoolSensor, "State Cool"},
    {HaierSuplaChannelId::ModeHeatSensor, "State Heat"},
    {HaierSuplaChannelId::ModeDrySensor, "State Dry"},
    {HaierSuplaChannelId::ModeAutoSensor, "State Auto"},
    {HaierSuplaChannelId::ModeFanSensor, "State Fan"},
    {HaierSuplaChannelId::FanHighSensor, "State Fan High"},
    {HaierSuplaChannelId::FanMidSensor, "State Fan Mid"},
    {HaierSuplaChannelId::FanLowSensor, "State Fan Low"},
    {HaierSuplaChannelId::FanAutoSensor, "State Fan Auto"},
    {HaierSuplaChannelId::TurboSensor, "State Turbo"},
    {HaierSuplaChannelId::QuietSensor, "State Quiet"},
    {HaierSuplaChannelId::DisplaySensor, "State Display Disabled"},
    {HaierSuplaChannelId::LockRemoteSensor, "State Remote Lock"},
    {HaierSuplaChannelId::HealthSensor, "State Health"},
    {HaierSuplaChannelId::CompressorSensor, "State Compressor"},
    {HaierSuplaChannelId::TenDegreeRelay, "10C Mode"},
    {HaierSuplaChannelId::TenDegreeSensor, "State 10C Mode"},
    {HaierSuplaChannelId::UseSwingBitsSensor, "State Swing Enabled"},
    {HaierSuplaChannelId::HorizontalSwingSensor, "State Horizontal Swing"},
    {HaierSuplaChannelId::VerticalSwingSensor, "State Vertical Swing"},
    {HaierSuplaChannelId::SetTemperatureSensor, "Set Temperature"},
    {HaierSuplaChannelId::TargetTemperatureStepRelay, "Adjust Set Temperature"},
};

static HaierSuplaChannelConfig buildHaierChannelConfig() {
  HaierSuplaChannelConfig config = makeHaierSuplaChannelConfig();

  // Register which channel definitions belong to each supported interface level.
  // The helper supports additional levels as well, but this sketch exposes
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

// Builds the legacy MAC suffix format used by earlier auto-generated names.
static void buildLegacyEfuseMacSuffix(char *suffix, size_t suffixSize) {
  const unsigned long long efuseMac =
      static_cast<unsigned long long>(ESP.getEfuseMac());
  snprintf(suffix, suffixSize, "%012llX", efuseMac & 0xFFFFFFFFFFFFULL);
}

// Reads the base MAC address and falls back to efuse MAC when needed.
static void buildMacSuffix(char *suffix, size_t suffixSize) {
#if defined(ESP32) || defined(SUPLA_DEVICE_ESP32)
  uint8_t baseMac[6] = {};
  if (esp_read_mac(baseMac, ESP_MAC_WIFI_STA) == ESP_OK) {
    snprintf(suffix, suffixSize, "%02X%02X%02X%02X%02X%02X", baseMac[0],
             baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    return;
  }
#endif
  const unsigned long long efuseMac =
      static_cast<unsigned long long>(ESP.getEfuseMac());
  snprintf(suffix, suffixSize, "%012llX", efuseMac & 0xFFFFFFFFFFFFULL);
}

// Generates the current default device name based on the device MAC address.
static void buildDefaultDeviceName(char *deviceName, size_t deviceNameSize) {
  char macSuffix[13] = {};
  buildMacSuffix(macSuffix, sizeof(macSuffix));
  snprintf(deviceName, deviceNameSize, "Supla2Haier-%s", macSuffix);
}

// Reproduces the legacy device-name format for migration checks.
static void buildLegacyEfuseDeviceName(char *deviceName,
                                       size_t deviceNameSize) {
  char macSuffix[13] = {};
  buildLegacyEfuseMacSuffix(macSuffix, sizeof(macSuffix));
  snprintf(deviceName, deviceNameSize, "Supla2Haier-%s", macSuffix);
}

// Converts the compiler month abbreviation into a numeric month value.
static uint8_t parseBuildMonth(const char *buildDate) {
  static const char *const months[] = {"Jan", "Feb", "Mar", "Apr",
                                       "May", "Jun", "Jul", "Aug",
                                       "Sep", "Oct", "Nov", "Dec"};

  for (uint8_t index = 0; index < 12; ++index) {
    if (strncmp(buildDate, months[index], 3) == 0) {
      return index + 1;
    }
  }

  return 0;
}

// Builds a compact firmware version string that includes the build timestamp.
static void buildCompactSwVersion(char *swVersion, size_t swVersionSize) {
  const char *buildDate = __DATE__;
  const char *buildTime = __TIME__;
  const uint8_t month = parseBuildMonth(buildDate);
  const uint8_t day = static_cast<uint8_t>(atoi(buildDate + 4));
  const uint8_t year = static_cast<uint8_t>(atoi(buildDate + 9));
  const uint8_t hour = static_cast<uint8_t>(atoi(buildTime));
  const uint8_t minute = static_cast<uint8_t>(atoi(buildTime + 3));

  snprintf(swVersion, swVersionSize, "%s-%02u%02u%02u%02u%02u", FW_VERSION,
           year, month, day, hour, minute);
}

// Detects names created by earlier auto-naming logic so they can be refreshed.
static bool isLegacyAutoDeviceName(const char *deviceName) {
  if (strncmp(deviceName, "Haier2Supla_", 12) == 0) {
    return true;
  }

  char legacyDeviceName[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  buildLegacyEfuseDeviceName(legacyDeviceName, sizeof(legacyDeviceName));
  return strcmp(deviceName, legacyDeviceName) == 0;
}

// Initializes storage, web configuration, SUPLA services, and the Haier bridge.
void setup() {
  Serial.begin(115200);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
  // Diagnostics must never stall the main loop when USB is connected to a
  // host but no serial terminal is consuming output.
  Serial.setTxTimeoutMs(0);
#endif
  delay(2000); // Wait for serial to initialize

  // Load persisted configuration before creating SUPLA and UART objects.
  Supla::Storage::Init();
  auto cfg = Supla::Storage::ConfigInstance();

  // Restore user-defined room name, pins, and interface level when available.
  char roomName[11] = {};
  const bool hasStoredRoomName =
      cfg && cfg->getString(CFG_TAG_ROOM_NAME, roomName, sizeof(roomName));
  if (!hasStoredRoomName) {
    strncpy(roomName, AC_ROOM, sizeof(roomName) - 1);
  }

  int32_t rxPin = DEFAULT_HAIER_UART_RX_PIN;
  int32_t txPin = DEFAULT_HAIER_UART_TX_PIN;
  int32_t ledPin = DEFAULT_STATUS_LED_PIN;
  int32_t cfgPin = DEFAULT_BUTTON_CFG_PIN;
  int32_t interfaceLvl = DEFAULT_INTERFACE_LEVEL;
  bool hasStoredRxPin = false;
  bool hasStoredTxPin = false;
  bool hasStoredLedPin = false;
  bool hasStoredCfgPin = false;
  bool hasStoredInterfaceLvl = false;
  if (cfg) {
    hasStoredRxPin = cfg->getInt32(CFG_TAG_RX_PIN, &rxPin);
    hasStoredTxPin = cfg->getInt32(CFG_TAG_TX_PIN, &txPin);
    hasStoredLedPin = cfg->getInt32(CFG_TAG_LED_PIN, &ledPin);
    hasStoredCfgPin = cfg->getInt32(CFG_TAG_BUTTON_PIN, &cfgPin);
    hasStoredInterfaceLvl = cfg->getInt32(CFG_TAG_INTERFACE_LEVEL, &interfaceLvl);
    if (!hasStoredInterfaceLvl) {
      interfaceLvl = DEFAULT_INTERFACE_LEVEL;
      cfg->setInt32(CFG_TAG_INTERFACE_LEVEL, interfaceLvl);
      cfg->commit();
      hasStoredInterfaceLvl = true;
    }
  }

  statusLedPin = static_cast<uint8_t>(ledPin);
  buttonCfgPin = static_cast<uint8_t>(cfgPin);
  haierUartRxPin = static_cast<uint8_t>(rxPin);
  haierUartTxPin = static_cast<uint8_t>(txPin);

  // Refresh persisted auto-generated names when they come from older formats.
  char deviceName[SUPLA_DEVICE_NAME_MAXSIZE] = {};
  const bool hasStoredDeviceName = cfg && cfg->getDeviceName(deviceName);
  if (!hasStoredDeviceName || isLegacyAutoDeviceName(deviceName)) {
    buildDefaultDeviceName(deviceName, sizeof(deviceName));
    if (cfg) {
      cfg->setDeviceName(deviceName);
      cfg->commit();
    }
  }

  SUPLA_LOG_DEBUG(
      "Startup config: storage=%s room_name=%s (%s) device_name=%s (%s) "
      "interface_level=%d (%s) status_led_pin=%d (%s) cfg_button_pin=%d (%s) "
      "uart_rx_pin=%d (%s) uart_tx_pin=%d (%s)",
      cfg ? "ready" : "missing",
      roomName,
      hasStoredRoomName ? "stored" : "default",
      deviceName,
      hasStoredDeviceName ? "stored" : "generated",
      interfaceLvl,
      hasStoredInterfaceLvl ? "stored" : "default",
      statusLedPin,
      hasStoredLedPin ? "stored" : "default",
      buttonCfgPin,
      hasStoredCfgPin ? "stored" : "default",
      haierUartRxPin,
      hasStoredRxPin ? "stored" : "default",
      haierUartTxPin,
      hasStoredTxPin ? "stored" : "default");

  SuplaDevice.setName(deviceName);
  SuplaDevice.setCustomHostnamePrefix("SUPLA2HAIER");
  char swVersion[18] = {};
  buildCompactSwVersion(swVersion, sizeof(swVersion));
  SuplaDevice.setSwVersion(swVersion);

  SUPLA_LOG_DEBUG("Firmware version: %s, build: %s", FW_VERSION,
                  FW_BUILD_INFO);

  // Create hardware helpers for status indication and entering config mode.
  statusLed = new Supla::Device::StatusLed(
      statusLedPin, DEFAULT_STATUS_LED_INVERTED);

  // config button to reset WiFi and other settings
  buttonCfg = new Supla::Control::Button(buttonCfgPin, true, true);
  buttonCfg->configureAsConfigButton(&SuplaDevice);

  // Register built-in and custom pages for the SUPLA web configuration UI.
  // SUPLA web configuration
  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::WifiParameters;
  new Supla::Html::ProtocolParameters;
  new Supla::Html::StatusLedParameters;
  new Supla::Html::ButtonUpdate(&suplaServer);
  
  // Custom configuration parameters
  new Supla::Html::CustomTextParameter(CFG_TAG_ROOM_NAME, "Room Name", 10);
  new Supla::Html::CustomParameter(CFG_TAG_RX_PIN, "UART RX Pin", DEFAULT_HAIER_UART_RX_PIN);
  new Supla::Html::CustomParameter(CFG_TAG_TX_PIN, "UART TX Pin", DEFAULT_HAIER_UART_TX_PIN);
  new Supla::Html::CustomParameter(CFG_TAG_LED_PIN, "Status LED Pin", DEFAULT_STATUS_LED_PIN);
  new Supla::Html::CustomParameter(CFG_TAG_BUTTON_PIN, "Config Button Pin", DEFAULT_BUTTON_CFG_PIN);
  
  // Interface Level dropdown
  auto interfaceLevel = new Supla::Html::SelectInputParameter(
      CFG_TAG_INTERFACE_LEVEL,
      "Interface Level (changing this requires removing and re-adding the device in Supla Cloud)");
  interfaceLevel->registerValue("Minimal", 1);
  interfaceLevel->registerValue("Standard", 2);
  interfaceLevel->registerValue("Debug", 3);

  // Start SUPLA services and then initialize the Haier UART bridge.
  SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);
  SuplaDevice.begin();

  // Init Haier controller with configured pins
  // Note: The controller was already constructed with default pins
  // For proper reconfiguration, you may need to reinitialize the serial
  HaierSerial.begin(HAIER_UART_BAUD, SERIAL_8N1, haierUartRxPin, haierUartTxPin);
  haierController.begin();
  haierSupla.begin(roomName, interfaceLvl, kHaierChannelConfig);
}

// Keeps the Haier controller and SUPLA state synchronization running.
void loop() {
  // Keep the controller protocol, SUPLA runtime, and channel state sync active.
  haierController.loop();
  SuplaDevice.iterate();

  haierSupla.iterate();

}
