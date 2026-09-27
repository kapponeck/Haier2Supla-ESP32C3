# Haier2Supla ESP32-C3 0.9.0

My version of the [piglet0/haier2supla](https://github.com/piglet0/haier2supla)
project. It adds support for air conditioners using the hOn protocol and
provides ready-to-use firmware for the Tenstar ROBOT ESP32-C3 Super Mini 4 MB.

## Features

- automatic SmartAir2 or hOn protocol detection;
- room temperature and full AC status reading;
- power control;
- Heat, Cool and Auto modes through the HVAC channel;
- separate Dry and Fan Only controls;
- target temperature control from SUPLA;
- one fan-speed slider: Auto, Low, Mid and High;
- separate horizontal and vertical swing controls;
- Health, Quiet, Turbo, display, energy-saving and 10°C modes;
- SUPLA status updates after using the original remote control;
- Wi-Fi, pin and interface-level configuration through the web interface;
- firmware updates through the web interface.

The first valid state received from the air conditioner takes priority over
the default SUPLA state. This prevents the device from sending unintended
settings to the AC during startup.

## Changes from the piglet0 version

- hOn support with CRC and device-ID initialization;
- automatic SmartAir2/hOn detection;
- default configuration for the ESP32-C3 Super Mini;
- simplified controls: HVAC, Dry, Fan Only and one fan-speed slider;
- improved synchronization after changes from SUPLA or the remote control;
- protection against an older status frame reverting a newly sent command;
- USB logging no longer blocks the ESP32-C3 when no terminal is connected.

## SUPLA controls

The `Minimal` interface level contains HVAC, temperature and humidity, Dry,
Fan Only, the fan-speed slider and both swing controls.

The `Standard` level adds Health, Power Saving, Quiet and display control.

The `Debug` level adds Turbo, 10°C mode and detailed AC status channels.

Changing the interface level changes the number of SUPLA channels. The device
may need to be removed from SUPLA Cloud and registered again after this change.

Auto mode also sets the fan to Auto. Fan Only does not use Auto fan speed, so
the firmware selects Low instead. The display-temperature toggle sequence is
not supported with the hOn protocol.

## Board and default pins

- board: Tenstar ROBOT ESP32-C3 Super Mini, 4 MB flash;
- UART RX: GPIO20;
- UART TX: GPIO21;
- status LED: GPIO8, active low;
- BOOT/configuration button: GPIO9;
- USB CDC on boot: enabled.

The pins can be changed in the web configuration. ESP32-C3 GPIO uses 3.3 V
logic and is not 5 V tolerant. The ESP32 replaces the original AC Wi-Fi dongle.

## Ready-to-use firmware

The compiled files are available in the `firmware` directory:

- `*-factory.bin` — complete image for the first installation at address `0x0`;
- `*-app.bin` — web update image, or cable upload at address `0x10000`.

Do not upload `app.bin` to an empty board. Checksums are available in
`SHA256SUMS.txt`.

## Arduino IDE

1. Install ESP32 board support version 3.3.11.
2. Copy the `Haier2Supla`, `HaierProtocol` and `SuplaDevice` directories to
   your Arduino libraries directory.
3. Open `Haier2Supla/examples/Haier2Supla/Haier2Supla.ino`.
4. Select `ESP32C3 Dev Module`, 4 MB flash, `USB CDC On Boot: Enabled` and
   `Minimal SPIFFS (1.9MB APP with OTA)`.
5. Compile and upload the sketch.

## PlatformIO

```text
python -m platformio run -e esp32-c3-super-mini
```

## Verification

Version `0.9.0` compiles successfully. hOn was tested with a Haier Tundra Plus
AS35TADHRA-CL. Protocol detection, CRC, temperature, status reading, control
and SUPLA registration are working.

SmartAir2 remains supported in the code, but this version has not been tested
again on a physical SmartAir2 unit.

## Credits and licenses

Original project: [piglet0/haier2supla](https://github.com/piglet0/haier2supla).
Haier communication uses
[paveldn/HaierProtocol](https://github.com/paveldn/HaierProtocol), while SUPLA
support is provided by
[SUPLA/supla-device](https://github.com/SUPLA/supla-device).

The main project is licensed under GPL-3.0. Dependency licenses are included
in their respective directories.
