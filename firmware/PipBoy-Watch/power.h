/* =========================================================
   power.h
   Управление питанием: screen timeout, Light Sleep,
   пробуждение по кнопкам, инверсия, поворот экрана.
   ========================================================= */

#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include "config.h"

// Внешние переменные (объявлены в основном .ino)
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern uint8_t brightness;
extern int     timeoutIdx;
extern bool    invertDisplay;
extern bool    flipDisplay;

// Состояние
extern unsigned long lastActivity;
extern bool          screenIsOff;
extern bool          inLightSleep;
extern unsigned long screenOffAt;

// Массив кнопок (для сброса состояния после пробуждения)
struct ButtonState {
  uint8_t pin;
  bool lastState;
  bool longFired;
};
extern ButtonState buttonStates[4];

// =========================================================
// Погасить экран (без сна)
// =========================================================
void screenOff() {
  u8g2.setPowerSave(1);
  screenIsOff = true;
  screenOffAt = millis();
  Serial.println("[Screen] OFF");
}

// =========================================================
// Включить экран
// =========================================================
void screenOn() {
  if (inLightSleep) return;   // разбудит обработчик сна
  u8g2.setPowerSave(0);
  u8g2.setContrast(brightness);
  screenIsOff = false;
  lastActivity = millis();
  Serial.println("[Screen] ON");
}

// =========================================================
// Настройка источников пробуждения (все 4 кнопки)
// =========================================================
void setupWakeupSources() {
  gpio_wakeup_enable((gpio_num_t)PIN_K1, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_K2, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_K3, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_K4, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
}

// =========================================================
// Уход в Light Sleep
// =========================================================
void enterLightSleep() {
  Serial.println("[Sleep] Entering Light Sleep...");
  inLightSleep = true;

  setupWakeupSources();

  // Отключаем Wi-Fi — иначе он будет жрать ток
  WiFi.mode(WIFI_OFF);

  // Спим до нажатия любой кнопки
  esp_light_sleep_start();

  // ==== ПРОСНУЛИСЬ ====
  Serial.println("[Sleep] Woke up!");
  inLightSleep = false;

  // Возвращаем Wi-Fi
  WiFi.mode(WIFI_STA);

  // Включаем экран
  u8g2.setPowerSave(0);
  u8g2.setContrast(brightness);
  screenIsOff = false;
  lastActivity = millis();

  // Пропускаем нажатие, которое разбудило
  delay(200);
  for (int i = 0; i < 4; i++) {
    buttonStates[i].lastState = digitalRead(buttonStates[i].pin);
    buttonStates[i].longFired = true;
  }
}

// =========================================================
// Проверка таймаута
// =========================================================
void checkScreenTimeout() {
  int sec = TIMEOUT_OPTIONS[timeoutIdx];
  if (sec == 0) return;             // OFF — не гасим
  if (inLightSleep) return;         // уже спим

  // Экран погашен — пора спать
  if (screenIsOff) {
    if (millis() - screenOffAt > LIGHT_SLEEP_DELAY) {
      enterLightSleep();
    }
    return;
  }

  // Проверка таймаута
  if (millis() - lastActivity > (unsigned long)sec * 1000UL) {
    screenOff();
  }
}

// =========================================================
// Инверсия
// =========================================================
void applyInvert() {
  u8g2.sendF("c", 0xA6 | (invertDisplay ? 0x01 : 0x00));
}

// =========================================================
// Поворот на 180°
// =========================================================
void applyFlip() {
  u8g2.setDisplayRotation(flipDisplay ? U8G2_R2 : U8G2_R0);
}

#endif // POWER_H
