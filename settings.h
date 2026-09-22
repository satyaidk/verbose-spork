/* =============================================================================
   settings.h  -  everything the user can change, and how it is persisted
   -----------------------------------------------------------------------------
   One global struct, S. Three operations:

       settingsDefaults()   fill S with factory values (does not touch flash)
       settingsLoad()       read flash into S, falling back to defaults per key
       settingsSave()       write S to flash

   Settings are applied live the moment the web UI changes them, and only
   written to flash when the user presses Save. That mirrors the original Mochi
   controller: you can audition a change and walk away without keeping it.

   NVS keys are capped at 15 characters by the ESP-IDF, which is why they are
   abbreviated. Do not lengthen them.
   ============================================================================= */
#pragma once

#include <Preferences.h>
#include "config.h"

struct Settings {
  /* ---- which app is running ---- */
  uint8_t  appMode;       // APP_MOCHI or APP_CLOCK, persisted across reboots

  /* ---- animation ---- */
  uint16_t gifSpeed;      // ms per frame
  uint8_t  gifDelay;      // seconds of idle face between random clips
  uint8_t  gifDefault;    // the resting face
  uint8_t  gifIntro;      // played once at boot, ANIM_NONE to skip
  uint8_t  gifTap;        // single tap reaction
  uint8_t  gifDouble;     // double tap reaction
  uint8_t  gifTriple;     // triple tap reaction
  uint8_t  gifLong;       // 5 s long press reaction
  uint32_t enabledMask;   // bit n set = clip n is in the random rotation
  bool     negative;      // invert the panel (white background)

  /* ---- sound ---- */
  bool     soundOn;
  String   mIntro;        // played with the intro clip at boot
  String   mNotify;       // played by /api/notify - hook for future BLE work
  String   mTap;
  String   mDouble;
  String   mTriple;
  String   mLong;

  /* ---- hardware ---- */
  uint8_t  pinSda;
  uint8_t  pinScl;
  uint8_t  pinTouch;      // PIN_NONE disables the touch pad
  uint8_t  pinBuzzer;     // PIN_NONE disables sound entirely
  bool     flip;          // rotate the panel 180 degrees
  uint32_t i2cHz;

  /* ---- clock app ---- */
  String   tz;            // POSIX timezone string
  bool     metric;        // false = Fahrenheit and mph
  String   noteTitle;     // header on the notes screen
  String   noteText;      // the text itself, wrapped on the device

  /* ---- network ---- */
  String   ssid;
  String   pass;
};

Settings   S;
Preferences prefs;

/* ---------------------------------------------------------------- defaults */
void settingsDefaults() {
  S.appMode     = APP_MOCHI;
  S.gifSpeed    = DEF_GIF_SPEED;
  S.gifDelay    = DEF_GIF_DELAY;
  S.gifDefault  = 1;              // Stuck
  S.gifIntro    = 0;              // Intro
  S.gifTap      = 4;              // Game
  S.gifDouble   = 6;              // Cry
  S.gifTriple   = 5;              // Smile
  S.gifLong     = 14;             // Speed
  S.enabledMask = 0xFFFFFFFFUL;   // every clip in rotation; trimmed at load
  S.negative    = false;

  S.soundOn     = true;
  S.mIntro      = "C5 90 20 E5 90 20 G5 90 20 C6 220 200";
  S.mNotify     = "G4 100 20 C5 100 20 E5 100 20 G5 200 200";
  S.mTap        = "C6 40 10 G6 60 100";
  S.mDouble     = "E5 90 10 G5 90 10 C6 200 150";
  S.mTriple     = "C5 70 10 E5 70 10 G5 70 10 C6 160 140";
  S.mLong       = "G5 90 20 E5 90 20 C5 90 20 G4 200 200";

  S.pinSda      = DEF_PIN_SDA;
  S.pinScl      = DEF_PIN_SCL;
  S.pinTouch    = DEF_PIN_TOUCH;
  S.pinBuzzer   = DEF_PIN_BUZZER;
  S.flip        = false;
  S.i2cHz       = DEF_I2C_HZ;

  S.tz          = DEF_TZ;
  S.metric      = true;
  S.noteTitle   = "Notes";
  S.noteText    = "Tap to change screen.\n\nHold 5s to refresh.\nHold 15s for Mochi.";

  S.ssid        = "";
  S.pass        = "";
}

/* -------------------------------------------------------------------- load */
void settingsLoad() {
  settingsDefaults();                       // every key falls back to this
  prefs.begin(NVS_NAMESPACE, true);         // read-only

  S.appMode     = prefs.getUChar ("mode",      S.appMode);
  S.gifSpeed    = prefs.getUShort("gifSpeed",  S.gifSpeed);
  S.gifDelay    = prefs.getUChar ("gifDelay",  S.gifDelay);
  S.gifDefault  = prefs.getUChar ("gDef",      S.gifDefault);
  S.gifIntro    = prefs.getUChar ("gIntro",    S.gifIntro);
  S.gifTap      = prefs.getUChar ("gTap",      S.gifTap);
  S.gifDouble   = prefs.getUChar ("gDbl",      S.gifDouble);
  S.gifTriple   = prefs.getUChar ("gTrip",     S.gifTriple);
  S.gifLong     = prefs.getUChar ("gLong",     S.gifLong);
  S.enabledMask = prefs.getUInt  ("mask",      S.enabledMask);
  S.negative    = prefs.getBool  ("neg",       S.negative);

  S.soundOn     = prefs.getBool  ("snd",       S.soundOn);
  S.mIntro      = prefs.getString("mIntro",    S.mIntro);
  S.mNotify     = prefs.getString("mNotify",   S.mNotify);
  S.mTap        = prefs.getString("mTap",      S.mTap);
  S.mDouble     = prefs.getString("mDbl",      S.mDouble);
  S.mTriple     = prefs.getString("mTrip",     S.mTriple);
  S.mLong       = prefs.getString("mLong",     S.mLong);

  S.pinSda      = prefs.getUChar ("sda",       S.pinSda);
  S.pinScl      = prefs.getUChar ("scl",       S.pinScl);
  S.pinTouch    = prefs.getUChar ("touch",     S.pinTouch);
  S.pinBuzzer   = prefs.getUChar ("buzz",      S.pinBuzzer);
  S.flip        = prefs.getBool  ("flip",      S.flip);
  S.i2cHz       = prefs.getUInt  ("i2c",       S.i2cHz);

  S.tz          = prefs.getString("tz",        S.tz);
  S.metric      = prefs.getBool  ("metric",    S.metric);
  S.noteTitle   = prefs.getString("ntitle",    S.noteTitle);
  S.noteText    = prefs.getString("ntext",     S.noteText);

  S.ssid        = prefs.getString("ssid",      S.ssid);
  S.pass        = prefs.getString("pass",      S.pass);

  prefs.end();
}

/* -------------------------------------------------------------------- save */
void settingsSave() {
  prefs.begin(NVS_NAMESPACE, false);        // read/write

  prefs.putUChar ("mode",     S.appMode);
  prefs.putUShort("gifSpeed", S.gifSpeed);
  prefs.putUChar ("gifDelay", S.gifDelay);
  prefs.putUChar ("gDef",     S.gifDefault);
  prefs.putUChar ("gIntro",   S.gifIntro);
  prefs.putUChar ("gTap",     S.gifTap);
  prefs.putUChar ("gDbl",     S.gifDouble);
  prefs.putUChar ("gTrip",    S.gifTriple);
  prefs.putUChar ("gLong",    S.gifLong);
  prefs.putUInt  ("mask",     S.enabledMask);
  prefs.putBool  ("neg",      S.negative);

  prefs.putBool  ("snd",      S.soundOn);
  prefs.putString("mIntro",   S.mIntro);
  prefs.putString("mNotify",  S.mNotify);
  prefs.putString("mTap",     S.mTap);
  prefs.putString("mDbl",     S.mDouble);
  prefs.putString("mTrip",    S.mTriple);
  prefs.putString("mLong",    S.mLong);

  prefs.putUChar ("sda",      S.pinSda);
  prefs.putUChar ("scl",      S.pinScl);
  prefs.putUChar ("touch",    S.pinTouch);
  prefs.putUChar ("buzz",     S.pinBuzzer);
  prefs.putBool  ("flip",     S.flip);
  prefs.putUInt  ("i2c",      S.i2cHz);

  prefs.putString("tz",       S.tz);
  prefs.putBool  ("metric",   S.metric);
  prefs.putString("ntitle",   S.noteTitle);
  prefs.putString("ntext",    S.noteText);

  prefs.putString("ssid",     S.ssid);
  prefs.putString("pass",     S.pass);

  prefs.end();
}

/* ------------------------------------------------------------------- erase */
void settingsErase() {
  prefs.begin(NVS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}

/* --------------------------------------------------------------- sanitise */
/* Called after load and after every web change. Keeps impossible values out
   of the playback engine, which does no checking of its own.               */
void settingsClamp(uint8_t animCount) {
  if (S.gifSpeed < MIN_GIF_SPEED) S.gifSpeed = MIN_GIF_SPEED;
  if (S.gifSpeed > MAX_GIF_SPEED) S.gifSpeed = MAX_GIF_SPEED;
  if (S.gifDelay > MAX_GIF_DELAY) S.gifDelay = MAX_GIF_DELAY;

  if (S.gifDefault >= animCount) S.gifDefault = 0;
  if (S.gifIntro  != ANIM_NONE && S.gifIntro  >= animCount) S.gifIntro  = ANIM_NONE;
  if (S.gifTap    != ANIM_NONE && S.gifTap    >= animCount) S.gifTap    = ANIM_NONE;
  if (S.gifDouble != ANIM_NONE && S.gifDouble >= animCount) S.gifDouble = ANIM_NONE;
  if (S.gifTriple != ANIM_NONE && S.gifTriple >= animCount) S.gifTriple = ANIM_NONE;
  if (S.gifLong   != ANIM_NONE && S.gifLong   >= animCount) S.gifLong   = ANIM_NONE;

  // drop mask bits above the number of clips actually compiled in
  uint32_t valid = (animCount >= 32) ? 0xFFFFFFFFUL : ((1UL << animCount) - 1UL);
  S.enabledMask &= valid;
  // A mask of 0 is legal and means "no spontaneous clips, just the resting
  // face". Deselect all in the web UI is a real choice, not a mistake to
  // correct, so it is left alone here and honoured in playerTick().

  if (S.i2cHz < 100000UL)  S.i2cHz = 100000UL;
  if (S.i2cHz > 1000000UL) S.i2cHz = 1000000UL;

  if (S.mIntro.length()  > MELODY_MAX_LEN) S.mIntro  = S.mIntro.substring(0, MELODY_MAX_LEN);
  if (S.mNotify.length() > MELODY_MAX_LEN) S.mNotify = S.mNotify.substring(0, MELODY_MAX_LEN);
  if (S.mTap.length()    > MELODY_MAX_LEN) S.mTap    = S.mTap.substring(0, MELODY_MAX_LEN);
  if (S.mDouble.length() > MELODY_MAX_LEN) S.mDouble = S.mDouble.substring(0, MELODY_MAX_LEN);
  if (S.mTriple.length() > MELODY_MAX_LEN) S.mTriple = S.mTriple.substring(0, MELODY_MAX_LEN);
  if (S.mLong.length()   > MELODY_MAX_LEN) S.mLong   = S.mLong.substring(0, MELODY_MAX_LEN);

  if (S.appMode > APP_CLOCK) S.appMode = APP_MOCHI;
  if (S.noteTitle.length() == 0) S.noteTitle = "Notes";
  if (S.noteText.length() > NOTE_MAX_LEN) S.noteText = S.noteText.substring(0, NOTE_MAX_LEN);
  if (S.tz.length() == 0 || S.tz.length() > 48) S.tz = DEF_TZ;
}
