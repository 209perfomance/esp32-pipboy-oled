/* =========================================================
   settings.h
   Preferences (NVS) + все экраны SETT:
   TIME, BRIGHT, CITY, PLAYER, TIMEOUT, INVERT, FLIP, RESET, INFO
   ========================================================= */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "ui.h"
#include "power.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// =========================================================
// ВНЕШНИЕ ПЕРЕМЕННЫЕ
// =========================================================
extern bool    invertDisplay;
extern bool    flipDisplay;
extern uint8_t brightness;
extern int     timeoutIdx;
extern int     cityIdx;

enum TimeSource { TIME_WIFI, TIME_MANUAL };
extern TimeSource timeSource;

extern int manualDay, manualMonth, manualYear;
extern int manualHour, manualMinute;

extern char playerName[16];
extern int  playerHP;
extern int  playerCaps;
extern int  playerLVL;

extern int settCursor;
extern int settScroll;
extern const int SETT_VISIBLE;
extern const int SETT_COUNT;
extern const char* SETT_ITEMS[];

extern int playerField;
extern const int PLAYER_FIELDS;

// Идентификаторы подэкранов SETT (enum, объявленный в .ino)
extern int SCREEN_SETT_BRIGHT_ID;
extern int SCREEN_SETT_CITY_ID;
extern int SCREEN_SETT_PLAYER_ID;
extern int SCREEN_SETT_TIMEOUT_ID;
extern int SCREEN_SETT_INVERT_ID;
extern int SCREEN_SETT_FLIP_ID;
extern int SCREEN_SETT_RESET_ID;
extern int SCREEN_SETT_INFO_ID;
extern int SCREEN_TIME_SOURCE_ID;
extern int SCREEN_TIME_EDIT_ID;

Preferences prefs;

// =========================================================
// PREFERENCES
// =========================================================
void loadPrefs() {
  prefs.begin("pipboy", true);
  invertDisplay = prefs.getBool("invert", false);
  flipDisplay   = prefs.getBool("flip", false);
  brightness    = prefs.getUChar("bright", 255);
  timeSource    = (TimeSource)prefs.getInt("timesrc", 0);
  cityIdx       = prefs.getInt("city", 0);
  timeoutIdx    = prefs.getInt("timeout", 3);
  playerHP      = prefs.getInt("hp", 95);
  playerCaps    = prefs.getInt("caps", 1337);
  playerLVL     = prefs.getInt("lvl", 12);
  String nm     = prefs.getString("name", "WANDERER");
  strncpy(playerName, nm.c_str(), sizeof(playerName) - 1);
  prefs.end();

  if (timeoutIdx < 0 || timeoutIdx >= TIMEOUT_COUNT) timeoutIdx = 3;
  if (cityIdx < 0 || cityIdx >= CITY_COUNT) cityIdx = 0;
}

void savePrefs() {
  prefs.begin("pipboy", false);
  prefs.putBool("invert", invertDisplay);
  prefs.putBool("flip", flipDisplay);
  prefs.putUChar("bright", brightness);
  prefs.putInt("timesrc", (int)timeSource);
  prefs.putInt("city", cityIdx);
  prefs.putInt("timeout", timeoutIdx);
  prefs.putInt("hp", playerHP);
  prefs.putInt("caps", playerCaps);
  prefs.putInt("lvl", playerLVL);
  prefs.putString("name", playerName);
  prefs.end();
}

// =========================================================
// SETTINGS — список с прокруткой
// =========================================================
void drawSettings() {
  drawTopMenu(3);
  u8g2.setFont(u8g2_font_5x7_tf);

  for (int i = 0; i < SETT_VISIBLE; i++) {
    int idx = settScroll + i;
    if (idx >= SETT_COUNT) break;
    int y = 20 + i * 11;

    const char* label = SETT_ITEMS[idx];
    const char* val = "";
    static char b[8];

    switch (idx) {
      case 0: val = (timeSource == TIME_WIFI) ? "WIFI" : "MAN"; break;
      case 1: snprintf(b, 8, "%d", brightness); val = b; break;
      case 2: val = CITY_LABEL[cityIdx]; break;
      case 3: val = playerName; break;
      case 4: val = TIMEOUT_LABELS[timeoutIdx]; break;
      case 5: val = invertDisplay ? "ON" : "OFF"; break;
      case 6: val = flipDisplay ? "ON" : "OFF"; break;
      case 7: val = ">"; break;
      case 8: val = "v2.0"; break;
    }

    if (idx == settCursor) {
      u8g2.drawBox(0, y - 7, 128, 10);
      u8g2.setDrawColor(0);
      u8g2.drawStr(4, y, label);
      int w = u8g2.getStrWidth(val);
      u8g2.drawStr(124 - w, y, val);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(4, y, label);
      int w = u8g2.getStrWidth(val);
      u8g2.drawStr(124 - w, y, val);
    }
  }

  // Скроллбар справа
  if (SETT_COUNT > SETT_VISIBLE) {
    int totalH = 52;
    int barH = (totalH * SETT_VISIBLE) / SETT_COUNT;
    int barY = 11 + (totalH - barH) * settScroll / (SETT_COUNT - SETT_VISIBLE);
    u8g2.drawVLine(126, 11, totalH);
    u8g2.drawBox(126, barY, 2, barH);
  }
}

// =========================================================
// SETT-BRIGHT
// =========================================================
void drawSettBright() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "BRIGHTNESS");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", brightness);
  u8g2.drawStr(50, 32, buf);

  int barX = 10, barW = 108, barY = 40, barH = 8;
  u8g2.drawFrame(barX, barY, barW, barH);
  int fill = (brightness * (barW - 2)) / 255;
  if (fill > 0) u8g2.drawBox(barX + 1, barY + 1, fill, barH - 2);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 58, "K1/K2 +/-  K4=back");
}

// =========================================================
// SETT-CITY
// =========================================================
void drawSettCity() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "CITY");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  for (int i = 0; i < CITY_COUNT; i++) {
    int y = 22 + i * 10;
    if (i == cityIdx) {
      u8g2.drawBox(4, y - 7, 120, 10);
      u8g2.setDrawColor(0);
      u8g2.drawStr(8, y, CITY_LABEL[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(8, y, CITY_LABEL[i]);
    }
  }

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 63, "K3=save K4=back");
}

// =========================================================
// SETT-PLAYER
// =========================================================
void drawSettPlayer() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "PLAYER");
  u8g2.drawHLine(0, 12, 128);

  char line[24];

  snprintf(line, sizeof(line), "HP:   %d", playerHP);
  if (playerField == 0) u8g2.drawBox(0, 15, 128, 10);
  u8g2.setDrawColor(playerField == 0 ? 0 : 1);
  u8g2.drawStr(4, 23, line);
  u8g2.setDrawColor(1);

  snprintf(line, sizeof(line), "CAPS: %d", playerCaps);
  if (playerField == 1) u8g2.drawBox(0, 26, 128, 10);
  u8g2.setDrawColor(playerField == 1 ? 0 : 1);
  u8g2.drawStr(4, 34, line);
  u8g2.setDrawColor(1);

  snprintf(line, sizeof(line), "LVL:  %d", playerLVL);
  if (playerField == 2) u8g2.drawBox(0, 37, 128, 10);
  u8g2.setDrawColor(playerField == 2 ? 0 : 1);
  u8g2.drawStr(4, 45, line);
  u8g2.setDrawColor(1);

  if (playerField == 3) {
    u8g2.drawBox(0, 48, 128, 10);
    u8g2.setDrawColor(0);
    u8g2.drawStr(40, 56, "> SAVE <");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(40, 56, "SAVE");
  }
}

// =========================================================
// SETT-TIMEOUT
// =========================================================
void drawSettTimeout() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "SCREEN TIMEOUT");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(30, 35, TIMEOUT_LABELS[timeoutIdx]);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 58, "K1/K2 sel  K4=back");
}

// =========================================================
// SETT-INVERT
// =========================================================
void drawSettInvert() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "INVERT");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(30, 35, invertDisplay ? "ON" : "OFF");

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 58, "K3=toggle K4=back");
}

// =========================================================
// SETT-FLIP
// =========================================================
void drawSettFlip() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "FLIP 180");
  u8g2.drawHLine(0, 12, 128);

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(30, 35, flipDisplay ? "ON" : "OFF");

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 58, "K3=toggle K4=back");
}

// =========================================================
// SETT-RESET
// =========================================================
void drawSettReset() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "FACTORY RESET");
  u8g2.drawHLine(0, 12, 128);

  u8g2.drawStr(4, 30, "All settings will be");
  u8g2.drawStr(4, 40, "reset to default.");

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(20, 58, "K3=RESET");
}

// =========================================================
// SETT-INFO
// =========================================================
void drawSettInfo() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(4, 10, "SYSTEM INFO");
  u8g2.drawHLine(0, 12, 128);

  char buf[32];
  snprintf(buf, 32, "IP: %s", WiFi.localIP().toString().c_str());
  u8g2.drawStr(4, 22, buf);

  snprintf(buf, 32, "RSSI: %d dBm", WiFi.RSSI());
  u8g2.drawStr(4, 30, buf);

  snprintf(buf, 32, "HEAP: %d", ESP.getFreeHeap());
  u8g2.drawStr(4, 38, buf);

  snprintf(buf, 32, "UP: %lu s", millis() / 1000);
  u8g2.drawStr(4, 46, buf);

  u8g2.drawStr(4, 56, "VERSION: v2.0");
}

#endif // SETTINGS_H
