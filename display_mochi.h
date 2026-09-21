/* =============================================================================
   display_mochi.h  -  bringing up the panel and getting a frame onto it
   -----------------------------------------------------------------------------
   The pins and I2C speed come from Settings rather than #defines, because the
   Hardware page of the web UI can change them. They are only read at
   displayBegin(); changing pins therefore requires a reboot, which the web
   handler does for you.

   displayOK is false if no panel answered. Nothing in this firmware halts on
   that: the web server still runs so you can fix the pins remotely. That is
   deliberate - a wrong pin should never lock you out of the device.
   ============================================================================= */
#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "settings.h"

Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

bool    displayOK   = false;
uint8_t displayAddr = 0x3C;

/* Hold-progress bar, 0 = hidden, 1..100 = fill percentage.
   touch.h writes it; both apps draw it. It lives here rather than in touch.h
   so that touch.h needs no knowledge of the display, and so that a single
   overlay function serves the animation player and the clock screens alike. */
uint8_t g_holdPct = 0;

/* Is there an I2C device at addr on these pins? */
static bool i2cProbe(uint8_t sda, uint8_t scl, uint32_t hz, uint8_t addr) {
  Wire.end();
  delay(5);
  if (!Wire.begin(sda, scl, hz)) return false;
  delay(10);
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

/* Apply the settings that can change without a reboot. */
void displayApplyLive() {
  if (!displayOK) return;
  display.setRotation(S.flip ? 2 : 0);
  display.invertDisplay(S.negative);
}

bool displayBegin() {
  displayOK = false;

  // 0x3D is the alternate address used by some 128x64 modules
  const uint8_t addrs[2] = { 0x3C, 0x3D };
  for (uint8_t i = 0; i < 2; i++) {
    if (i2cProbe(S.pinSda, S.pinScl, 100000UL, addrs[i])) {
      displayAddr = addrs[i];
      displayOK = true;
      break;
    }
  }
  if (!displayOK) {
    Serial.printf("[display] nothing on SDA=%u SCL=%u at 0x3C or 0x3D\n",
                  S.pinSda, S.pinScl);
    return false;
  }

  Wire.end();
  delay(5);
  Wire.begin(S.pinSda, S.pinScl, S.i2cHz);

  if (!display.begin(SSD1306_SWITCHCAPVCC, displayAddr)) {
    Serial.println("[display] responded on I2C but begin() failed");
    displayOK = false;
    return false;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  displayApplyLive();
  display.display();

  Serial.printf("[display] OK on SDA=%u SCL=%u addr=0x%02X at %lu Hz\n",
                S.pinSda, S.pinScl, displayAddr, (unsigned long)S.i2cHz);
  return true;
}

/* Draw the hold-progress bar if a long press is in flight. Call this after
   the app has drawn its own content and before display(). */
void overlayHold() {
  if (!displayOK || g_holdPct == 0) return;
  display.fillRect(0, OLED_H - 5, OLED_W, 5, SSD1306_BLACK);
  display.drawRect(0, OLED_H - 5, OLED_W, 5, SSD1306_WHITE);
  int w = (OLED_W - 4) * (int)g_holdPct / 100;
  if (w > 0) display.fillRect(2, OLED_H - 3, w, 1, SSD1306_WHITE);
}

/* Every app finishes a frame through here, so the hold bar can never be
   forgotten by one of them. */
void displayFinish() {
  if (!displayOK) return;
  overlayHold();
  display.display();
}

/* Push a decoded 1024-byte frame. The buffer layout is row-major, MSB first,
   which is exactly what Adafruit_GFX::drawBitmap consumes, so there is no
   conversion step. Passing a non-const uint8_t* selects the RAM overload.   */
void displayFrame(uint8_t* buf) {
  if (!displayOK) return;
  display.clearDisplay();
  display.drawBitmap(0, 0, buf, OLED_W, OLED_H, SSD1306_WHITE);
  displayFinish();
}

/* Centre a string horizontally at a given baseline. Used by the clock app. */
void centerText(const String& s, int y, uint8_t size) {
  if (!displayOK) return;
  int16_t x1, y1; uint16_t w, h;
  display.setTextSize(size);
  display.getTextBounds(s, 0, y, &x1, &y1, &w, &h);
  display.setCursor((OLED_W - (int)w) / 2, y);
  display.print(s);
}

/* Small centred message, used for boot and error states. */
void displayMessage(const String& l1, const String& l2 = "") {
  if (!displayOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  int16_t x1, y1; uint16_t w, h;

  display.getTextBounds(l1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((OLED_W - (int)w) / 2, l2.length() ? 22 : 28);
  display.print(l1);

  if (l2.length()) {
    display.getTextBounds(l2, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((OLED_W - (int)w) / 2, 36);
    display.print(l2);
  }
  display.display();
}
