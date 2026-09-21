/* =============================================================================
   MochiWeb 2.0  -  two apps, one device
   -----------------------------------------------------------------------------
   An animated desk companion that is also a clock, weather station, crypto
   ticker and notepad. Both apps share one screen, one touch pad and one web
   controller. Hold the pad for 15 seconds to swap between them.

       APP 1  MOCHI    16 animations, 1232 frames, ~14 FPS
       APP 2  CLOCK    clock / weather / crypto / notes

   TOUCH, IN BOTH APPS
                        Mochi                      Clock
       tap              tap reaction               next screen
       double tap       double reaction            next screen
       triple tap       triple reaction            next screen
       hold 5 s         hold reaction              refresh weather + prices
       hold 15 s        -> switch to Clock         -> switch to Mochi

   The two hold thresholds share one press, so they resolve differently: the
   5 s action fires on RELEASE, the 15 s switch fires WHILE HELD. A progress
   bar appears at 5 s and fills toward 15 s so you can see which you are
   about to get. See touch.h for the full reasoning.

   After any Mochi reaction plays through, the player returns to the resting
   face by itself. Nothing in the mode layer interferes with that.

   HOW THE FILES FIT TOGETHER
       Single translation unit. Every module is a .h that defines as well as
       declares, included below in dependency order. No .cpp files, no extern
       declarations, no link order to get wrong.

       config.h          defaults and limits
       settings.h        struct S and its NVS persistence
       melody.h          the "C4 100 20" language and a non-blocking buzzer
       display_mochi.h   panel bring-up, frame blitting, the hold bar
       animations.h      generated clip index  (+ 16 anim_*.h)
       player.h          frame decoder and the animation state machine
       clock_app.h       the four information screens and their data sources
       touch.h           five gestures from one pin
       app_mode.h        the switch between the apps, and gesture routing
       web_ui.h          the controller page, in flash
       web_api.h         Wi-Fi and the REST API for both apps

   FIRST RUN
       No credentials are stored, so it comes up as its own hotspot:
       join MOCHI-xxxx with password 12345678 and open http://192.168.4.1/.
       Enter your 2.4 GHz network on the System tab; afterwards it lives at
       http://mochi.local/.

   BOARD SETTINGS
       Board            ESP32C3 Dev Module
       USB CDC On Boot  Enabled                            <- required
       Partition        Huge APP (3MB No OTA/1MB SPIFFS)   <- required
   LIBRARIES
       Adafruit SSD1306, Adafruit GFX
   ============================================================================= */

#include "config.h"
#include "settings.h"
#include "melody.h"
#include "display_mochi.h"
#include "animations.h"
#include "player.h"
#include "clock_app.h"
#include "touch.h"
#include "app_mode.h"
#include "web_ui.h"
#include "web_api.h"

/* ------------------------------------------------------------------ setup */
void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== " FW_NAME " " FW_VERSION " ===");

  settingsLoad();
  settingsClamp(ANIM_COUNT);

  melody.begin(S.pinBuzzer);
  if (S.pinBuzzer != PIN_NONE) pinMode(S.pinBuzzer, OUTPUT);

  if (!displayBegin()) {
    // Not fatal on purpose: the web server still comes up, so the pins can be
    // corrected from the Hardware tab without reflashing.
    Serial.println("[boot] continuing without a display");
  }
  displayMessage("MOCHI", "starting");

  if (!startWifi()) startAP();
  displayMessage(apMode ? "hotspot" : "online", currentIP());
  delay(1200);

  webBegin();

  touchBegin();
  appModeBegin();

  randomSeed(esp_random());
  rebuildNoteLines();
  clockApplyTimezone();       // harmless with no network; NTP syncs when it joins

  // Boot into whichever app was last in use. The Mochi app gets its intro;
  // the clock app has none, so it goes straight to the first screen.
  if (S.appMode == APP_CLOCK) {
    clockBegin();
  } else {
    playerBegin();
  }

  Serial.printf("[boot] %s app, %u clips, %lu frames, %lu bytes of frame data\n",
                modeName(S.appMode), ANIM_COUNT,
                (unsigned long)animFramesTotal(), (unsigned long)animBytesTotal());
}

/* ------------------------------------------------------------------- loop */
void loop() {
  server.handleClient();
  touchTick();
  melody.tick();
  appTick();                  // exactly one app runs; the other is fully idle

  // If we joined a network and it drops, keep trying. In hotspot mode we stay
  // put, since somebody may be connected to us right now.
  static unsigned long lastRetry = 0;
  if (!apMode && WiFi.status() != WL_CONNECTED && millis() - lastRetry > WIFI_RETRY) {
    lastRetry = millis();
    Serial.println("[wifi] dropped, reconnecting");
    WiFi.reconnect();
  }
}
