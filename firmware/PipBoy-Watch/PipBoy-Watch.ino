/* =========================================================
   PipBoy-Watch.ino
   Главный скетч проекта ESP32 Pip-Boy OLED Watch.
   ========================================================= */

#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include "config.h"
#include "vaultboy.h"
#include "ui.h"
#include "power.h"
#include "weather.h"
#include "settings.h"

// =========================================================
// ДИСПЛЕЙ
// =========================================================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);

// =========================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// =========================================================
// Настройки (сохраняются в NVS через Preferences)
bool    invertDisplay = false;
bool    flipDisplay   = false;
uint8_t brightness    = 255;
int     timeoutIdx    = 3;
int     cityIdx       = 0;

TimeSource timeSource = TIME_WIFI;

int manualDay = 13, manualMonth = 10, manualYear = 2026;
int manualHour = 14, manualMinute = 35;

char playerName[16] = "WANDERER";
int  playerHP   = 95;
int  playerCaps = 1337;
int  playerLVL  = 12;

// SETT-меню
const char* SETT_ITEMS[] = {"TIME", "BRIGHT", "CITY", "PLAYER", "TIMEOUT", "INVERT", "FLIP", "RESET", "INFO"};
const int SETT_COUNT   = 9;
const int SETT_VISIBLE = 4;
int settCursor = 0;
int settScroll = 0;

// PLAYER
int playerField = 0;
const int PLAYER_FIELDS = 4;

// TIME EDIT
int timeEditField = 0;
const int TIME_FIELDS = 6;
int timeSourceCursor = 0;

// Экраны
enum Screen {
  SCREEN_STAT, SCREEN_DATA, SCREEN_MAP,
  SCREEN_SETTINGS, SCREEN_PROVISION, SCREEN_TIME_SOURCE, SCREEN_TIME_EDIT,
  SCREEN_SETT_BRIGHT, SCREEN_SETT_CITY, SCREEN_SETT_PLAYER,
  SCREEN_SETT_INVERT, SCREEN_SETT_FLIP, SCREEN_SETT_RESET, SCREEN_SETT_INFO,
  SCREEN_SETT_TIMEOUT
};
Screen currentScreen = SCREEN_STAT;

int SCREEN_SETT_BRIGHT_ID = SCREEN_SETT_BRIGHT;
int SCREEN_SETT_CITY_ID   = SCREEN_SETT_CITY;
int SCREEN_SETT_PLAYER_ID = SCREEN_SETT_PLAYER;
int SCREEN_SETT_TIMEOUT_ID = SCREEN_SETT_TIMEOUT;
int SCREEN_SETT_INVERT_ID = SCREEN_SETT_INVERT;
int SCREEN_SETT_FLIP_ID   = SCREEN_SETT_FLIP;
int SCREEN_SETT_RESET_ID  = SCREEN_SETT_RESET;
int SCREEN_SETT_INFO_ID   = SCREEN_SETT_INFO;
int SCREEN_TIME_SOURCE_ID = SCREEN_TIME_SOURCE;
int SCREEN_TIME_EDIT_ID   = SCREEN_TIME_EDIT;

// Погода
DayForecast forecast[2];
bool weatherLoaded = false;
unsigned long lastWeatherUpdate = 0;

// Батарея
int batteryPercent = 89;

// Радиация
uint8_t radLevel = 2;

// Бегущая строка
unsigned long marqueeStart = 0;

// Кнопки
struct Button {
  uint8_t pin;
  bool lastState;
  unsigned long lastPress;
  unsigned long pressStart;
  bool longFired;
};
Button buttons[4] = {
  {PIN_K1, HIGH, 0, 0, false}, {PIN_K2, HIGH, 0, 0, false},
  {PIN_K3, HIGH, 0, 0, false}, {PIN_K4, HIGH, 0, 0, false}
};
ButtonState buttonStates[4];  // для power.h

// Анимация перехода
bool transitionActive = false;
unsigned long transitionStart = 0;

// Screen timeout / Light Sleep
unsigned long lastActivity = 0;
bool screenIsOff = false;
bool inLightSleep = false;
unsigned long screenOffAt = 0;

// Провижининг
enum ProvState {
  PROV_SCANNING, PROV_CONNECTING, PROV_SYNCING_TIME,
  PROV_FETCHING_WEATHER, PROV_DONE, PROV_TIMEOUT, PROV_ABORTED
};
ProvState provState = PROV_SCANNING;
unsigned long provStart = 0;
unsigned long lastScanAttempt = 0;

// =========================================================
// ПРОТОТИПЫ
// =========================================================
void drawStat();
void drawProvisioning();
void drawTimeSource();
void drawTimeEdit();
void drawCurrentScreen();
void handleButtons();
void onButtonPress(int btn);
void onLongPress(int btn);
void startTransition();
void startProvisioning();
void handleProvisioning();
void runBootSequence(unsigned long duration);
void drawBootFrame(unsigned long elapsed, unsigned long total);
int  readBatteryPercent();

// =========================================================
// SETUP
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n=== PIP-BOY 3000 MK IV ===");

  loadPrefs();
  Serial.printf("[Prefs] inv=%d flip=%d bright=%d city=%d timeout=%d\n",
                invertDisplay, flipDisplay, brightness, cityIdx, timeoutIdx);

  Wire.begin(PIN_SDA, PIN_SCL);
  u8g2.begin();
  u8g2.setContrast(brightness);
  u8g2.setBusClock(400000);
  u8g2.setDisplayRotation(flipDisplay ? U8G2_R2 : U8G2_R0);

  for (int i = 0; i < 4; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttonStates[i].pin = buttons[i].pin;
    buttonStates[i].lastState = HIGH;
    buttonStates[i].longFired = false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(HOME_SSID, HOME_PASS);
  configTime(GMT_OFFSET, 0, "pool.ntp.org", "time.nist.gov");

  runBootSequence(4000);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) delay(200);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] OK, IP=%s\n", WiFi.localIP().toString().c_str());
    fetchWeather();
    lastWeatherUpdate = millis();
  } else {
    Serial.println("[WiFi] Home network unavailable");
  }

  marqueeStart = millis();
  lastActivity = millis();
  currentScreen = SCREEN_STAT;
  if (invertDisplay) applyInvert();
  Serial.println("=== READY ===");
}

// =========================================================
// LOOP
// =========================================================
void loop() {
  handleButtons();
  checkScreenTimeout();

  if (currentScreen != SCREEN_PROVISION &&
      WiFi.status() == WL_CONNECTED &&
      millis() - lastWeatherUpdate > WEATHER_INTERVAL) {
    fetchWeather();
    lastWeatherUpdate = millis();
  }

  if (currentScreen == SCREEN_PROVISION) handleProvisioning();

  if (!screenIsOff && !inLightSleep) drawCurrentScreen();
  delay(30);
}

// =========================================================
// КНОПКИ
// =========================================================
void handleButtons() {
  if (inLightSleep) return;

  for (int i = 0; i < 4; i++) {
    bool state = digitalRead(buttons[i].pin);

    // Нажатие
    if (state == LOW && buttons[i].lastState == HIGH) {
      if (millis() - buttons[i].lastPress > DEBOUNCE_MS) {
        buttons[i].lastPress = millis();
        buttons[i].pressStart = millis();
        buttons[i].longFired = false;

        if (screenIsOff) {
          screenOn();
          buttons[i].lastState = state;
          return;
        }
        lastActivity = millis();
      }
    }

    // Долгое нажатие
    if (state == LOW && !buttons[i].longFired &&
        millis() - buttons[i].pressStart > LONG_PRESS_MS) {
      buttons[i].longFired = true;
      if (i == 3) onLongPress(i);
    }

    // Отпускание
    if (state == HIGH && buttons[i].lastState == LOW) {
      if (!buttons[i].longFired) onButtonPress(i);
    }

    buttons[i].lastState = state;
  }
}

void onLongPress(int btn) {
  if (btn == 3) {
    if (currentScreen != SCREEN_STAT) {
      currentScreen = SCREEN_STAT;
      startTransition();
    }
    Serial.println("[BTN] K4 long -> CLOCK");
  }
}

void onButtonPress(int btn) {
  lastActivity = millis();

  // ============ ПРОВИЖИНИНГ ============
  if (currentScreen == SCREEN_PROVISION) {
    if (btn == 2 || btn == 3) {
      WiFi.disconnect(true);
      provState = PROV_ABORTED;
      currentScreen = SCREEN_SETTINGS;
      startTransition();
    }
    return;
  }

  // ============ TIME SOURCE ============
  if (currentScreen == SCREEN_TIME_SOURCE) {
    if (btn == 0 || btn == 1) timeSourceCursor = (timeSourceCursor + 1) % 2;
    if (btn == 2) {
      if (timeSourceCursor == 0) {
        timeSource = TIME_WIFI;
        startProvisioning();
        currentScreen = SCREEN_PROVISION;
      } else {
        timeSource = TIME_MANUAL;
        struct tm ti;
        if (getLocalTime(&ti, 5)) {
          manualDay    = ti.tm_mday;
          manualMonth  = ti.tm_mon + 1;
          manualYear   = ti.tm_year + 1900;
          manualHour   = ti.tm_hour;
          manualMinute = ti.tm_min;
        }
        timeEditField = 0;
        currentScreen = SCREEN_TIME_EDIT;
      }
      startTransition();
    }
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ TIME EDIT ============
  if (currentScreen == SCREEN_TIME_EDIT) {
    if (btn == 0) {
      switch (timeEditField) {
        case 0: manualDay    = (manualDay    % 31) + 1; break;
        case 1: manualMonth  = (manualMonth  % 12) + 1; break;
        case 2: manualYear++;                            break;
        case 3: manualHour   = (manualHour   + 1) % 24;  break;
        case 4: manualMinute = (manualMinute + 1) % 60;  break;
      }
    }
    if (btn == 1) {
      switch (timeEditField) {
        case 0: manualDay    = (manualDay    - 2 + 31) % 31 + 1; break;
        case 1: manualMonth  = (manualMonth  - 2 + 12) % 12 + 1; break;
        case 2: manualYear--;                                     break;
        case 3: manualHour   = (manualHour   - 1 + 24) % 24;     break;
        case 4: manualMinute = (manualMinute - 1 + 60) % 60;     break;
      }
    }
    if (btn == 2) {
      timeEditField = (timeEditField + 1) % TIME_FIELDS;
      if (timeEditField == 5) {
        struct tm ti = {};
        ti.tm_mday = manualDay;
        ti.tm_mon  = manualMonth - 1;
        ti.tm_year = manualYear - 1900;
        ti.tm_hour = manualHour;
        ti.tm_min  = manualMinute;
        time_t t = mktime(&ti);
        struct timeval tv = { t, 0 };
        settimeofday(&tv, nullptr);
        timeSource = TIME_MANUAL;
        savePrefs();
        currentScreen = SCREEN_SETTINGS;
        startTransition();
      }
    }
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-BRIGHT ============
  if (currentScreen == SCREEN_SETT_BRIGHT) {
    if (btn == 0) brightness = (brightness < 245) ? brightness + 10 : 255;
    if (btn == 1) brightness = (brightness > 15)  ? brightness - 10 : 0;
    if (btn == 2) { u8g2.setContrast(brightness); savePrefs(); }
    if (btn == 3) { u8g2.setContrast(brightness); savePrefs(); currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-CITY ============
  if (currentScreen == SCREEN_SETT_CITY) {
    if (btn == 0) cityIdx = (cityIdx + 1) % CITY_COUNT;
    if (btn == 1) cityIdx = (cityIdx - 1 + CITY_COUNT) % CITY_COUNT;
    if (btn == 2) { savePrefs(); fetchWeather(); lastWeatherUpdate = millis(); }
    if (btn == 3) { savePrefs(); currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-PLAYER ============
  if (currentScreen == SCREEN_SETT_PLAYER) {
    if (btn == 0) {
      switch (playerField) {
        case 0: playerHP   = playerHP + 5;   if (playerHP > 100) playerHP = 100; break;
        case 1: playerCaps = playerCaps + 10; break;
        case 2: playerLVL  = playerLVL + 1;   if (playerLVL > 99) playerLVL = 99; break;
      }
    }
    if (btn == 1) {
      switch (playerField) {
        case 0: playerHP   = playerHP - 5;   if (playerHP < 0) playerHP = 0; break;
        case 1: playerCaps = playerCaps - 10; if (playerCaps < 0) playerCaps = 0; break;
        case 2: playerLVL  = playerLVL - 1;   if (playerLVL < 1) playerLVL = 1; break;
      }
    }
    if (btn == 2) {
      playerField++;
      if (playerField >= PLAYER_FIELDS) {
        savePrefs();
        currentScreen = SCREEN_SETTINGS;
        startTransition();
      }
    }
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-TIMEOUT ============
  if (currentScreen == SCREEN_SETT_TIMEOUT) {
    if (btn == 0) timeoutIdx = (timeoutIdx + 1) % TIMEOUT_COUNT;
    if (btn == 1) timeoutIdx = (timeoutIdx - 1 + TIMEOUT_COUNT) % TIMEOUT_COUNT;
    if (btn == 2) savePrefs();
    if (btn == 3) { savePrefs(); currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-INVERT ============
  if (currentScreen == SCREEN_SETT_INVERT) {
    if (btn == 2 || btn == 0 || btn == 1) { invertDisplay = !invertDisplay; applyInvert(); savePrefs(); }
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-FLIP ============
  if (currentScreen == SCREEN_SETT_FLIP) {
    if (btn == 2 || btn == 0 || btn == 1) { flipDisplay = !flipDisplay; applyFlip(); savePrefs(); }
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-RESET ============
  if (currentScreen == SCREEN_SETT_RESET) {
    if (btn == 2) {
      prefs.begin("pipboy", false);
      prefs.clear();
      prefs.end();
      ESP.restart();
    }
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETT-INFO ============
  if (currentScreen == SCREEN_SETT_INFO) {
    if (btn == 3) { currentScreen = SCREEN_SETTINGS; startTransition(); }
    return;
  }

  // ============ SETTINGS (главный список) ============
  static bool settInside = false;
  if (currentScreen != SCREEN_SETTINGS) settInside = false;

  if (currentScreen == SCREEN_SETTINGS && settInside) {
    if (btn == 0) {
      settCursor--;
      if (settCursor < 0) settCursor = SETT_COUNT - 1;
      if (settCursor < settScroll) settScroll = settCursor;
      if (settCursor >= settScroll + SETT_VISIBLE) settScroll = settCursor - SETT_VISIBLE + 1;
      return;
    }
    if (btn == 1) {
      settCursor++;
      if (settCursor >= SETT_COUNT) settCursor = 0;
      if (settCursor < settScroll) settScroll = settCursor;
      if (settCursor >= settScroll + SETT_VISIBLE) settScroll = settCursor - SETT_VISIBLE + 1;
      return;
    }
    if (btn == 2) {
      switch (settCursor) {
        case 0: timeSourceCursor = (timeSource == TIME_WIFI) ? 0 : 1; currentScreen = SCREEN_TIME_SOURCE; break;
        case 1: currentScreen = SCREEN_SETT_BRIGHT;  break;
        case 2: currentScreen = SCREEN_SETT_CITY;    break;
        case 3: playerField = 0; currentScreen = SCREEN_SETT_PLAYER; break;
        case 4: currentScreen = SCREEN_SETT_TIMEOUT; break;
        case 5: currentScreen = SCREEN_SETT_INVERT;  break;
        case 6: currentScreen = SCREEN_SETT_FLIP;    break;
        case 7: currentScreen = SCREEN_SETT_RESET;   break;
        case 8: currentScreen = SCREEN_SETT_INFO;    break;
      }
      startTransition();
      return;
    }
    if (btn == 3) { settInside = false; return; }
    return;
  }

  // ============ ОСНОВНАЯ НАВИГАЦИЯ ============
  if (btn == 0) {
    int s = (int)currentScreen - 1;
    if (s < SCREEN_STAT) s = SCREEN_SETTINGS;
    currentScreen = (Screen)s;
    startTransition();
    return;
  }
  if (btn == 1) {
    int s = (int)currentScreen + 1;
    if (s > SCREEN_SETTINGS) s = SCREEN_STAT;
    currentScreen = (Screen)s;
    startTransition();
    return;
  }
  if (btn == 2) {
    if (currentScreen == SCREEN_SETTINGS) {
      settInside = true;
      settCursor = 0;
      settScroll = 0;
    }
    return;
  }
  if (btn == 3) {
    if (currentScreen != SCREEN_STAT) {
      currentScreen = SCREEN_STAT;
      startTransition();
    }
    return;
  }
}

void startTransition() {
  transitionActive = true;
  transitionStart = millis();
}

// =========================================================
// ОТРИСОВКА
// =========================================================
void drawCurrentScreen() {
  if (transitionActive) {
    if (millis() - transitionStart < TRANSITION_MS) {
      u8g2.clearBuffer();
      u8g2.drawBox(0, 0, 128, 64);
      u8g2.sendBuffer();
      return;
    }
    transitionActive = false;
  }

  u8g2.clearBuffer();
  switch (currentScreen) {
    case SCREEN_STAT:           drawStat();            break;
    case SCREEN_DATA:           drawData();            break;
    case SCREEN_MAP:            drawMap();             break;
    case SCREEN_SETTINGS:       drawSettings();        break;
    case SCREEN_PROVISION:      drawProvisioning();    break;
    case SCREEN_TIME_SOURCE:    drawTimeSource();      break;
    case SCREEN_TIME_EDIT:      drawTimeEdit();        break;
    case SCREEN_SETT_BRIGHT:    drawSettBright();      break;
    case SCREEN_SETT_CITY:      drawSettCity();        break;
    case SCREEN_SETT_PLAYER:    drawSettPlayer();      break;
    case SCREEN_SETT_TIMEOUT:   drawSettTimeout();     break;
    case SCREEN_SETT_INVERT:    drawSettInvert();      break;
    case SCREEN_SETT_FLIP:      drawSettFlip();        break;
    case SCREEN_SETT_RESET:     drawSettReset();       break;
    case SCREEN_SETT_INFO:      drawSettInfo();        break;
  }
  u8g2.sendBuffer();
}

// =========================================================
// STAT — часы
// =========================================================
void drawStat() {
  struct tm ti;
  bool timeOk = getLocalTime(&ti, 5);
  drawTopMenu(0);
  drawVaultBoyScaled(70, 6, 64);

  char hh[4], mm[4];
  if (timeOk) {
    strftime(hh, sizeof(hh), "%H", &ti);
    strftime(mm, sizeof(mm), "%M", &ti);
  } else {
    strcpy(hh, "--");
    strcpy(mm, "--");
  }

  u8g2.setFont(u8g2_font_logisoso20_tn);
  const int CLOCK_CX = 44;
  int hw = u8g2.getStrWidth(hh);
  u8g2.drawStr(CLOCK_CX - hw / 2, 33, hh);
  int mw = u8g2.getStrWidth(mm);
  u8g2.drawStr(CLOCK_CX - mw / 2, 55, mm);

  const int DND_CX = 14;
  const char* wdays[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
  const char* day = timeOk ? wdays[ti.tm_wday] : "---";
  char dstr[4];
  if (timeOk) snprintf(dstr, sizeof(dstr), "%02d", ti.tm_mday);
  else        strcpy(dstr, "--");

  u8g2.setFont(u8g2_font_5x7_tf);
  int dw = u8g2.getStrWidth(day);
  int box_w = dw + 6, box_h = 10;
  int box_x = DND_CX - box_w / 2;
  int box_y = 24;

  u8g2.drawBox(box_x, box_y, box_w, box_h);
  u8g2.setDrawColor(0);
  u8g2.drawStr(box_x + 3, box_y + 8, day);
  u8g2.setDrawColor(1);

  int ddw = u8g2.getStrWidth(dstr);
  u8g2.drawStr(DND_CX - ddw / 2, 42, dstr);

  batteryPercent = readBatteryPercent();
  drawBatteryBar();
}

// =========================================================
// ПРОВИЖИНИНГ
// =========================================================
void startProvisioning() {
  Serial.println("[Prov] Starting...");
  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_STA);
  provState = PROV_SCANNING;
  provStart = millis();
  lastScanAttempt = 0;
}

void handleProvisioning() {
  unsigned long elapsed = millis() - provStart;

  switch (provState) {
    case PROV_SCANNING:
      if (millis() - lastScanAttempt > PROV_SCAN_INTERVAL) {
        lastScanAttempt = millis();
        Serial.println("[Prov] Scanning...");
        int n = WiFi.scanNetworks();

        Serial.printf("[Prov] Found %d networks:\n", n);
        bool found = false;
        for (int i = 0; i < n; i++) {
          Serial.printf("  - '%s' (RSSI %d)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
          if (WiFi.SSID(i) == PHONE_SSID) found = true;
        }

        if (found) {
          Serial.println("[Prov] Connecting to iPhone...");
          WiFi.begin(PHONE_SSID, PHONE_PASS);
          provState = PROV_CONNECTING;
          provStart = millis();
        } else if (elapsed > PROV_TIMEOUT_MS) {
          provState = PROV_TIMEOUT;
        }
      }
      break;

    case PROV_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[Prov] Connected!");
        configTime(GMT_OFFSET, 0, "pool.ntp.org", "time.nist.gov");
        provState = PROV_SYNCING_TIME;
        provStart = millis();
      } else if (millis() - provStart > 15000) {
        Serial.println("[Prov] Connect failed, rescanning");
        WiFi.disconnect(true);
        provState = PROV_SCANNING;
        provStart = millis();
        lastScanAttempt = 0;
      }
      break;

    case PROV_SYNCING_TIME:
      if (millis() - provStart > 3000) {
        provState = PROV_FETCHING_WEATHER;
        provStart = millis();
      }
      break;

    case PROV_FETCHING_WEATHER:
      if (millis() - provStart > 500) {
        fetchWeather();
        lastWeatherUpdate = millis();
        provState = PROV_DONE;
        provStart = millis();
        Serial.println("[Prov] Done");
      }
      break;

    default:
      break;
  }
}

void drawProvisioning() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 8, "WIFI PROVISIONING");
  u8g2.drawHLine(0, 11, 128);
  u8g2.drawStr(4, 22, "Looking for:");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(4, 34, "iPhone");
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 44, "PASS: 12345678");

  int barX = 4, barY = 50, barW = 120, barH = 8;
  u8g2.drawFrame(barX, barY, barW, barH);

  int progress = 0;
  const char* status = "";
  switch (provState) {
    case PROV_SCANNING:         progress = 25;  status = "Scanning... K3=STOP";   break;
    case PROV_CONNECTING:       progress = 40;  status = "Connecting... K3=STOP"; break;
    case PROV_SYNCING_TIME:     progress = 65;  status = "Syncing time...";       break;
    case PROV_FETCHING_WEATHER: progress = 85;  status = "Fetching weather...";   break;
    case PROV_DONE:             progress = 100; status = "DONE! Turn off WiFi";   break;
    case PROV_TIMEOUT:          progress = 100; status = "TIMEOUT (K3/K4=exit)";  break;
    case PROV_ABORTED:          progress = 0;   status = "ABORTED";               break;
  }

  int fill = (progress * (barW - 2)) / 100;
  if (fill > 0) u8g2.drawBox(barX + 1, barY + 1, fill, barH - 2);
  u8g2.drawStr(4, 63, status);
}

// =========================================================
// TIME SOURCE / EDIT
// =========================================================
void drawTimeSource() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "TIME SOURCE");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  if (timeSourceCursor == 0) {
    u8g2.drawBox(4, 18, 120, 12);
    u8g2.setDrawColor(0); u8g2.drawStr(8, 27, "WIFI (iPhone)"); u8g2.setDrawColor(1);
  } else u8g2.drawStr(8, 27, "WIFI (iPhone)");

  if (timeSourceCursor == 1) {
    u8g2.drawBox(4, 34, 120, 12);
    u8g2.setDrawColor(0); u8g2.drawStr(8, 43, "MANUAL"); u8g2.setDrawColor(1);
  } else u8g2.drawStr(8, 43, "MANUAL");

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 58, "K1/K2=sel K3=ok K4=back");
}

void drawTimeEdit() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 8, "SET DATE/TIME");
  u8g2.drawHLine(0, 10, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  char line[24];

  snprintf(line, sizeof(line), "DAY:   %02d", manualDay);
  if (timeEditField == 0) u8g2.drawBox(0, 12, 128, 10);
  u8g2.setDrawColor(timeEditField == 0 ? 0 : 1);
  u8g2.drawStr(4, 20, line);
  u8g2.setDrawColor(1);

  snprintf(line, sizeof(line), "MONTH: %02d", manualMonth);
  if (timeEditField == 1) u8g2.drawBox(0, 23, 128, 10);
  u8g2.setDrawColor(timeEditField == 1 ? 0 : 1);
  u8g2.drawStr(4, 31, line);
  u8g2.setDrawColor(1);

  snprintf(line, sizeof(line), "YEAR:  %04d", manualYear);
  if (timeEditField == 2) u8g2.drawBox(0, 34, 128, 10);
  u8g2.setDrawColor(timeEditField == 2 ? 0 : 1);
  u8g2.drawStr(4, 42, line);
  u8g2.setDrawColor(1);

  snprintf(line, sizeof(line), "TIME:  %02d:%02d", manualHour, manualMinute);
  if (timeEditField == 3 || timeEditField == 4) u8g2.drawBox(0, 45, 128, 10);
  u8g2.setDrawColor((timeEditField == 3 || timeEditField == 4) ? 0 : 1);
  u8g2.drawStr(4, 53, line);
  u8g2.setDrawColor(1);

  if (timeEditField == 5) {
    u8g2.drawBox(70, 55, 58, 9);
    u8g2.setDrawColor(0);
    u8g2.drawStr(78, 62, "> SAVE <");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(78, 62, "SAVE");
  }
}

// =========================================================
// БАТАРЕЯ — заглушка
// =========================================================
int readBatteryPercent() {
  return 89;
}

// =========================================================
// ЗАГРУЗКА
// =========================================================
void runBootSequence(unsigned long duration) {
  unsigned long start = millis();
  while (millis() - start < duration) {
    drawBootFrame(millis() - start, duration);
    delay(40);
  }
}

void drawBootFrame(unsigned long elapsed, unsigned long total) {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  struct tm ti;
  bool timeOk = getLocalTime(&ti, 5);

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "ROBCO INDUSTRIES (TM)");
  u8g2.drawStr(4, 19, "TERMLINK PROTOCOL v7.1");
  u8g2.drawHLine(2, 22, 124);

  if (elapsed > 200)  u8g2.drawStr(4, 33, "> BOOT CORE........ OK");
  if (elapsed > 600)  u8g2.drawStr(4, 42, "> MEMORY 640K...... OK");
  if (elapsed > 1000) u8g2.drawStr(4, 51, wifiOk ? "> NETWORK LINK..... OK" : "> NETWORK LINK.....");
  if (elapsed > 1600) u8g2.drawStr(4, 60, timeOk ? "> TIME SYNC........ OK" : "> TIME SYNC........");

  int barW = 40, barX = 128 - barW - 4, barY = 53;
  int fill = (elapsed * barW) / total;
  u8g2.drawFrame(barX, barY, barW, 8);
  if (fill > 2) u8g2.drawBox(barX + 1, barY + 1, fill - 2, 6);

  if ((millis() / 300) % 2 == 0) u8g2.drawBox(4, 4, 3, 3);
  u8g2.sendBuffer();
}
