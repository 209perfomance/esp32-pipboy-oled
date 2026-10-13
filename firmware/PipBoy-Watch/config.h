/* =========================================================
   config.h
   Все константы проекта: пины, Wi-Fi, API-ключи, города.
   Меняйте только этот файл при настройке под себя.
   ========================================================= */

#ifndef CONFIG_H
#define CONFIG_H

// =========================================================
// ПИНЫ (для классического ESP32 WROOM-32)
// =========================================================
#define PIN_SDA  21
#define PIN_SCL  22
#define PIN_K1   32   // Верхняя кнопка (^)
#define PIN_K2   33   // Вторая кнопка (v)
#define PIN_K3   25   // Третья кнопка (#)
#define PIN_K4   26   // Нижняя кнопка (*)

// Для ESP32-C3 Super Mini раскомментируйте эти пины:
// #define PIN_SDA  8
// #define PIN_SCL  9
// #define PIN_K1   3
// #define PIN_K2   4
// #define PIN_K3   5
// #define PIN_K4   10

// =========================================================
// WIFI
// =========================================================
const char* HOME_SSID = "TP-Link_FD14";
const char* HOME_PASS = "ВАШ_ПАРОЛЬ";

const char* PHONE_SSID = "iPhone";
const char* PHONE_PASS = "12345678";

const long  GMT_OFFSET = 8 * 3600;   // Иркутск UTC+8

// =========================================================
// OpenWeatherMap
// =========================================================
// Получить бесплатный ключ: https://home.openweathermap.org/api_keys
const char* OWM_API_KEY = "ВАШ_КЛЮЧ_OPENWEATHERMAP";

// =========================================================
// ГОРОДА
// =========================================================
// Города для выбора в SETT → CITY
const char* CITY_LIST[]  = {"Irkutsk", "Angarsk", "Bratsk", "Ust-Kut"};
const char* CITY_LABEL[] = {"IRKUTSK", "ANGARSK", "BRATSK", "UST-KUT"};
const int   CITY_COUNT   = 4;

// =========================================================
// ТАЙМАУТЫ
// =========================================================
const int TIMEOUT_OPTIONS[]   = {0, 5, 15, 30, 60};        // секунды (0 = OFF)
const char* TIMEOUT_LABELS[]  = {"OFF", "5s", "15s", "30s", "60s"};
const int TIMEOUT_COUNT       = 5;

const unsigned long WEATHER_INTERVAL   = 30UL * 60UL * 1000UL;  // 30 минут
const unsigned long LIGHT_SLEEP_DELAY  = 3000;                  // 3 сек после гашения
const unsigned long PROV_TIMEOUT_MS    = 120000;                // 2 мин поиска точки
const unsigned long PROV_SCAN_INTERVAL = 3000;                  // сканировать каждые 3 сек
const unsigned long DEBOUNCE_MS        = 180;                   // антидребезг кнопок
const unsigned long LONG_PRESS_MS      = 700;                   // долгое нажатие
const unsigned long TRANSITION_MS      = 80;                    // анимация перехода

// =========================================================
// БЕГУЩАЯ СТРОКА
// =========================================================
const char* MARQUEE_TEXT = "> SIGNAL: OK - SCANNING FOR VAULT FREQUENCIES... ";
const int   MARQUEE_LEN  = 51;

// =========================================================
// РАДИАЦИЯ (имитация)
// =========================================================
const char* RAD_SPINNER = "-\\|/-\\|/";

// =========================================================
// АТМОСФЕРНЫЕ СТРОКИ
// =========================================================
const char* PIP_LINES[] = {
  "CAPS: 1337",
  "HP: 95/100",
  "LVL: 12",
  "STIMPAK: 3",
  "RADAWAY: 2",
  "WATER: 5",
  "FOOD: 8",
  "VAULT 111"
};
const int PIP_LINES_COUNT = 8;

#endif // CONFIG_H
//📌 Что здесь важно
//HOME_SSID / HOME_PASS — впишите свою домашнюю сеть.

//OWM_API_KEY — впишите свой ключ OpenWeatherMap. Не оставляйте заглушку!

//PIN_SDA / PIN_SCL — если у вас ESP32-C3 Super Mini, раскомментируйте нижний блок и закомментируйте верхний.

//CITY_LIST — можно расширить, добавив свои города. Главное — чтобы количество совпадало с CITY_COUNT.
