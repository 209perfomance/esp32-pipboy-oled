/* =========================================================
   weather.h
   Погода через OpenWeatherMap + экраны STATS и MAP.
   ========================================================= */

#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "ui.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// =========================================================
// СТРУКТУРА ПОГОДЫ
// =========================================================
struct WeatherSlot {
  int8_t  temp;
  uint8_t code;   // 0=sun, 1=rain, 2=snow
};

struct DayForecast {
  WeatherSlot morning;
  WeatherSlot day;
  WeatherSlot night;
};

extern DayForecast forecast[2];        // 0=сегодня, 1=завтра
extern bool weatherLoaded;
extern int  cityIdx;
extern uint8_t radLevel;
extern const char* playerName;

// =========================================================
// ЗАГРУЗКА ПОГОДЫ
// =========================================================
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String("http://api.openweathermap.org/data/2.5/forecast?") +
               "q=" + CITY_LIST[cityIdx] + ",RU" +
               "&units=metric&lang=ru&APPID=" + OWM_API_KEY;

  http.begin(url);
  http.setTimeout(10000);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[Weather] HTTP error: %d\n", code);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[Weather] Got %d bytes\n", payload.length());

  DynamicJsonDocument doc(24576);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[Weather] JSON error: %s\n", err.c_str());
    return;
  }

  JsonArray list = doc["list"];
  if (list.isNull()) return;

  time_t now = time(nullptr);
  struct tm ti;
  localtime_r(&now, &ti);
  int todayYday = ti.tm_yday;

  // Сброс
  for (int d = 0; d < 2; d++) {
    forecast[d].morning = {0, 0};
    forecast[d].day     = {0, 0};
    forecast[d].night   = {0, 0};
  }
  int mCount[2] = {0,0}, dCount[2] = {0,0}, nCount[2] = {0,0};
  float mTemp[2] = {0,0}, dTemp[2] = {0,0}, nTemp[2] = {0,0};
  int mCode[2] = {0,0}, dCode[2] = {0,0}, nCode[2] = {0,0};

  for (JsonObject item : list) {
    time_t t = (long)item["dt"];
    struct tm local;
    localtime_r(&t, &local);

    int dayDiff = local.tm_yday - todayYday;
    if (dayDiff < 0) dayDiff += 365;
    if (dayDiff > 1) continue;

    int h = local.tm_hour;
    float temp = item["main"]["temp"];
    const char* desc = item["weather"][0]["description"] | "clear";

    uint8_t wcode = 0;
    if (strstr(desc, "дождь") || strstr(desc, "ливень") || strstr(desc, "морось")) wcode = 1;
    else if (strstr(desc, "снег") || strstr(desc, "метель")) wcode = 2;

    if (h >= 6 && h < 12)       { mTemp[dayDiff] += temp; mCode[dayDiff] += wcode; mCount[dayDiff]++; }
    else if (h >= 12 && h < 18) { dTemp[dayDiff] += temp; dCode[dayDiff] += wcode; dCount[dayDiff]++; }
    else if (h >= 18 && h < 24) { nTemp[dayDiff] += temp; nCode[dayDiff] += wcode; nCount[dayDiff]++; }
  }

  for (int d = 0; d < 2; d++) {
    if (mCount[d]) { forecast[d].morning.temp = round(mTemp[d]/mCount[d]); forecast[d].morning.code = mCode[d]/mCount[d]; }
    if (dCount[d]) { forecast[d].day.temp     = round(dTemp[d]/dCount[d]); forecast[d].day.code     = dCode[d]/dCount[d]; }
    if (nCount[d]) { forecast[d].night.temp   = round(nTemp[d]/nCount[d]); forecast[d].night.code   = nCode[d]/nCount[d]; }
    Serial.printf("[Weather] Day %d: M=%d D=%d N=%d\n",
                  d, forecast[d].morning.temp, forecast[d].day.temp, forecast[d].night.temp);
  }

  weatherLoaded = true;
  Serial.println("[Weather] OK");
}

// =========================================================
// STATS — регион + радиация + игрок + погода
// =========================================================
void drawData() {
  drawTopMenu(1);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(2, 19, CITY_LABEL[cityIdx]);

  // Радиация со спиннером
  int spinIdx = (millis() / 200) % 8;
  char radLine[32];
  const char* radNames[] = {"LOW","MOD","MED","HIGH","CRIT"};
  uint8_t lvl = radLevel;
  if (lvl > 4) lvl = 4;
  snprintf(radLine, sizeof(radLine), "RAD: %s %c 0.0%d mSv/h",
           radNames[lvl], RAD_SPINNER[spinIdx], lvl + 1);
  u8g2.drawStr(2, 28, radLine);

  // Имя игрока
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(2, 36, playerName);

  // Разделитель
  u8g2.drawHLine(0, 38, 128);

  // Погода
  if (!weatherLoaded) {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(4, 50, "Loading weather...");
    drawBatteryBar();
    return;
  }

  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(20, 46, "TODAY");
  u8g2.drawStr(76, 46, "T+1");
  u8g2.drawStr(2, 54, "M");
  u8g2.drawStr(2, 62, "D");

  char buf[16];
  for (int d = 0; d < 2; d++) {
    int xBase = 20 + d * 56;

    snprintf(buf, sizeof(buf), "%d %s", forecast[d].morning.temp,
             forecast[d].morning.code == 0 ? "SUN" :
             forecast[d].morning.code == 1 ? "RAIN" : "SNOW");
    u8g2.drawStr(xBase, 54, buf);

    snprintf(buf, sizeof(buf), "%d %s", forecast[d].day.temp,
             forecast[d].day.code == 0 ? "SUN" :
             forecast[d].day.code == 1 ? "RAIN" : "SNOW");
    u8g2.drawStr(xBase, 62, buf);
  }

  drawBatteryBar();
}

// =========================================================
// MAP — Иркутская область + бегущая строка
// =========================================================
void drawMap() {
  drawTopMenu(2);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(2, 19, CITY_LABEL[cityIdx]);

  // Байкал (эллипс из точек)
  int cx = 98, cy = 38, rx = 11, ry = 14;
  for (float a = 0; a < 6.283; a += 0.15) {
    int px = cx + (int)(cos(a) * rx);
    int py = cy + (int)(sin(a) * ry);
    if (py > 22 && py < 54) u8g2.drawPixel(px, py);
  }
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(90, 53, "BAIKAL");

  // Ангара (извилистая линия)
  u8g2.drawHLine(30, 38, 56);
  u8g2.drawPixel(40, 37);
  u8g2.drawPixel(55, 39);
  u8g2.drawPixel(70, 37);

  // Иркутск (мигающий маркер)
  bool blink = (millis() / 500) % 2 == 0;
  if (blink) {
    u8g2.drawDisc(48, 38, 3);
    u8g2.drawCircle(48, 38, 6, U8G2_DRAW_ALL);
  } else {
    u8g2.drawDisc(48, 38, 2);
  }
  u8g2.drawStr(34, 49, "IRKUTSK");

  // Ангарск
  u8g2.drawDisc(58, 30, 1);
  u8g2.drawStr(40, 26, "ANGARSK");

  // Разделитель и бегущая строка
  u8g2.drawHLine(0, 55, 128);
  drawMarquee(62);
}

#endif // WEATHER_H
