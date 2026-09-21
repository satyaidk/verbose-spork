/* =============================================================================
   app_mode.h  -  the switch between the two apps, and gesture routing
   -----------------------------------------------------------------------------
   Two apps share one device, one screen and one touch pad. This file owns the
   handover and nothing else, which keeps the apps themselves unaware of each
   other: player.h has no idea the clock exists, and clock_app.h has no idea
   there are animations.

   THE RULES

   1. Exactly one app is live at a time. appTick() calls one of the two tick
      functions, never both. The other is completely idle - no fetching, no
      frame decoding, no timers advancing.

   2. Switching always goes through setAppMode(). It stops any melody, clears
      the screen, resets the gesture state so a hold cannot leak across, and
      calls the incoming app's begin function. Nothing else may assign to
      S.appMode.

   3. The mode is persisted, so the device boots back into whichever app you
      were last using.

   GESTURE ROUTING
                        Mochi app                 Clock app
       tap              gifTap reaction           next screen
       double tap       gifDouble reaction        next screen
       triple tap       gifTriple reaction        next screen
       hold 5 s         gifLong reaction          refresh weather + crypto
       hold 15 s        switch to Clock           switch to Mochi

   In the clock app every tap count advances one screen. The original Info
   Terminal only had a single tap, and leaving double and triple dead would
   feel broken to anyone who taps quickly; advancing is the least surprising
   thing they can do.

   After any Mochi reaction finishes, the player returns to the resting face
   on its own - that behaviour lives in player.h and is untouched here.
   ============================================================================= */
#pragma once

#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "melody.h"
#include "display_mochi.h"
#include "player.h"
#include "clock_app.h"
#include "touch.h"

const char* modeName(uint8_t m) { return (m == APP_CLOCK) ? "clock" : "mochi"; }

/* The one place S.appMode changes. */
void setAppMode(uint8_t m, bool persist) {
  if (m > APP_CLOCK) m = APP_MOCHI;
  S.appMode = m;

  melody.stop();
  touchReset();                 // a half-finished gesture must not carry over
  if (displayOK) {
    display.clearDisplay();
    display.display();
  }

  if (m == APP_CLOCK) clockBegin();
  else                enterIdle();      // resting face, no intro on a switch

  if (persist) settingsSave();
  Serial.printf("[mode] now %s\n", modeName(m));
}

void toggleAppMode() {
  setAppMode(S.appMode == APP_MOCHI ? APP_CLOCK : APP_MOCHI, true);
}

/* --------------------------------------------------------- gesture handlers */
void gestureTap() {
  if (S.appMode == APP_CLOCK) { clockNextScreen(); playMelody(S.mTap); return; }
  playMelody(S.mTap);
  playReaction(S.gifTap);
}

void gestureDouble() {
  if (S.appMode == APP_CLOCK) { clockNextScreen(); playMelody(S.mDouble); return; }
  playMelody(S.mDouble);
  playReaction(S.gifDouble);
}

void gestureTriple() {
  if (S.appMode == APP_CLOCK) { clockNextScreen(); playMelody(S.mTriple); return; }
  playMelody(S.mTriple);
  playReaction(S.gifTriple);
}

void gestureLongAction() {
  if (S.appMode == APP_CLOCK) {
    displayMessage("Refreshing");
    clockRefresh(true);
    lastDraw = 0;
    return;
  }
  playMelody(S.mLong);
  playReaction(S.gifLong);
}

void gestureModeSwitch() {
  toggleAppMode();
}

void appModeBegin() {
  onTap        = gestureTap;
  onDouble     = gestureDouble;
  onTriple     = gestureTriple;
  onLongAction = gestureLongAction;
  onModeSwitch = gestureModeSwitch;
}

/* Called once per loop. Exactly one app runs. */
void appTick() {
  if (S.appMode == APP_CLOCK) clockTick();
  else                        playerTick();
}
