/* =============================================================================
   config.h  -  compile-time defaults and hard limits
   -----------------------------------------------------------------------------
   Nothing here is written to flash. These are the values a factory reset falls
   back to, plus a few limits the firmware enforces. Everything a user can
   change lives in Settings (settings.h) and is stored in NVS.
   ============================================================================= */
#pragma once

#define FW_NAME       "MOCHI"
#define FW_VERSION    "2.0.0"

/* ---- networking ---------------------------------------------------------- */
#define AP_PREFIX     "MOCHI"      // hotspot becomes MOCHI-A1B2
#define AP_PASS       "12345678"    // minimum 8 characters
#define MDNS_NAME     "mochi"       // -> http://mochi.local
#define WIFI_TIMEOUT  20000UL       // ms to wait before falling back to the hotspot
#define WIFI_RETRY    30000UL       // ms between reconnect attempts

/* ---- default pin map (your confirmed wiring) ----------------------------- */
#define DEF_PIN_SDA     20
#define DEF_PIN_SCL     21
#define DEF_PIN_TOUCH    1
#define DEF_PIN_BUZZER   2

// The web UI uses 22 to mean "not fitted", matching the convention used by the
// original Mochi controller. GPIO22 does not exist on the ESP32-C3, so it is a
// safe sentinel value.
#define PIN_NONE        22

/* ---- display ------------------------------------------------------------- */
#define OLED_W         128
#define OLED_H          64
#define FRAME_BYTES   1024          // 128 * 64 / 8
#define DEF_I2C_HZ  700000UL        // drop to 400000 if your module glitches

/* ---- playback ------------------------------------------------------------ */
#define DEF_GIF_SPEED   70          // ms per frame; 70 = ~14 FPS, the native rate
#define MIN_GIF_SPEED   20
#define MAX_GIF_SPEED  500
#define DEF_GIF_DELAY    5          // seconds of idle face between random clips
#define MAX_GIF_DELAY  120
#define ANIM_NONE      255          // "no animation selected"

/* ---- application modes ---------------------------------------------------
   Two apps share one device. The mode is persisted, so it boots back into
   whichever one you were last using.                                      */
#define APP_MOCHI       0           // animated face
#define APP_CLOCK       1           // clock / weather / crypto / notes

/* ---- touch gestures ------------------------------------------------------
   Two long-press thresholds share one physical hold, so they cannot both
   fire while held. See touch.h for the resolution:
       release under 5 s   -> counts as a tap (1, 2 or 3 of them)
       release 5 s to 15 s -> the in-mode long action
       still held at 15 s  -> mode switch, fires immediately              */
#define TAP_MIN_MS       40         // shorter than this is contact bounce
#define MULTI_TAP_MS    350         // window to wait for another tap
#define LONG_ACTION_MS 500         // in-mode long press
#define MODE_SWITCH_MS 1500        // hold this long to swap apps

#define ARM_TUNE   "C6 60 40"                        // 5 s reached, release now
#define SWITCH_TUNE "G4 90 20 C5 90 20 E5 180 140"   // 15 s reached, switching

/* ---- clock app ----------------------------------------------------------- */
// POSIX timezone string. The sign is inverted: India is IST-5:30.
// London "GMT0BST,M3.5.0/1,M10.5.0"   New York "EST5EDT,M3.2.0,M11.1.0"
#define DEF_TZ           "IST-5:30"
#define WEATHER_REFRESH  (15UL * 60UL * 1000UL)
#define CRYPTO_REFRESH   (60UL * 1000UL)
#define NOTE_PAGE_MS     (5UL * 1000UL)
#define HTTP_TIMEOUT     8000       // ms; a stalled fetch blocks the loop
#define NOTE_MAX_LEN     1400
#define CLOCK_SCREENS    4

/* ---- sound --------------------------------------------------------------- */
#define MELODY_MAX_LEN 320          // characters, per melody field
#define MELODY_MAX_HZ 8000
#define MELODY_MIN_HZ   40

/* ---- storage ------------------------------------------------------------- */
#define NVS_NAMESPACE "mochi"
