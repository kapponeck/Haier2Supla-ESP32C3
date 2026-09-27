# Haier2Supla Library Documentation

Haier2Supla is an Arduino library for ESP32 boards that integrates Haier air conditioners with SUPLA.

This fork supports the Haier SmartAir2 and hOn UART protocols. The protocol is detected automatically during startup.

## What This Library Does

The library provides two main layers:

- SmartAir2 or hOn communication with the indoor unit over UART
- SUPLA channel adapters that expose Haier AC functions as HVAC, relay, and sensor channels

The intent is to treat the air conditioner as a remote-controlled device. The AC unit keeps its own internal control logic, while the ESP32 reads current state and sends requested changes through the SmartAir2 interface.

## Dependencies

This library depends on the following components:

- ESP32 Arduino core
	Required for `HardwareSerial`, ESP32 runtime, and board support.

- HaierProtocol library
	Required for packet handling and protocol-level communication with the AC unit.

- SUPLA device library
	Required for HVAC, relay, sensor, configuration, and web interface integration.

## Protocol Support

This library supports:

- Haier SmartAir2 protocol
- Haier hOn protocol with CRC and device-ID initialization

## Main Components

- `HaierSmartair2Controller`
	Handles UART communication, state tracking, command sending, and SmartAir2-specific logic.

- `HaierSuplaChannels`
	Creates SUPLA channels based on sketch-defined level configuration and keeps them synchronized with the controller.

- `HaierSuplaStateSync`
	Synchronizes created channels with the latest AC state.

- `HaierAcHvacChannel`
	HVAC channel adapter used for target temperature and HVAC-related control.

- `HaierAcVirtualRelay` and related relay helpers
	Helper classes used to expose AC functions as SUPLA relay-style channels.

## Available Channels

All available channel identifiers are defined in `HaierSuplaChannelId`.

### Control Channels

- `Hvac` - main control of the AC unit: on/off, temperature setting, room temperature and humidity sensor
- `ModeCoolRelay` 			- AC Cool mode
- `ModeHeatRelay`			- AC Heat mode
- `ModeDryRelay`			- AC Dry mode
- `ModeFanRelay`			- AC Fan only mode
- `ModeAutoRelay`			- AC Auto mode
- `FanAutoRelay`			- Fan Auto speed mode
- `FanHighRelay`			- Fan High speed mode
- `FanMidRelay`				- Fan Mid speed mode
- `FanLowRelay`				- Fan Low speed mode
- `SwingHorizontalRelay`	- Horizontal Swing	
- `SwingVerticalRelay`		- Vertical Swing
- `HealthRelay`				- Health mode
- `PowerSavingRelay`		- Power saving mode (3 beeps at the end of sequence ON, 2 beeps OFF)
- `DisplayTemperatureRelay`	- not used
- `QuietRelay`				- Quite mode
- `DisplayRelay`			- Disable Display
- `TurboRelay`				- Turbo mode
- `TenDegreeRelay`			- 10 degree mode
- `TargetTemperatureStepRelay`	- alternative change of temperature setting

### Sensor and State Channels

- `TemperatureHumidity` 	- room Temperature and Humidity (required for HVAC channel)
- `PowerStateSensor`		- AC Power state
- `SetTemperatureSensor`	- room Temperature sensor
- `ModeCoolSensor`			- AC Cool mode state
- `ModeHeatSensor`			- AC Heat mode state
- `ModeDrySensor`			- AC Dry mode state
- `ModeAutoSensor`			- AC Auto mode state
- `ModeFanSensor`			- Fan Auto speed state
- `FanHighSensor`			- Fan High speed state
- `FanMidSensor`			- Fan Mid speed state
- `FanLowSensor`			- Fan Low speed state
- `FanAutoSensor`			- Fan Auto speed state
- `TurboSensor`				- Fan Turbo mode state
- `QuietSensor`				- Quite mode state
- `DisplaySensor`			- Disable Display state
- `LockRemoteSensor`		- Lock remote state
- `HealthSensor`			- Health mode state
- `CompressorSensor`		- Compressor state
- `TenDegreeSensor`			- Ten Degree mode state
- `UseSwingBitsSensor`		- Use Swing Bits sensor
- `HorizontalSwingSensor`	- Horizontal Swing mode state
- `VerticalSwingSensor`		- Vertical Swing mode state

## Level Configuration

Channel exposure is defined in the sketch, not hardcoded in the library user API.

The library uses a single universal mechanism for assigning channels to levels.

### Universal Level Helper

Use:

- `makeHaierSuplaChannelConfig()`
- `setHaierSuplaChannelLevel(config, level, channels, count)`

This lets the sketch keep separate channel arrays per level while using one common API for configuration.

Example:

```cpp
static HaierSuplaChannelConfig buildHaierChannelConfig() {
	HaierSuplaChannelConfig config = makeHaierSuplaChannelConfig();

	setHaierSuplaChannelLevel(
			config, 1, kLevel1Channels,
			sizeof(kLevel1Channels) / sizeof(kLevel1Channels[0]));
	setHaierSuplaChannelLevel(
			config, 2, kLevel2Channels,
			sizeof(kLevel2Channels) / sizeof(kLevel2Channels[0]));
	setHaierSuplaChannelLevel(
			config, 3, kLevel3Channels,
			sizeof(kLevel3Channels) / sizeof(kLevel3Channels[0]));

	return config;
}
```

The helper supports flexible level numbers. The current implementation stores up to `8` configured levels in the configuration object.

The library itself can therefore handle additional levels such as `4`, `5`, and higher, as long as your sketch passes them into the configuration. Whether those levels are selectable from a web interface depends entirely on the sketch implementation.

## Channel Naming

Each `HaierSuplaChannelDefinition` contains:

- `HaierSuplaChannelId id`
- `const char *caption`

The `caption` is treated as a channel name suffix.

If `roomName` is set, the final caption becomes:

```text
<roomName>-<caption>
```

If `roomName` is empty:

- custom captions remain unchanged
- default library captions are generated without a leading `-`

## Examples

The library includes the following examples:

- `examples/Haier2Supla`
	The main reference application for full integration Haier SmartAir2 device to Supla device and broader project-specific configuration.

- `examples/ControllerOnly/`
	Minimal controller-only example without SUPLA channel configuration.

- `examples/ConfigurableLevels/`
	Full SUPLA example with web-configurable interface levels `1-3`. It also shows the universal level helper API.

- `examples/Level1Only/`
	Simplified SUPLA example that creates only level 1 channels and does not expose interface level selection in the web configuration.

## Installation

Copy the `Haier2Supla` directory into your Arduino `libraries` folder.

After installation, restart Arduino IDE if needed so the new library and examples are detected.

# Changelog
## v.0.9.0
- added automatic SmartAir2/hOn detection
- added hOn status reading and control with CRC
- added ESP32-C3 Super Mini defaults
- simplified the compact UI: HVAC Heat/Cool/Auto, separate Dry and Fan Only,
  one fan-speed slider and separate horizontal/vertical swing
- improved synchronization after local and remote-control changes

## v.0.8.0
- added trubo and 10deg mode
- improved examples
