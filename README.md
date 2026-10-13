markdown
# ESP32 Pip-Boy OLED Watch

Наручные часы в стиле Pip-Boy из Fallout на базе ESP32 и OLED 0.96" 128x64.

![status](https://img.shields.io/badge/status-active-brightgreen)
![platform](https://img.shields.io/badge/platform-ESP32-blue)
![license](https://img.shields.io/badge/license-MIT-green)

## ✨ Возможности

- **CLOCK** — большие часы, день недели, дата, Vault Boy 64×64 справа
- **STATS** — регион, уровень радиации со спиннером, погода на 3 дня (утро/день/ночь)
- **MAP** — карта Иркутской области: Байкал, Ангара, Иркутск, Ангарск, бегущая строка
- **SETT** — 9 пунктов настроек с прокруткой:
  - TIME (WIFI / MANUAL)
  - BRIGHT (яркость OLED)
  - CITY (Irkutsk / Angarsk / Bratsk / Ust-Kut)
  - PLAYER (HP / CAPS / LVL)
  - TIMEOUT (OFF / 5s / 15s / 30s / 60s)
  - INVERT, FLIP, RESET, INFO
- **Wi-Fi провижининг** — при старте подключается к домашней сети, по K3 в SETT ищет iPhone для синхронизации
- **Light Sleep** — экран гаснет по таймауту, ESP32 уходит в лёгкий сон, просыпается любой кнопкой
- **Preferences (NVS)** — все настройки сохраняются между перезагрузками
- **Погода** — через OpenWeatherMap API (бесплатный ключ)
- **NTP** — синхронизация времени через pool.ntp.org

## 🔧 Железо

| Компонент | Описание |
| :--- | :--- |
| Микроконтроллер | ESP32 (WROOM-32) или ESP32-C3 Super Mini |
| Экран | OLED 0.96" 128×64, SSD1306, I2C |
| Кнопки | 4 тактовые кнопки (встроены в модуль экрана) |
| Питание | Li-Po 3.7V ~350 mAh + модуль зарядки TP4056 |
| Корпус | 3D-печать (см. `docs/`) |

## 📐 Схема подключения

| Экран | ESP32 |
| :--- | :--- |
| GND | GND |
| VCC | 3.3V |
| SCL | GPIO 22 |
| SDA | GPIO 21 |
| K1 (верх) | GPIO 32 |
| K2 (вниз) | GPIO 33 |
| K3 (#) | GPIO 25 |
| K4 (*) | GPIO 26 |

Подробнее: [docs/WIRING.md](docs/WIRING.md)

## 🚀 Установка

### 1. Клонируйте репозиторий
git clone https://github.com/209perfomance/esp32-pipboy-oled.git

### 2. Откройте прошивку в Arduino IDE
text
firmware/PipBoy-Watch/PipBoy-Watch.ino

### 3. Установите библиотеки
U8g2 by Oliver Kraus

ArduinoJson by Benoit Blanchon

### 4. Заполните config.h
cpp
const char* HOME_SSID = "ВашаСеть";
const char* HOME_PASS = "ВашПароль";
const char* OWM_API_KEY = "ВашКлючOpenWeatherMap";
Бесплатный ключ: https://openweathermap.org/api

### 5. Прошейте ESP32
Плата: ESP32 Dev Module или ESP32-C3 Dev Module.

🎮 Управление
Экран	K1	K2	K3	K4 (короткое)	K4 (долгое)
CLOCK / STATS / MAP	← вкладка	→ вкладка	—	—	на CLOCK
SETT (навигация)	← вкладка	→ вкладка	войти	—	на CLOCK
SETT (внутри)	↑	↓	действие	выйти	на CLOCK
TIME SOURCE	выбрать	выбрать	OK	назад	на CLOCK
TIME EDIT	+1	−1	след. поле	назад	на CLOCK
PROVISION	—	—	стоп	стоп	на CLOCK
📁 Структура
text
firmware/PipBoy-Watch/
├── PipBoy-Watch.ino    главный скетч
├── config.h            пины, Wi-Fi, API-ключи
├── vaultboy.h          XBM-спрайт Vault Boy 64×64
├── power.h             Light Sleep, screen timeout
├── ui.h                общие элементы UI
├── weather.h           погода OpenWeatherMap
└── settings.h          Preferences + экраны SETT
🗺️ Roadmap
☑ Базовая навигация и экран CLOCK
☑ Vault Boy спрайт 64×64
☑ Погода на 3 дня (OpenWeatherMap)
☑ SETT с Preferences
☑ Light Sleep + screen timeout
□ Реальное измерение батареи (делитель на GPIO 34)
□ Редактор имени игрока
□ Будильник
□ OTA обновление по Wi-Fi
□ Редактор радиации (реальный датчик)
□ Deep Sleep с RTC-кнопкой
📜 Лицензия
MIT License — см. LICENSE

🙏 Благодарности
U8g2 — Oliver Kraus

ArduinoJson — Benoit Blanchon

OpenWeatherMap — бесплатный API погоды

Вдохновлено играми Fallout (Bethesda)

Автор: @209perfomance
