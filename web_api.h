/* =============================================================================
   web_api.h  -  Wi-Fi bring-up, the HTTP routes, and the REST API
   -----------------------------------------------------------------------------
   TRANSPORT CHOICE
       Requests come in form-encoded (application/x-www-form-urlencoded) and
       responses go out as JSON built by hand. That is on purpose: WebServer
       already parses form bodies for free via server.arg(), so the device
       never needs a JSON parsing library. Only the browser parses JSON, and
       it has one built in.

   APPLY VERSUS SAVE
       /api/gif, /api/snd and /api/wifi apply to the running device but do not
       touch flash. /api/save commits whatever is currently live. This is what
       lets you audition a melody or a speed and walk away without keeping it,
       and it mirrors how the original Mochi controller behaves.

       /api/hw is the exception: it saves and reboots, because pins are only
       bound at startup.

   ROUTES
       GET  /                 the controller page
       GET  /api/state        everything the UI needs to paint itself
       POST /api/gif          speed, delay, clip assignments, rotation mask
       POST /api/snd          melodies and the sound on/off flag
       POST /api/hw           pins, bus speed, flip   (saves + reboots)
       POST /api/wifi         credentials             (saves + reboots)
       POST /api/play?i=N     play clip N immediately
       POST /api/preview      m=<melody>  audition a melody
       POST /api/notify       play the stored notification melody
       POST /api/save         commit live settings to flash
       POST /api/defaults     erase flash, reboot to factory state
       POST /api/mode         m=0 mochi, m=1 clock   (applies and saves)
       POST /api/clock        tz, metric, title, text
       POST /api/refresh      force a weather + crypto fetch
       POST /api/reboot       restart
   ============================================================================= */
#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "settings.h"
#include "melody.h"
#include "player.h"
#include "clock_app.h"
#include "app_mode.h"
#include "web_ui.h"

WebServer server(80);
bool apMode = false;

/* --------------------------------------------------------------- utilities */
String jsonEscape(const String& s) {
  String o;
  o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if (c == '\n')        { o += "\\n"; }
    else if (c == '\r')        { }
    else if ((uint8_t)c < 0x20){ }
    else                       { o += c; }
  }
  return o;
}

/* server.arg() returns "" for a missing field, which would silently zero a
   setting. These helpers keep the current value when the field is absent.  */
static uint32_t argU(const char* k, uint32_t cur) {
  if (!server.hasArg(k)) return cur;
  return (uint32_t)strtoul(server.arg(k).c_str(), nullptr, 10);
}
static bool argB(const char* k, bool cur) {
  if (!server.hasArg(k)) return cur;
  String v = server.arg(k);
  return (v == "1" || v == "true" || v == "on");
}
static String argS(const char* k, const String& cur) {
  if (!server.hasArg(k)) return cur;
  return server.arg(k);
}

void scheduleReboot(uint16_t ms = 700) {
  server.send(200, "text/plain", "rebooting");
  delay(ms);
  ESP.restart();
}

/* ------------------------------------------------------------------ Wi-Fi */
String macSuffix() {
  String m = WiFi.macAddress();      // AA:BB:CC:DD:EE:FF
  m.replace(":", "");
  return m.substring(8);             // last four hex digits
}

void startAP() {
  apMode = true;
  WiFi.mode(WIFI_AP);
  String ssid = String(AP_PREFIX) + macSuffix();
  WiFi.softAP(ssid.c_str(), AP_PASS);
  Serial.printf("[wifi] hotspot %s  ->  http://%s/\n",
                ssid.c_str(), WiFi.softAPIP().toString().c_str());
}

bool startWifi() {
  if (S.ssid.length() == 0) {
    Serial.println("[wifi] no credentials stored");
    return false;
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(S.ssid.c_str(), S.pass.c_str());
  Serial.printf("[wifi] joining %s\n", S.ssid.c_str());

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT) delay(150);

  if (WiFi.status() == WL_CONNECTED) {
    apMode = false;
    Serial.printf("[wifi] joined, IP %s, RSSI %d\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  }
  Serial.println("[wifi] could not join");
  return false;
}

String currentIP() {
  return apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

/* ------------------------------------------------------------- GET /api/state */
/* Built as a plain String rather than through a JSON library. Kept separate
   from the handler so it can be exercised by the host-side test harness. */
String buildStateJson() {
  String j;
  j.reserve(3000);

  j += "{\"fw\":\"" FW_VERSION "\"";
  j += ",\"mac\":\"" + WiFi.macAddress() + "\"";
  j += ",\"ip\":\"" + currentIP() + "\"";
  j += ",\"ap\":" + String(apMode ? "true" : "false");
  j += ",\"ssid\":\"" + jsonEscape(S.ssid) + "\"";
  j += ",\"rssi\":" + String(apMode ? 0 : WiFi.RSSI());
  j += ",\"heap\":" + String((unsigned long)ESP.getFreeHeap());
  j += ",\"up\":" + String(millis() / 1000UL);
  j += ",\"display\":" + String(displayOK ? "true" : "false");
  j += ",\"mode\":" + String(S.appMode);
  j += ",\"screen\":" + String(clockScreen);
  j += ",\"addr\":\"0x" + String(displayAddr, HEX) + "\"";

  j += ",\"anims\":[";
  for (uint8_t i = 0; i < ANIM_COUNT; i++) {
    String nm = String(ANIMS[i].name);
    nm.trim();
    uint16_t bytes = pgm_read_word(&ANIMS[i].offsets[ANIMS[i].frames]);
    if (i) j += ",";
    j += "{\"n\":\"" + jsonEscape(nm) + "\",\"f\":" + String(ANIMS[i].frames) +
         ",\"b\":" + String(bytes) + "}";
  }
  j += "]";

  j += ",\"presets\":[";
  for (uint8_t i = 0; i < MELODY_PRESET_COUNT; i++) {
    if (i) j += ",";
    j += "{\"n\":\"" + String(MELODY_PRESETS[i].name) + "\",\"s\":\"" +
         String(MELODY_PRESETS[i].seq) + "\"}";
  }
  j += "]";

  j += ",\"gif\":{\"speed\":" + String(S.gifSpeed) +
       ",\"delay\":" + String(S.gifDelay) +
       ",\"def\":" + String(S.gifDefault) +
       ",\"intro\":" + String(S.gifIntro) +
       ",\"tap\":" + String(S.gifTap) +
       ",\"dbl\":" + String(S.gifDouble) +
       ",\"trip\":" + String(S.gifTriple) +
       ",\"lng\":" + String(S.gifLong) +
       ",\"mask\":" + String(S.enabledMask) +
       ",\"neg\":" + String(S.negative ? "true" : "false") + "}";

  j += ",\"snd\":{\"on\":" + String(S.soundOn ? "true" : "false") +
       ",\"mIntro\":\""  + jsonEscape(S.mIntro)  + "\"" +
       ",\"mTap\":\""    + jsonEscape(S.mTap)    + "\"" +
       ",\"mDbl\":\""    + jsonEscape(S.mDouble) + "\"" +
       ",\"mTrip\":\""   + jsonEscape(S.mTriple) + "\"" +
       ",\"mLong\":\""   + jsonEscape(S.mLong)   + "\"" +
       ",\"mNotify\":\"" + jsonEscape(S.mNotify) + "\"}";

  j += ",\"hw\":{\"sda\":" + String(S.pinSda) +
       ",\"scl\":" + String(S.pinScl) +
       ",\"touch\":" + String(S.pinTouch) +
       ",\"buzz\":" + String(S.pinBuzzer) +
       ",\"i2c\":" + String((unsigned long)S.i2cHz) +
       ",\"flip\":" + String(S.flip ? "true" : "false") + "}";

  j += ",\"clk\":{\"tz\":\"" + jsonEscape(S.tz) + "\"" +
       ",\"metric\":" + String(S.metric ? "true" : "false") +
       ",\"title\":\"" + jsonEscape(S.noteTitle) + "\"" +
       ",\"text\":\"" + jsonEscape(S.noteText) + "\"}";

  j += ",\"live\":{\"city\":\"" + jsonEscape(wxCity) + "\"" +
       ",\"wx\":" + String(wxValid ? "true" : "false") +
       ",\"temp\":" + String(wxTemp, 1) +
       ",\"cond\":\"" + String(weatherWord(wxCode)) + "\"" +
       ",\"btc\":" + String(coins[0].ok ? money(coins[0].usd) : String("0")) +
       ",\"eth\":" + String(coins[1].ok ? money(coins[1].usd) : String("0")) +
       ",\"sol\":" + String(coins[2].ok ? money(coins[2].usd) : String("0")) + "}";

  j += ",\"stats\":{\"frames\":" + String((unsigned long)animFramesTotal()) +
       ",\"bytes\":" + String((unsigned long)animBytesTotal()) + "}";

  j += "}";
  return j;
}

void handleState() {
  server.send(200, "application/json", buildStateJson());
}

/* -------------------------------------------------------- POST /api/gif */
void handleGif() {
  uint8_t oldDefault = S.gifDefault;
  bool    oldNeg     = S.negative;

  S.gifSpeed    = (uint16_t)argU("speed", S.gifSpeed);
  S.gifDelay    = (uint8_t) argU("delay", S.gifDelay);
  S.gifDefault  = (uint8_t) argU("def",   S.gifDefault);
  S.gifIntro    = (uint8_t) argU("intro", S.gifIntro);
  S.gifTap      = (uint8_t) argU("tap",   S.gifTap);
  S.gifDouble   = (uint8_t) argU("dbl",   S.gifDouble);
  S.gifTriple   = (uint8_t) argU("trip",  S.gifTriple);
  S.gifLong     = (uint8_t) argU("lng",   S.gifLong);
  S.enabledMask = argU("mask", S.enabledMask);
  S.negative    = argB("neg", S.negative);

  settingsClamp(ANIM_COUNT);
  if (S.negative != oldNeg) displayApplyLive();

  // if the resting face changed and we are sitting on it, switch now.
  // Only meaningful while the Mochi app owns the screen.
  if (S.appMode == APP_MOCHI && S.gifDefault != oldDefault && playMode == MODE_LOOP)
    enterIdle();

  server.send(200, "text/plain", "ok");
}

/* -------------------------------------------------------- POST /api/snd */
void handleSnd() {
  S.soundOn  = argB("on", S.soundOn);
  S.mIntro   = argS("mIntro",  S.mIntro);
  S.mTap     = argS("mTap",    S.mTap);
  S.mDouble  = argS("mDbl",    S.mDouble);
  S.mTriple  = argS("mTrip",   S.mTriple);
  S.mLong    = argS("mLong",   S.mLong);
  S.mNotify  = argS("mNotify", S.mNotify);
  settingsClamp(ANIM_COUNT);
  if (!S.soundOn) melody.stop();
  server.send(200, "text/plain", "ok");
}

/* --------------------------------------------------------- POST /api/hw */
void handleHw() {
  S.pinSda    = (uint8_t)argU("sda",   S.pinSda);
  S.pinScl    = (uint8_t)argU("scl",   S.pinScl);
  S.pinTouch  = (uint8_t)argU("touch", S.pinTouch);
  S.pinBuzzer = (uint8_t)argU("buzz",  S.pinBuzzer);
  S.i2cHz     = argU("i2c", S.i2cHz);
  S.flip      = argB("flip", S.flip);
  settingsClamp(ANIM_COUNT);
  settingsSave();                    // pins only bind at startup
  scheduleReboot();
}

/* ------------------------------------------------------- POST /api/wifi */
void handleWifi() {
  String ssid = argS("ssid", S.ssid);
  String pass = server.arg("pass");
  S.ssid = ssid;
  if (pass.length()) S.pass = pass;  // blank means "leave the password alone"
  settingsSave();
  scheduleReboot();
}

/* -------------------------------------------------------- POST /api/play */
void handlePlay() {
  if (!server.hasArg("i")) { server.send(400, "text/plain", "need i"); return; }
  uint8_t i = (uint8_t)server.arg("i").toInt();
  if (i >= ANIM_COUNT) { server.send(400, "text/plain", "out of range"); return; }
  // Previewing a clip only makes sense with the Mochi app on screen, so
  // switch to it rather than playing into a display the clock owns.
  if (S.appMode != APP_MOCHI) setAppMode(APP_MOCHI, false);
  playClip(i, MODE_ONCE);
  server.send(200, "text/plain", "ok");
}

/* ----------------------------------------------------- POST /api/preview */
void handlePreview() {
  String m = server.arg("m");
  if (m.length() > MELODY_MAX_LEN) m = m.substring(0, MELODY_MAX_LEN);
  if (S.pinBuzzer != PIN_NONE) melody.play(m);   // auditions ignore soundOn
  server.send(200, "text/plain", "ok");
}

void handleNotify() {
  playMelody(S.mNotify);
  server.send(200, "text/plain", "ok");
}

/* -------------------------------------------------------- POST /api/mode */
void handleMode() {
  uint8_t m = (uint8_t)argU("m", S.appMode);
  if (m != S.appMode) setAppMode(m, true);
  server.send(200, "text/plain", modeName(S.appMode));
}

/* ------------------------------------------------------- POST /api/clock */
void handleClock() {
  String oldTz = S.tz;
  S.tz        = argS("tz",     S.tz);
  S.metric    = argB("metric", S.metric);
  S.noteTitle = argS("title",  S.noteTitle);
  S.noteText  = argS("text",   S.noteText);
  settingsClamp(ANIM_COUNT);

  rebuildNoteLines();
  if (S.tz != oldTz) clockApplyTimezone();
  wxValid = false;                 // units may have changed; force a refetch
  lastDraw = 0;
  server.send(200, "text/plain", "ok");
}

/* ----------------------------------------------------- POST /api/refresh */
void handleRefresh() {
  clockRefresh(true);
  lastDraw = 0;
  server.send(200, "text/plain", "ok");
}

/* --------------------------------------------------- save / reset routes */
void handleSave() {
  settingsSave();
  server.send(200, "text/plain", "saved");
}

void handleDefaults() {
  settingsErase();
  scheduleReboot();
}

void handleReboot() { scheduleReboot(); }

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleNotFound() {
  // Captive-portal friendliness: anything unknown lands on the controller.
  server.sendHeader("Location", "/");
  server.send(303);
}

/* ------------------------------------------------------------------ setup */
void webBegin() {
  server.on("/",             HTTP_GET,  handleRoot);
  server.on("/api/state",    HTTP_GET,  handleState);
  server.on("/api/gif",      HTTP_POST, handleGif);
  server.on("/api/snd",      HTTP_POST, handleSnd);
  server.on("/api/hw",       HTTP_POST, handleHw);
  server.on("/api/wifi",     HTTP_POST, handleWifi);
  server.on("/api/mode",     HTTP_POST, handleMode);
  server.on("/api/clock",    HTTP_POST, handleClock);
  server.on("/api/refresh",  HTTP_POST, handleRefresh);
  server.on("/api/play",     HTTP_POST, handlePlay);
  server.on("/api/preview",  HTTP_POST, handlePreview);
  server.on("/api/notify",   HTTP_POST, handleNotify);
  server.on("/api/save",     HTTP_POST, handleSave);
  server.on("/api/defaults", HTTP_POST, handleDefaults);
  server.on("/api/reboot",   HTTP_POST, handleReboot);
  server.onNotFound(handleNotFound);
  server.begin();

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[web] http://" MDNS_NAME ".local/");
  }
  Serial.printf("[web] http://%s/\n", currentIP().c_str());
}
