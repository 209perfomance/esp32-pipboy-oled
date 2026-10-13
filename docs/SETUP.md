# Настройка Arduino IDE

Пошаговая инструкция для новичков. Если уже умеете — пропускайте к разделу «Прошивка».

## 1. Установка Arduino IDE

Скачайте **Arduino IDE 2.x** с официального сайта:

👉 https://www.arduino.cc/en/software

Установите стандартным способом (Windows / macOS / Linux).

## 2. Поддержка ESP32

По умолчанию Arduino IDE **не умеет** работать с ESP32. Нужно добавить поддержку.

### Способ А — через менеджер плат (проще)

1. Откройте Arduino IDE.
2. **Файл → Настройки** (или `Ctrl + ,`).
3. В поле **«Дополнительные ссылки для менеджера плат»** вставьте:
https://espressif.github.io/arduino-esp32/package_esp32_index.json

text
4. Нажмите **ОК**.
5. Откройте **Инструменты → Плата → Менеджер плат**.
6. В поиске введите: `esp32`.
7. Найдите **"esp32 by Espressif Systems"** и нажмите **Установить**.
8. Дождитесь окончания (может занять 5–10 минут).

### Способ Б — если способ А не работает

Иногда GitHub-репозиторий Espressif заблокирован в РФ. Тогда:

1. Скачайте архив **esp32-arduino** вручную с [зеркала](https://github.com/espressif/arduino-esp32/releases).
2. Распакуйте в:
C:\Users<ВашеИмя>\AppData\Local\Arduino15\packages\

text
3. Перезапустите Arduino IDE.

## 3. Установка библиотек

Проект использует **две библиотеки**:

| Библиотека | Автор | Зачем |
| :--- | :--- | :--- |
| **U8g2** | Oliver Kraus | Работа с OLED SSD1306 |
| **ArduinoJson** | Benoit Blanchon | Парсинг JSON от OpenWeatherMap |

### Установка через менеджер

1. **Инструменты → Управлять библиотеками** (или `Ctrl + Shift + I`).
2. В поиске: `U8g2`.
3. Найдите **"U8g2" by oliver** → **Установить**.
4. В поиске: `ArduinoJson`.
5. Найдите **"ArduinoJson" by Benoit Blanchon** → **Установить**.

> ⚠️ **Важно про U8g2:** имя файла заголовка — **`U8g2lib.h`** (маленькая `g`). Если в коде написано `U8G2lib.h` — компилятор не найдёт файл. Следите за регистром!

## 4. Получение ключа OpenWeatherMap

1. Зарегистрируйтесь: https://home.openweathermap.org/users/sign_up
2. Подтвердите email.
3. Перейдите: https://home.openweathermap.org/api_keys
4. Скопируйте ваш **API key** (строка из 32 символов).
5. **Важно:** ключ активируется в течение **10 минут — 2 часов**. Если сразу не работает — подождите.

Бесплатный тариф: **60 запросов/минуту, 1 000 000 запросов/месяц** — с запасом хватит.

## 5. Настройка config.h

Откройте `firmware/PipBoy-Watch/config.h` и заполните:

// Домашняя сеть Wi-Fi (2.4 ГГц!)
const char* HOME_SSID = "ВашаСеть";
const char* HOME_PASS = "ВашПароль";

// Точка доступа iPhone (для провижининга)
const char* PHONE_SSID = "iPhone";
const char* PHONE_PASS = "12345678";

// Ключ OpenWeatherMap
const char* OWM_API_KEY = "ваш_ключ_сюда";
⚠️ ESP32 работает только с 2.4 ГГц. Если роутер раздаёт 5 ГГц — подключитесь к сети без приставки _5G.

## 6. Выбор платы
Инструменты → Плата → esp32 → ESP32 Dev Module (для WROOM-32).

Для ESP32-C3:
Инструменты → Плата → esp32 → ESP32C3 Dev Module.

Настройки:
Параметр	Значение
Board	ESP32 Dev Module
Upload Speed	921600
CPU Frequency	240 MHz
Flash Frequency	80 MHz
Flash Mode	QIO
Flash Size	4 MB
Partition Scheme	Default 4MB
PSRAM	Disabled
## 7. Прошивка
Подключите ESP32 к компьютеру через USB.

Инструменты → Порт — выберите появившийся COM-порт (Windows) или /dev/ttyUSB0 (Linux).

Нажмите кнопку «Загрузить» (стрелка вправо) или Ctrl + U.

Дождитесь надписи Done uploading.

Если не загружается
Проблема	Решение
Failed to connect to ESP32	Зажмите кнопку BOOT на плате, нажмите EN/RST, отпустите EN, потом BOOT.
Порт не найден	Установите драйверы CP2102 или CH340 (зависит от платы).
A fatal error occurred: Timed out	Понизьте Upload Speed до 115200.
## 8. Проверка
Откройте Сервис → Монитор порта (или Ctrl + Shift + M).

Установите скорость 115200.

Нажмите кнопку EN/RST на ESP32.

Должно появиться:

text
=== PIP-BOY 3000 MK IV ===
[Prefs] inv=0 flip=0 bright=255 city=0 timeout=3
[WiFi] Connecting to TP-Link_FD14.....
[WiFi] OK, IP=192.168.0.101
[Weather] Fetching OWM...
[Weather] Got 18542 bytes
[Weather] Day 0: M=-2 D=3 N=-4
=== READY ===
На экране — загрузка в стиле ROBCO, затем экран CLOCK с текущим временем.

## 9. Отладка
Экран не светится
Проверьте питание (3.3V на VCC).

Проверьте SDA/SCL (поменяйте местами).

Откройте [I2C] Scanning bus... — должен быть адрес 0x3C.

Время не синхронизируется
Wi-Fi подключён?

NTP: pool.ntp.org доступен?

Отключите VPN и попробуйте снова.

Погода не грузится
Ключ OpenWeatherMap активирован? (может занять до 2 часов)

В Serial Monitor будет [Weather] HTTP error: 401 — ключ ещё не активен.

HTTP error: -1 — Wi-Fi отвалился.

Light Sleep не просыпается
Проверьте, что в setup() есть pinMode(..., INPUT_PULLUP) для всех кнопок.

Если не помогает — увеличьте LIGHT_SLEEP_DELAY до 5000 мс.

## 10. Что дальше
Прочитайте docs/WIRING.md для сборки корпуса.

Настройте SETT → TIME → WIFI, чтобы синхронизировать время через iPhone.

Настройте SETT → BRIGHT для комфортной яркости.

Экспериментируйте с SETT → PLAYER для HP / CAPS / LVL.

Удачи! Если что-то не получается — создайте Issue на GitHub.
