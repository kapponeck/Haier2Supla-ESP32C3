# Haier2Supla ESP32-C3 0.9.0

Moja wersja projektu [piglet0/haier2supla](https://github.com/piglet0/haier2supla).
Dodałem obsługę klimatyzatorów z protokołem hOn i przygotowałem firmware dla
Tenstar ROBOT ESP32-C3 Super Mini 4 MB.

Repozytorium: https://github.com/kapponeck/Haier2Supla-ESP32C3

## Co działa

- automatyczne wykrywanie SmartAir2 lub hOn;
- odczyt temperatury i stanu klimatyzatora;
- włączanie i wyłączanie;
- tryby Heat, Cool i Auto w kanale HVAC;
- osobne sterowanie Dry i Fan Only;
- zadawanie temperatury z SUPLA;
- jeden suwak wentylatora: Auto, Low, Mid i High;
- osobny swing poziomy i pionowy;
- Health, Quiet, Turbo, wyświetlacz, oszczędzanie energii i tryb 10°C;
- aktualizacja stanu SUPLA po użyciu oryginalnego pilota;
- konfiguracja Wi-Fi, pinów i poziomu interfejsu przez WWW;
- aktualizacja firmware przez WWW.

Pierwszy prawidłowy stan odczytany z klimatyzatora ma pierwszeństwo przed
domyślnym stanem SUPLA. Dzięki temu urządzenie po starcie nie wysyła od razu
przypadkowych ustawień do klimatyzatora.

## Zmiany względem wersji piglet0

- obsługa hOn z CRC i odczytem identyfikatora urządzenia;
- automatyczne rozpoznawanie hOn/SmartAir2;
- domyślna konfiguracja dla ESP32-C3 Super Mini;
- uproszczone sterowanie: HVAC + Dry + Fan Only + jeden suwak wentylatora;
- poprawiona synchronizacja po sterowaniu z SUPLA i pilota;
- zabezpieczenie przed cofnięciem świeżo wysłanej komendy przez starszą ramkę;
- logowanie USB nie blokuje pracy urządzenia, gdy terminal nie jest otwarty.

## Sterowanie w SUPLA

Poziom `Minimal` zawiera HVAC, temperaturę i wilgotność, Dry, Fan Only, suwak
wentylatora oraz oba kierunki swing.

Poziom `Standard` dodaje Health, Power Saving, Quiet i sterowanie wyświetlaczem.

Poziom `Debug` dodaje Turbo, tryb 10°C i kanały pokazujące szczegółowy stan
klimatyzatora.

Zmiana poziomu interfejsu zmienia liczbę kanałów. Po zmianie może być potrzebne
usunięcie urządzenia z SUPLA Cloud i ponowna rejestracja.

W trybie Auto wentylator przechodzi na Auto. W trybie Fan Only prędkość Auto
nie jest używana — firmware wybiera Low. Funkcja przełączania temperatury
wyświetlanej na panelu nie jest obsługiwana przez hOn.

## Płytka i piny

- płytka: Tenstar ROBOT ESP32-C3 Super Mini, flash 4 MB;
- UART RX: GPIO20;
- UART TX: GPIO21;
- LED: GPIO8, aktywna stanem niskim;
- BOOT/konfiguracja: GPIO9;
- USB CDC przy starcie: włączone.

Piny można zmienić w konfiguracji WWW. GPIO ESP32-C3 pracują z logiką 3,3 V
i nie są odporne na 5 V. ESP zastępuje oryginalny moduł Wi-Fi klimatyzatora.

## Gotowe pliki BIN

Pliki są w katalogu `firmware`:

- `*-factory.bin` — pierwsze wgranie pod adres `0x0`;
- `*-app.bin` — aktualizacja przez stronę WWW albo po kablu pod adres `0x10000`.

Nie wgrywaj `app.bin` do pustej płytki. Sumy kontrolne są w
`SHA256SUMS.txt`.

## Arduino IDE

1. Zainstaluj obsługę ESP32 w wersji 3.3.11.
2. Skopiuj katalogi `Haier2Supla`, `HaierProtocol` i `SuplaDevice` do katalogu
   bibliotek Arduino.
3. Otwórz `Haier2Supla/examples/Haier2Supla/Haier2Supla.ino`.
4. Wybierz `ESP32C3 Dev Module`, flash 4 MB, `USB CDC On Boot: Enabled` oraz
   partycję `Minimal SPIFFS (1.9MB APP with OTA)`.
5. Skompiluj i wgraj szkic.

## PlatformIO

```text
python -m platformio run -e esp32-c3-super-mini
```

## Testy

Kompilacja `0.9.0` zakończyła się poprawnie. hOn był sprawdzony na
Haier Tundra Plus AS35TADHRA-CL: wykrywanie protokołu, CRC, temperatura, stan,
sterowanie i rejestracja w SUPLA działają.

SmartAir2 pozostał w kodzie, ale tej wersji nie sprawdzałem ponownie na
fizycznym urządzeniu SmartAir2.

## Autorzy i licencje

Projekt bazowy: [piglet0/haier2supla](https://github.com/piglet0/haier2supla).
Komunikacja Haier korzysta z projektu
[paveldn/HaierProtocol](https://github.com/paveldn/HaierProtocol), a obsługa
SUPLA z [SUPLA/supla-device](https://github.com/SUPLA/supla-device).

Licencja głównego projektu: GPL-3.0. Licencje zależności znajdują się w ich
katalogach.
