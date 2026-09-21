/* =============================================================================
   touch.h  -  five gestures from one digital pin
   -----------------------------------------------------------------------------
   The TTP223 gives a plain HIGH while touched. From that we derive:

       tap          one press-release, then no further tap
       double tap   two, inside MULTI_TAP_MS of each other
       triple tap   three
       long action  held 5 s, then RELEASED before 15 s
       mode switch  held past 15 s

   THE CONFLICT, AND HOW IT IS RESOLVED
       The 5 s and 15 s thresholds share one physical hold: you cannot reach
       15 s without passing through 5 s. If the 5 s action fired the moment
       the threshold was crossed, every mode switch would also refresh the
       clock or play a clip on the way through.

       So the two behave differently on purpose:

           released under 5 s   -> counts as a tap
           released 5 s to 15 s -> onLongAction()   fires on RELEASE
           still held at 15 s   -> onModeSwitch()   fires WHILE HELD,
                                   and the release is then ignored entirely

       That is the only arrangement where both gestures stay reachable and
       neither triggers the other.

   FEEDBACK
       A 15 s hold with no feedback feels broken - you cannot tell whether
       the device noticed. So at 5 s there is a short high beep and a
       progress bar appears along the bottom of the screen, filling as the
       hold approaches 15 s. Release while the bar is showing and you get
       the long action; let it fill and the mode switches.

       The bar is published as g_holdPct and drawn by whichever app owns the
       screen, so this file still knows nothing about display internals.

   Events go out through function pointers, so this module knows nothing
   about animations, the clock, or sound.
   ============================================================================= */
#pragma once

#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "melody.h"
#include "display_mochi.h"   // for g_holdPct

typedef void (*TouchHandler)();

TouchHandler onTap        = nullptr;
TouchHandler onDouble     = nullptr;
TouchHandler onTriple     = nullptr;
TouchHandler onLongAction = nullptr;
TouchHandler onModeSwitch = nullptr;

static bool          tPrev       = false;
static bool          tArmed      = false;   // passed 5 s, bar is showing
static bool          tSwitched   = false;   // passed 15 s, already fired
static uint8_t       tTapCount   = 0;
static unsigned long tPressAt    = 0;
static unsigned long tReleaseAt  = 0;

void touchBegin() {
  if (S.pinTouch == PIN_NONE) {
    Serial.println("[touch] disabled");
    return;
  }
  pinMode(S.pinTouch, INPUT);
  Serial.printf("[touch] on GPIO%u\n", S.pinTouch);
}

/* Reset gesture state. Called when the mode changes so a gesture cannot leak
   from one app into the other.

   The subtle part: tSwitched must NOT be cleared while the pad is still being
   held. setAppMode() calls this from inside the 15 s handler, and if the latch
   were cleared the threshold would still be satisfied on the very next tick,
   firing the switch again and again for as long as the finger stayed down.
   Latching it to tPrev means "if a press is in flight, treat it as already
   handled and ignore it until release". */
void touchReset() {
  tArmed = false;
  tTapCount = 0;
  g_holdPct = 0;
  tSwitched = tPrev;      // held -> stay latched; idle -> genuinely clear
}

void touchTick() {
  if (S.pinTouch == PIN_NONE) return;

  bool          now = digitalRead(S.pinTouch) == HIGH;
  unsigned long t   = millis();

  /* ---- pressed ---- */
  if (now && !tPrev) {
    tPressAt  = t;
    tArmed    = false;
    tSwitched = false;
  }

  /* ---- held ---- */
  if (now) {
    unsigned long held = t - tPressAt;

    if (!tArmed && held >= LONG_ACTION_MS) {
      tArmed = true;
      tTapCount = 0;                    // this is no longer a tap sequence
      playMelody(ARM_TUNE);
    }

    if (tArmed && !tSwitched) {
      // fill the bar across the 5 s -> 15 s window
      unsigned long span = MODE_SWITCH_MS - LONG_ACTION_MS;
      unsigned long into = held - LONG_ACTION_MS;
      if (into > span) into = span;
      g_holdPct = (uint8_t)((into * 100UL) / span);
      if (g_holdPct < 1) g_holdPct = 1;   // 0 means "no bar", so never use it
    }

    if (!tSwitched && held >= MODE_SWITCH_MS) {
      tSwitched = true;
      g_holdPct = 0;
      playMelody(SWITCH_TUNE);
      if (onModeSwitch) onModeSwitch();
    }
  }

  /* ---- released ---- */
  if (!now && tPrev) {
    unsigned long held = t - tPressAt;
    g_holdPct = 0;

    if (tSwitched) {
      // the mode already changed while held; the release means nothing
    } else if (tArmed) {
      if (onLongAction) onLongAction();
    } else if (held > TAP_MIN_MS) {
      tTapCount++;
      tReleaseAt = t;
    }
    tArmed = false;
    tSwitched = false;
  }
  tPrev = now;

  /* ---- resolve the tap count once the window closes ---- */
  if (tTapCount && !now && t - tReleaseAt > MULTI_TAP_MS) {
    uint8_t n = tTapCount;
    tTapCount = 0;
    if      (n == 1) { if (onTap)    onTap();    }
    else if (n == 2) { if (onDouble) onDouble(); }
    else             { if (onTriple) onTriple(); }   // 3 or more
  }
}
