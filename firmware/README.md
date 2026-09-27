# Haier2Supla 0.9.0 — ESP32-C3 Super Mini

Firmware dla Tenstar ROBOT ESP32-C3 Super Mini z pamięcią flash 4 MB.

## Pliki

- `haier2supla-0.9.0-esp32-c3-super-mini-factory.bin` — pierwsze wgranie, adres `0x0`.
- `haier2supla-0.9.0-esp32-c3-super-mini-app.bin` — aktualizacja przez WWW lub po kablu od adresu `0x10000`.

Nie wgrywaj pliku `app.bin` do pustej płytki.

## Domyślne piny

- RX: GPIO20
- TX: GPIO21
- LED: GPIO8, aktywna stanem niskim
- BOOT/konfiguracja: GPIO9

## Build

- Arduino-ESP32 3.3.11
- ESP-IDF 5.5.5
- partycja `min_spiffs.csv`
- RAM: 45 752 / 327 680 B (14,0%)
- aplikacja: 1 515 759 / 1 966 080 B (77,1%)

## SHA-256

```text
DFBE9ADEEA5FCD745EC705BB13687828F59DC9460FA3DC58791F51D1B14B2875  haier2supla-0.9.0-esp32-c3-super-mini-app.bin
AF29067B9DCC4FAB3FA67F176CB62C94D7AA289875F491EA4B602ED58F48FA98  haier2supla-0.9.0-esp32-c3-super-mini-factory.bin
```

Wersja hOn została sprawdzona na Haier Tundra Plus AS35TADHRA-CL. SmartAir2
pozostaje obsługiwany, ale ta wersja nie była ponownie testowana na fizycznym
urządzeniu SmartAir2.
