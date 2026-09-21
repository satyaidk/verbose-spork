/* =============================================================================
   player.h  -  the frame decoder and the playback state machine
   -----------------------------------------------------------------------------
   THE DECODER
       Every frame in anim_*.h is stored as the XOR difference from the frame
       before it, run-length compressed with PackBits. decodeFrame() reverses
       both steps in one pass, XORing into a persistent 1024-byte buffer.

       The consequence that matters: a clip must always be played from frame 0
       with a cleared buffer. Jump into the middle and you get noise, because
       the chain of differences has no starting point. Everything that changes
       clip goes through playClip() for exactly that reason - never assign to
       curIdx yourself.

   THE STATE MACHINE
       MODE_LOOP   repeat the current clip forever (the resting face)
       MODE_ONCE   play through once, then fall back to the idle face

       Idle behaviour mirrors the original Mochi: the resting face loops, and
       every gifDelay seconds one clip is picked at random from the enabled
       set and played once. Touch reactions interrupt whatever is on screen.
   ============================================================================= */
#pragma once

#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "display_mochi.h"
#include "animations.h"

uint8_t frameBuf[FRAME_BYTES];

enum PlayMode { MODE_LOOP, MODE_ONCE };

uint8_t       curIdx    = 0;
uint16_t      frameIdx  = 0;
PlayMode      playMode  = MODE_LOOP;
unsigned long lastFrame = 0;
unsigned long idleUntil = 0;      // when to fire the next random clip
bool          idleArmed = false;

/* ------------------------------------------------------------------ decode */
void decodeFrame(const Anim& a, uint16_t frame) {
  uint16_t i   = pgm_read_word(&a.offsets[frame]);
  uint16_t end = pgm_read_word(&a.offsets[frame + 1]);
  uint16_t o   = 0;

  while (o < FRAME_BYTES && i < end) {
    int8_t t = (int8_t)pgm_read_byte(&a.data[i++]);
    if (t >= 0) {                                   // literal run of t+1 bytes
      uint16_t n = (uint16_t)t + 1;
      while (n-- && o < FRAME_BYTES) frameBuf[o++] ^= pgm_read_byte(&a.data[i++]);
    } else {                                        // one byte repeated 1-t times
      uint16_t n = (uint16_t)(1 - t);
      uint8_t  v = pgm_read_byte(&a.data[i++]);
      while (n-- && o < FRAME_BYTES) frameBuf[o++] ^= v;
    }
  }
}

/* ----------------------------------------------------------------- helpers */
bool clipEnabled(uint8_t i) {
  return (i < ANIM_COUNT) && (S.enabledMask & (1UL << i));
}

/* Pick a random clip from the enabled set, avoiding an immediate repeat when
   there is more than one to choose from. */
uint8_t randomClip() {
  uint8_t pool[32];
  uint8_t n = 0;
  for (uint8_t i = 0; i < ANIM_COUNT && n < 32; i++)
    if (clipEnabled(i)) pool[n++] = i;

  if (n == 0) return S.gifDefault;
  if (n == 1) return pool[0];

  uint8_t pick;
  for (uint8_t tries = 0; tries < 8; tries++) {
    pick = pool[random(n)];
    if (pick != curIdx) return pick;
  }
  return pick;
}

/* -------------------------------------------------------------- transitions */
void playClip(uint8_t idx, PlayMode mode) {
  if (idx == ANIM_NONE || idx >= ANIM_COUNT) return;
  curIdx   = idx;
  frameIdx = 0;
  playMode = mode;
  memset(frameBuf, 0, sizeof(frameBuf));    // the delta chain restarts here
  lastFrame = 0;                            // draw the first frame immediately
}

/* Return to the resting face and arm the next random clip. */
void enterIdle() {
  playClip(S.gifDefault, MODE_LOOP);
  idleUntil = millis() + (unsigned long)S.gifDelay * 1000UL;
  idleArmed = (S.gifDelay > 0);
}

/* A reaction: play once, then drop back to idle. ANIM_NONE means the user
   has disabled that gesture, in which case nothing interrupts the screen.  */
void playReaction(uint8_t idx) {
  if (idx == ANIM_NONE || idx >= ANIM_COUNT) return;
  playClip(idx, MODE_ONCE);
}

/* --------------------------------------------------------------------- tick */
void playerTick() {
  unsigned long now = millis();

  // Is it time for a spontaneous clip? An empty rotation means the user
  // deselected everything, so nothing ever interrupts the resting face.
  if (playMode == MODE_LOOP && idleArmed && S.enabledMask && now >= idleUntil) {
    idleArmed = false;
    playClip(randomClip(), MODE_ONCE);
  }

  if (now - lastFrame < S.gifSpeed) return;
  lastFrame = now;

  decodeFrame(ANIMS[curIdx], frameIdx);
  displayFrame(frameBuf);

  frameIdx++;
  if (frameIdx >= ANIMS[curIdx].frames) {
    if (playMode == MODE_ONCE) enterIdle();
    else                       playClip(curIdx, MODE_LOOP);   // loop, chain reset
  }
}

/* ------------------------------------------------------------------ startup */
/* The intro plays once at boot, then hands over to the idle face. If no intro
   is configured we go straight to idle. */
void playerBegin() {
  if (S.gifIntro != ANIM_NONE && S.gifIntro < ANIM_COUNT) {
    playClip(S.gifIntro, MODE_ONCE);
    playMelody(S.mIntro);
  } else {
    enterIdle();
  }
}

/* Flash usage of the compiled-in frame data, reported on the System page. */
uint32_t animBytesTotal() {
  uint32_t t = 0;
  for (uint8_t i = 0; i < ANIM_COUNT; i++)
    t += pgm_read_word(&ANIMS[i].offsets[ANIMS[i].frames]);
  return t;
}

uint32_t animFramesTotal() {
  uint32_t t = 0;
  for (uint8_t i = 0; i < ANIM_COUNT; i++) t += ANIMS[i].frames;
  return t;
}
