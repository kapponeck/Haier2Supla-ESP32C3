Cześć,

przygotowałem swoją wersję projektu Haier2Supla od @piglet0:

https://github.com/kapponeck/Haier2Supla-ESP32C3

Najważniejsza zmiana to obsługa nowszego protokołu hOn. Zostawiłem też
SmartAir2, a firmware sam rozpoznaje protokół przy starcie.

Wersję przygotowałem dla Tenstar ROBOT ESP32-C3 Super Mini 4 MB. Domyślne
piny to RX GPIO20, TX GPIO21, LED GPIO8 i BOOT GPIO9.

Zmieniłem też sterowanie w SUPLA:

- Heat, Cool, Auto, włączanie i temperatura są w kanale HVAC;
- Dry i Fan Only są osobnymi kanałami;
- prędkość wentylatora jest na jednym suwaku: Auto, Low, Mid, High;
- swing pionowy i poziomy mają osobne przełączniki;
- stan w SUPLA aktualizuje się również po użyciu oryginalnego pilota;
- po starcie najpierw odczytywany jest stan klimatyzatora, więc SUPLA nie
  nadpisuje go wartościami domyślnymi.

Dodatkowo poprawiłem synchronizację komend i problem z blokowaniem ESP32-C3
przez logowanie USB bez otwartego terminala.

hOn sprawdziłem na Haier Tundra Plus AS35TADHRA-CL. Działa wykrywanie
protokołu, CRC, temperatura, odczyt stanu, sterowanie i rejestracja w SUPLA.
SmartAir2 pozostał obsługiwany w kodzie, ale tej wersji nie testowałem ponownie
na fizycznym urządzeniu SmartAir2.

W repozytorium jest kod do Arduino IDE i PlatformIO oraz gotowe pliki BIN:

- `factory.bin` do pierwszego wgrania od adresu `0x0`;
- `app.bin` do aktualizacji przez WWW lub od adresu `0x10000`.

Pełny opis, piny, instrukcja i sumy SHA-256 są w README.

Dzięki @piglet0 za bazowy projekt oraz paveldn i zespołowi SUPLA za biblioteki.
