# Haier2Supla ESP32-C3

Fork projektu [piglet0/haier2supla](https://github.com/piglet0/haier2supla),
rozszerzony o obsługę protokołu hOn i płytki ESP32-C3 Super Mini.

Główny szkic znajduje się w `examples/Haier2Supla`.
## Base of the project - references
Communication to Haier AC is developed by paveldn. He preapred a HaierProtocol library. Communication with Haier AC is based on his other project haier-esphome.
- Supla-Device https://github.com/SUPLA/supla-device
- HaierProtocol https://github.com/paveldn/HaierProtocol
- Haier-esphome https://github.com/paveldn/haier-esphome
## Hardware requirements
### Communication with Haier AC
Haier AC communicate using standard UART serial. Units which uses SmartAir2 app connects UART using standard USB plug. For more information please refer to https://github.com/paveldn/haier-esphome/blob/master/README.rst
### Tested version
Kod był testowany na Tenstar ROBOT ESP32-C3 Super Mini i klimatyzatorze
Haier Tundra Plus AS35TADHRA-CL z protokołem hOn.

Domyślne piny: RX GPIO20, TX GPIO21, LED GPIO8 i BOOT GPIO9. Piny można
zmienić w konfiguracji WWW.
## Current status of the project
Version 
- Automatyczne wykrywanie protokołów SmartAir2 i hOn.
- Working functions: On/Off, AC Mode, Fan Mode, Room temperature sensor, Setting of room temperature, Health mode, Display On/Off
- UART, Button, LED pins can be set via web config
- Interface can be set via web config to:
  - minimal - Cool/Heat mode only, Fan mode and temperature setting
  - standard - all AC modes, Fan modes and temperature setting
  - debug - additional sensor channels showing state of AC functions
Note that change of interface level requires removing device from supla cloud and registration with new channels.
Interface level can be changed in the project sketch.
## OTA Update
For the first install upload 4MB file haier2supla-v0.0.0-merged.bin
For OTA update (available in web config) use smaller file haier2supla-v0.0.0.bin 
# Compilation
Project was compiled in VSCode with Arduino Maker plugin. It can be also compiled in Arduino IDE.
## Libraries
Project requries following libriaries to be installed:
- Supla-device - install from the Arduino catalogue
- HaierProtocol - install from the Arduino catalogue
## Compilation settings
- USB CDC On Boot: Enabled - to allow Logs displayed in board USB serial (115200 bound rate)
- Partition scheme: Minimal SPIFFS (1.9MB APP with OTA/ 128KB SPIFFS)
## release
Gotowe pliki BIN dla ESP32-C3 Super Mini znajdują się w katalogu `firmware-c3`.
# Change log
## v0.0.5
- initial release
- all main AC functions working
## v0.2.0
- change in UX by using HVAC channel. AC modes kept in separate switches.
- option of timer and weekly programs
- added humidity reading
- option to change pins of LED and config button in web config
- bug fix
## v0.2.2
- OTA update
- power-saving mode (command combo)
- display value change (comand compo) - not confirmed
- wifi icon on AC
## v0.2.3
- removed display value change - not working
## v0.2.5
- change to Arduino library
- device name base on MAC to avoid same name for simillar boards
## v0.8.0
- checked examples, comments added
- added turbo mode and 10deg mode in debug interface level
- unified version numbering with library
## v0.9.0
- automatic SmartAir2/hOn detection
- hOn status reading and control with CRC
- ESP32-C3 Super Mini support
- simplified HVAC and fan control
- synchronization with changes made from the original remote
