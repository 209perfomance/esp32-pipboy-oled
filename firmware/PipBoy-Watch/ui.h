/* =========================================================
   ui.h
   Общие элементы интерфейса:
   - drawTopMenu         — верхнее меню CLOCK/STATS/MAP/SETT
   - drawBatteryBar      — компактная батарея внизу
   - drawMarquee         — бегущая строка
   - drawVaultBoyScaled  — Vault Boy нужного размера
   ========================================================= */

#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "vaultboy.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// =========================================================
// ВЕРХНЕЕ МЕНЮ
// activeIdx: 0=CLOCK, 1=STATS, 2=MAP, 3=SETT, -1=без подсветки
// =========================================================
void drawTopMenu(int activeIdx) {
  const char* items[] = {"CLOCK", "STATS", "MAP", "SETT"};
  int pos[] = {2, 34, 64, 84};

  u8g2.setFont(u8g2_font_5x7_tf);
  for (int i = 0; i < 4; i++) u8g2.drawStr(pos[i], 8, items[i]);

  if (activeIdx < 0 || activeIdx > 3) return;

  int x0 = pos[activeIdx] - 2;
  int w  = u8g2.getStrWidth(items[activeIdx]);
  int x1 = pos[activeIdx] + w + 1;

  // Верхние уголки
  u8g2.drawPixel(x0, 0);     u8g2.drawPixel(x0 + 1, 0); u8g2.drawPixel(x0, 1);
  u8g2.drawPixel(x1, 0);     u8g2.drawPixel(x1 + 1, 0); u8g2.drawPixel(x1 + 1, 1);
  // Нижние уголки
  u8g2.drawPixel(x0, 9);     u8g2.drawPixel(x0 + 1, 9); u8g2.drawPixel(x0, 8);
  u8g2.drawPixel(x1, 9);     u8g2.drawPixel(x1 + 1, 9); u8g2.drawPixel(x1 + 1, 8);
}

// =========================================================
// БАТАРЕЯ (компактная, внизу экрана)
// =========================================================
extern int batteryPercent;   // объявлен в .ino

void drawBatteryBar() {
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(2, 63, "PWR");

  int barX = 22, barW = 38, barY = 59, barH = 4;
  u8g2.drawFrame(barX, barY, barW, barH);

  int fill = (batteryPercent * (barW - 2)) / 100;
  if (fill > 0) u8g2.drawBox(barX + 1, barY + 1, fill, barH - 2);

  char buf[8];
  snprintf(buf, sizeof(buf), "%d", batteryPercent);
  u8g2.drawStr(62, 63, buf);
}

// =========================================================
// БЕГУЩАЯ СТРОКА
// y — базовая линия
// =========================================================
extern unsigned long marqueeStart;

void drawMarquee(int y) {
  int offset = ((millis() - marqueeStart) / 80) % (MARQUEE_LEN * 6);

  u8g2.setFont(u8g2_font_4x6_tf);

  for (int copy = -1; copy <= 2; copy++) {
    int xStart = -offset + copy * (MARQUEE_LEN * 6);
    if (xStart > 128 || xStart + MARQUEE_LEN * 6 < 0) continue;
    u8g2.drawStr(xStart, y, MARQUEE_TEXT);
  }
}

// =========================================================
// VAULT BOY (масштабирование по пикселям)
// targetSize — желаемый размер: 16, 32, 48, 64
// =========================================================
void drawVaultBoyScaled(int x, int y, int targetSize) {
  for (int row = 0; row < targetSize; row++) {
    for (int col = 0; col < targetSize; col++) {
      int srcX = col * 64 / targetSize;
      int srcY = row * 64 / targetSize;
      int byteIdx = srcY * 8 + (srcX / 8);
      int bitIdx  = srcX % 8;

      if (vaultboy_bits[byteIdx] & (1 << bitIdx)) {
        int px = x + col;
        int py = y + row;
        if (px >= 0 && px < 128 && py >= 0 && py < 64) {
          u8g2.drawPixel(px, py);
        }
      }
    }
  }
}

#endif // UI_H
