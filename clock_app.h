/* =============================================================================
   clock_app.h  -  the second app: clock, weather, crypto, notes
   -----------------------------------------------------------------------------
   This is the Info Terminal sketch from earlier in the project, adapted to
   share this firmware's Settings, display and touch layer instead of owning
   them. The four screens, the data sources and the drawing are unchanged.

   FOUR SCREENS, cycled by a single tap:
       0  Clock    time, seconds, weekday, date        NTP, timezone from S.tz
       1  Weather  temperature, condition, min/max     ip-api + Open-Meteo
       2  Crypto   BTC / ETH / SOL with 24 h change    CoinGecko
       3  Notes    your own text, auto-paged           typed on the web page

   No API keys are needed for any of it.

   BLOCKING FETCHES
       HTTP GETs here are synchronous and can take a few seconds. That stalls
       loop(), so the web server and touch pad are unresponsive during a
       fetch. This is accepted rather than solved: fetches only happen while
       the clock app is on screen, they are rate limited, and the alternative
       (an async client or a second task) costs more complexity and flash
       than the problem is worth. It is the reason the animation player is
       never ticked in this mode - a stalled fetch would visibly stutter it.
   ============================================================================= */
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <vector>
#include "config.h"
#include "settings.h"
#include "display_mochi.h"

/* ------------------------------------------------------------------ state */
uint8_t clockScreen = 0;          // 0..CLOCK_SCREENS-1

String  wxCity  = "";
float   wxTemp = 0, wxMin = 0, wxMax = 0, wxWind = 0;
int     wxHum  = 0, wxCode = -1;
bool    wxValid = false;
double  geoLat = 0, geoLon = 0;
bool    geoValid = false;

struct Coin { const char* id; const char* tag; float usd; float chg; bool ok; };
Coin coins[3] = {
  { "bitcoin",  "BTC", 0, 0, false },
  { "ethereum", "ETH", 0, 0, false },
  { "solana",   "SOL", 0, 0, false }
};

std::vector<String> noteLines;
uint8_t  notePage = 0;
unsigned long lastWeather = 0, lastCrypto = 0, lastDraw = 0, lastNoteFlip = 0;
bool     timeConfigured = false;

/* ------------------------------------------------------------ note wrapping */
/* Wrap plain text into <= maxChars chunks, honouring existing newlines. */
void wrapInto(const String& src, uint8_t maxChars, std::vector<String>& out) {
  out.clear();
  int start = 0;
  while (start <= (int)src.length()) {
    int nl = src.indexOf('\n', start);
    String para = (nl < 0) ? src.substring(start) : src.substring(start, nl);
    para.replace("\r", "");
    if (para.length() == 0) out.push_back("");
    while (para.length() > 0) {
      if (para.length() <= maxChars) { out.push_back(para); break; }
      int cut = -1;
      for (int i = maxChars; i > 0; i--) if (para.charAt(i) == ' ') { cut = i; break; }
      if (cut <= 0) cut = maxChars;
      out.push_back(para.substring(0, cut));
      para = para.substring(cut);
      para.trim();
    }
    if (nl < 0) break;
    start = nl + 1;
  }
  if (out.empty()) out.push_back("");
}

void rebuildNoteLines() {
  wrapInto(S.noteText, 21, noteLines);
  notePage = 0;
}

/* ------------------------------------------------------------------ fetching */
bool fetchJson(const String& url, String& payload) {
  if (WiFi.status() != WL_CONNECTED) return false;
  int code = -1;

  if (url.startsWith("https")) {
    WiFiClientSecure client;
    client.setInsecure();          // no cert pinning for public price/weather feeds
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    http.setUserAgent("mochi-terminal");
    if (http.begin(client, url)) {
      code = http.GET();
      if (code == HTTP_CODE_OK) payload = http.getString();
      http.end();
    }
  } else {
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    if (http.begin(client, url)) {
      code = http.GET();
      if (code == HTTP_CODE_OK) payload = http.getString();
      http.end();
    }
  }
  if (code != HTTP_CODE_OK || payload.length() == 0) {
    Serial.printf("[clock] HTTP %d for %s\n", code, url.substring(0, 44).c_str());
    return false;
  }
  return true;
}

/* Minimal JSON scraping. These three endpoints return flat, predictable
   objects, so pulling a value by key is enough and saves ~30 KB of flash
   against linking a real JSON parser. */
static bool jsonNum(const String& src, const String& key, double& out) {
  int k = src.indexOf("\"" + key + "\"");
  if (k < 0) return false;
  int c = src.indexOf(':', k);
  if (c < 0) return false;
  int i = c + 1;
  // skip whitespace, a quote, or an opening bracket: Open-Meteo returns the
  // daily figures as one-element arrays, "temperature_2m_max":[34.1]
  while (i < (int)src.length() &&
         (src[i] == ' ' || src[i] == '"' || src[i] == '[')) i++;
  int j = i;
  while (j < (int)src.length() &&
         (isDigit(src[j]) || src[j] == '-' || src[j] == '+' ||
          src[j] == '.' || src[j] == 'e' || src[j] == 'E')) j++;
  if (j == i) return false;
  out = atof(src.substring(i, j).c_str());
  return true;
}

/* Slice out one flat object by key: jsonObj(payload, "current") returns the
   text between its braces. Needed because Open-Meteo sends "current_units"
   alongside "current", with the same field names but string values - scraping
   the whole payload would hit the units and find no number. */
static String jsonObj(const String& src, const char* key) {
  String k = String("\"") + key + "\":";
  int p = src.indexOf(k);
  if (p < 0) return String();
  int b = src.indexOf('{', p);
  if (b < 0) return String();
  int e = src.indexOf('}', b);
  if (e < 0) e = src.length();
  return src.substring(b, e);
}

static bool jsonStr(const String& src, const String& key, String& out) {
  int k = src.indexOf("\"" + key + "\"");
  if (k < 0) return false;
  int c = src.indexOf(':', k);
  if (c < 0) return false;
  int q1 = src.indexOf('"', c + 1);
  if (q1 < 0) return false;
  int q2 = src.indexOf('"', q1 + 1);
  if (q2 < 0) return false;
  out = src.substring(q1 + 1, q2);
  return true;
}

bool fetchLocation() {
  String p;
  if (!fetchJson("http://ip-api.com/json/?fields=status,city,lat,lon", p)) return false;
  String status;
  if (!jsonStr(p, "status", status) || status != "success") return false;
  jsonStr(p, "city", wxCity);
  double la = 0, lo = 0;
  if (!jsonNum(p, "lat", la) || !jsonNum(p, "lon", lo)) return false;
  geoLat = la; geoLon = lo; geoValid = true;
  Serial.printf("[clock] location %s (%.3f, %.3f)\n", wxCity.c_str(), geoLat, geoLon);
  return true;
}

bool fetchWeather() {
  if (!geoValid && !fetchLocation()) return false;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(geoLat, 4) +
               "&longitude=" + String(geoLon, 4) +
               "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m" +
               "&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto";
  if (!S.metric) url += "&temperature_unit=fahrenheit&wind_speed_unit=mph";

  String p;
  if (!fetchJson(url, p)) return false;

  double v;
  String cur = jsonObj(p, "current");
  if (cur.length() == 0) return false;

  if (jsonNum(cur, "temperature_2m",       v)) wxTemp = (float)v; else return false;
  if (jsonNum(cur, "relative_humidity_2m", v)) wxHum  = (int)v;
  if (jsonNum(cur, "weather_code",         v)) wxCode = (int)v;
  if (jsonNum(cur, "wind_speed_10m",       v)) wxWind = (float)v;

  String day = jsonObj(p, "daily");
  wxMax = wxTemp; wxMin = wxTemp;
  if (day.length()) {
    if (jsonNum(day, "temperature_2m_max", v)) wxMax = (float)v;
    if (jsonNum(day, "temperature_2m_min", v)) wxMin = (float)v;
  }
  wxValid = true;
  return true;
}

bool fetchCrypto() {
  String p;
  if (!fetchJson("https://api.coingecko.com/api/v3/simple/price"
                 "?ids=bitcoin,ethereum,solana&vs_currencies=usd"
                 "&include_24hr_change=true", p)) return false;

  bool any = false;
  for (int i = 0; i < 3; i++) {
    int k = p.indexOf("\"" + String(coins[i].id) + "\"");
    if (k < 0) { coins[i].ok = false; continue; }
    int end = p.indexOf('}', k);
    String obj = p.substring(k, end < 0 ? p.length() : end);
    double v;
    if (jsonNum(obj, "usd", v)) { coins[i].usd = (float)v; coins[i].ok = true; any = true; }
    if (jsonNum(obj, "usd_24h_change", v)) coins[i].chg = (float)v;
  }
  return any;
}

void clockRefresh(bool force) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (force || !wxValid || millis() - lastWeather > WEATHER_REFRESH)
    if (fetchWeather()) lastWeather = millis();
  if (force || !coins[0].ok || millis() - lastCrypto > CRYPTO_REFRESH)
    if (fetchCrypto()) lastCrypto = millis();
}

/* ------------------------------------------------------------------ drawing */
const char* weatherWord(int c) {
  switch (c) {
    case 0:  return "Clear";
    case 1:  return "Mostly clear";
    case 2:  return "Part cloudy";
    case 3:  return "Overcast";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 56: case 57: return "Frz drizzle";
    case 61: case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: case 67: return "Freezing rain";
    case 71: case 73: case 75: case 77: return "Snow";
    case 80: case 81: return "Showers";
    case 82: return "Heavy showers";
    case 85: case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "Storm + hail";
    default: return "--";
  }
}

void drawWeatherIcon(int x, int y, int c) {
  if (c == 0 || c == 1) {                                   // sun
    display.fillCircle(x + 10, y + 10, 5, SSD1306_WHITE);
    for (int a = 0; a < 8; a++) {
      float r = a * PI / 4;
      display.drawLine(x + 10 + cos(r) * 8, y + 10 + sin(r) * 8,
                       x + 10 + cos(r) * 11, y + 10 + sin(r) * 11, SSD1306_WHITE);
    }
    return;
  }
  display.fillCircle(x + 7,  y + 10, 5, SSD1306_WHITE);     // cloud body
  display.fillCircle(x + 14, y + 10, 6, SSD1306_WHITE);
  display.fillRect(x + 4, y + 11, 14, 5, SSD1306_WHITE);
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82))
    for (int i = 0; i < 3; i++)
      display.drawLine(x + 5 + i * 5, y + 18, x + 3 + i * 5, y + 23, SSD1306_WHITE);
  if (c >= 71 && c <= 77)
    for (int i = 0; i < 3; i++) display.fillCircle(x + 6 + i * 5, y + 20, 1, SSD1306_WHITE);
  if (c >= 95) {
    display.drawLine(x + 11, y + 17, x + 7,  y + 22, SSD1306_WHITE);
    display.drawLine(x + 7,  y + 22, x + 12, y + 21, SSD1306_WHITE);
    display.drawLine(x + 12, y + 21, x + 8,  y + 26, SSD1306_WHITE);
  }
}

String money(float v) {
  if (v >= 1000) return String((long)(v + 0.5f));
  if (v >= 1)    return String(v, 2);
  return String(v, 4);
}

void drawHeader(const char* title) {
  display.fillRect(0, 0, OLED_W, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(title);
  if (WiFi.status() == WL_CONNECTED) display.fillCircle(122, 5, 2, SSD1306_BLACK);
  else                               display.drawCircle(122, 5, 2, SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);
}

void drawScreenClock() {
  struct tm t;
  if (!getLocalTime(&t, 50)) {
    drawHeader("Clock");
    centerText("Waiting for time", 30, 1);
    return;
  }
  char hhmm[8], ss[4], date[24], wd[12];
  strftime(hhmm, sizeof(hhmm), "%H:%M", &t);
  strftime(ss,   sizeof(ss),   "%S",    &t);
  strftime(date, sizeof(date), "%d %b %Y", &t);
  strftime(wd,   sizeof(wd),   "%A",    &t);

  display.setTextSize(3);
  display.setCursor(8, 12);
  display.print(hhmm);
  display.setTextSize(1);
  display.setCursor(98, 28);
  display.print(ss);
  display.drawFastHLine(0, 42, OLED_W, SSD1306_WHITE);
  centerText(String(wd), 46, 1);
  centerText(String(date), 56, 1);
}

void drawScreenWeather() {
  drawHeader("Weather");
  if (!wxValid) {
    centerText(WiFi.status() == WL_CONNECTED ? "Loading..." : "No Wi-Fi", 32, 1);
    return;
  }
  drawWeatherIcon(2, 14, wxCode);
  display.setTextSize(2);
  display.setCursor(34, 16);
  display.print(String(wxTemp, 1));
  display.setTextSize(1);
  display.print(S.metric ? "C" : "F");
  display.setCursor(34, 34);
  display.print(weatherWord(wxCode));
  display.drawFastHLine(0, 44, OLED_W, SSD1306_WHITE);
  display.setCursor(2, 47);
  display.print(wxCity.substring(0, 12));
  display.setCursor(2, 56);
  display.print(String((int)round(wxMin)) + "/" + String((int)round(wxMax)) + "  " +
                String(wxHum) + "%  " + String((int)round(wxWind)) +
                (S.metric ? "km/h" : "mph"));
}

void drawScreenCrypto() {
  drawHeader("Crypto  USD");
  if (!coins[0].ok && !coins[1].ok && !coins[2].ok) {
    centerText(WiFi.status() == WL_CONNECTED ? "Loading..." : "No Wi-Fi", 32, 1);
    return;
  }
  int y = 15;
  for (int i = 0; i < 3; i++) {
    display.setTextSize(1);
    display.setCursor(2, y);
    display.print(coins[i].tag);
    if (!coins[i].ok) { display.setCursor(30, y); display.print("--"); y += 17; continue; }
    display.setCursor(28, y);
    display.print(money(coins[i].usd));

    String ch = (coins[i].chg >= 0 ? "+" : "") + String(coins[i].chg, 1) + "%";
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(ch, 0, y, &x1, &y1, &w, &h);
    display.setCursor(OLED_W - w - 2, y);
    display.print(ch);
    if (coins[i].chg >= 0)
      display.fillTriangle(OLED_W-w-10, y+6, OLED_W-w-6, y+6, OLED_W-w-8, y+1, SSD1306_WHITE);
    else
      display.fillTriangle(OLED_W-w-10, y+1, OLED_W-w-6, y+1, OLED_W-w-8, y+6, SSD1306_WHITE);

    y += 17;
    if (i < 2) display.drawFastHLine(0, y - 5, OLED_W, SSD1306_WHITE);
  }
}

void drawScreenNotes() {
  drawHeader(S.noteTitle.substring(0, 18).c_str());
  const uint8_t rows = 6;
  uint16_t pages = (noteLines.size() + rows - 1) / rows;
  if (pages == 0) pages = 1;
  if (notePage >= pages) notePage = 0;

  for (uint8_t r = 0; r < rows; r++) {
    uint16_t idx = notePage * rows + r;
    if (idx >= noteLines.size()) break;
    display.setTextSize(1);
    display.setCursor(0, 14 + r * 8);
    display.print(noteLines[idx]);
  }
  if (pages > 1)
    for (uint16_t pg = 0; pg < pages && pg < 8; pg++) {
      if (pg == notePage) display.fillCircle(125, 16 + pg * 6, 1, SSD1306_WHITE);
      else                display.drawPixel(125, 16 + pg * 6, SSD1306_WHITE);
    }
}

void clockRender() {
  if (!displayOK) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  switch (clockScreen) {
    case 0: drawScreenClock();   break;
    case 1: drawScreenWeather(); break;
    case 2: drawScreenCrypto();  break;
    default: drawScreenNotes();  break;
  }
  displayFinish();             // draws the hold bar if a long press is in flight
}

/* ------------------------------------------------------------- entry points */
void clockApplyTimezone() {
  configTzTime(S.tz.c_str(), "pool.ntp.org", "time.google.com", "time.nist.gov");
  timeConfigured = true;
  Serial.printf("[clock] timezone %s\n", S.tz.c_str());
}

void clockBegin() {
  clockScreen = 0;
  rebuildNoteLines();
  if (!timeConfigured) clockApplyTimezone();
  lastDraw = 0;
  lastNoteFlip = millis();
  clockRefresh(false);
}

void clockNextScreen() {
  clockScreen = (clockScreen + 1) % CLOCK_SCREENS;
  notePage = 0;
  lastNoteFlip = millis();
  lastDraw = 0;
}

void clockTick() {
  if (millis() - lastDraw > 400) {
    clockRender();
    lastDraw = millis();
  }

  if (clockScreen == 3 && millis() - lastNoteFlip > NOTE_PAGE_MS) {
    uint16_t pages = (noteLines.size() + 5) / 6;
    if (pages > 1) { notePage = (notePage + 1) % pages; lastDraw = 0; }
    lastNoteFlip = millis();
  }

  static unsigned long lastPoll = 0;
  if (millis() - lastPoll > 5000) {
    lastPoll = millis();
    clockRefresh(false);
  }
}
