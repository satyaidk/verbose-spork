#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
SerialT Serial; ESPClass ESP; TwoWire Wire; WiFiClass WiFi; MDNSClass MDNS;
static unsigned long _t = 0;
unsigned long millis(){ return _t; }
void adv(unsigned long ms){ _t += ms; }

/* Fake the touch pin so gestures can be driven from the test. */
static int g_pin = 0;
#define digitalRead(p) (g_pin)

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

int fails = 0;
void ck(bool c, const char* w){ printf("%-60s %s\n", w, c?"ok":"FAIL"); if(!c) fails++; }

/* Drive the pad: hold for `ms`, then release and settle past the tap window. */
void press(unsigned long ms){
  g_pin = 1;
  unsigned long end = _t + ms;
  while (_t < end) { adv(5); touchTick(); }
  g_pin = 0; touchTick();
}
void settle(){ for (int i=0;i<120;i++){ adv(5); touchTick(); } }
void taps(int n){ for (int i=0;i<n;i++){ press(80); if (i<n-1) for(int k=0;k<20;k++){adv(5);touchTick();} } settle(); }

/* Record which handler fired. */
const char* fired = "";
void hTap(){ fired="tap"; }  void hDbl(){ fired="dbl"; }  void hTrip(){ fired="trip"; }
void hLong(){ fired="long"; } void hSwitch(){ fired="switch"; }

int main(){
  settingsDefaults(); settingsClamp(ANIM_COUNT);
  S.pinTouch = 3; S.pinBuzzer = PIN_NONE;
  onTap=hTap; onDouble=hDbl; onTriple=hTrip; onLongAction=hLong; onModeSwitch=hSwitch;

  printf("--- gesture resolution ---\n");
  fired=""; taps(1); ck(!strcmp(fired,"tap"),    "one tap resolves as tap");
  fired=""; taps(2); ck(!strcmp(fired,"dbl"),    "two taps resolve as double");
  fired=""; taps(3); ck(!strcmp(fired,"trip"),   "three taps resolve as triple");
  fired=""; taps(4); ck(!strcmp(fired,"trip"),   "four taps clamp to triple, never crash");

  fired=""; press(20); settle();  ck(!strcmp(fired,""), "20 ms contact bounce is ignored");

  fired=""; press(3000); settle();
  ck(!strcmp(fired,"tap"), "3 s hold (under 5 s) still counts as a tap");

  fired=""; press(7000); settle();
  ck(!strcmp(fired,"long"), "7 s hold fires the long action on release");

  fired=""; press(20000); settle();
  ck(!strcmp(fired,"switch"), "20 s hold fires the mode switch");

  printf("--- the two thresholds must not both fire ---\n");
  // hold past 15 s and count how many different handlers ran
  int nLong=0, nSwitch=0;
  onLongAction   = [](){ };  // replaced below via counters
  struct C { static void l(){ } };
  // use static counters instead
  static int cl=0, cs=0;
  onLongAction = [](){ cl++; };
  onModeSwitch = [](){ cs++; };
  cl=0; cs=0; press(20000); settle();
  ck(cs==1, "mode switch fires exactly once on a 20 s hold");
  ck(cl==0, "long action does NOT fire on the way through 5 s");
  (void)nLong; (void)nSwitch;

  cl=0; cs=0; press(9000); settle();
  ck(cl==1 && cs==0, "9 s hold gives the long action and no switch");

  printf("--- hold progress bar ---\n");
  g_pin=1; unsigned long e=_t+4000; while(_t<e){adv(5);touchTick();}
  ck(g_holdPct==0, "no bar before 5 s");
  e=_t+2000; while(_t<e){adv(5);touchTick();}
  ck(g_holdPct>0 && g_holdPct<40, "bar appears just after 5 s, near empty");
  e=_t+8000; while(_t<e){adv(5);touchTick();}
  ck(g_holdPct>85, "bar nearly full approaching 15 s");
  e=_t+2000; while(_t<e){adv(5);touchTick();}
  ck(g_holdPct==0, "bar clears once the switch fires");
  g_pin=0; touchTick(); settle();

  printf("--- a hold must not leak a tap ---\n");
  cl=0; cs=0; onTap=hTap; fired="";
  press(7000); settle();
  ck(strcmp(fired,"tap")!=0, "the release after a long hold is not counted as a tap");

  printf("--- mode routing ---\n");
  onTap=gestureTap; onDouble=gestureDouble; onTriple=gestureTriple;
  onLongAction=gestureLongAction; onModeSwitch=gestureModeSwitch;
  S.appMode = APP_MOCHI; playerBegin();
  taps(1);
  ck(S.appMode==APP_MOCHI && curIdx==S.gifTap && playMode==MODE_ONCE,
     "mochi: tap plays the tap clip once");
  taps(3);
  ck(curIdx==S.gifTriple, "mochi: triple tap plays the triple clip");
  ck(frameIdx==0, "mochi: a reaction always restarts at frame 0");

  press(20000); settle();
  ck(S.appMode==APP_CLOCK, "15 s hold in mochi switches to the clock app");
  uint8_t before = clockScreen;
  taps(1);
  ck(clockScreen==(uint8_t)((before+1)%CLOCK_SCREENS), "clock: tap advances one screen");
  before = clockScreen; taps(2);
  ck(clockScreen==(uint8_t)((before+1)%CLOCK_SCREENS), "clock: double tap also advances one");
  press(20000); settle();
  ck(S.appMode==APP_MOCHI, "15 s hold in the clock switches back to mochi");
  ck(playMode==MODE_LOOP && curIdx==S.gifDefault,
     "returning to mochi lands on the resting face, no intro");

  printf("--- clock screens wrap ---\n");
  setAppMode(APP_CLOCK,false);
  for (int i=0;i<CLOCK_SCREENS;i++) clockNextScreen();
  ck(clockScreen==0, "screens cycle back to the clock after four taps");

  printf("--- JSON scraping ---\n");
  double v; String sv;
  String wx = "{\"current\":{\"time\":\"x\",\"temperature_2m\":31.4,"
              "\"relative_humidity_2m\":52,\"weather_code\":3,\"wind_speed_10m\":11.2},"
              "\"daily\":{\"temperature_2m_max\":[34.1],\"temperature_2m_min\":[24.8]}}";
  ck(jsonNum(wx,"temperature_2m",v) && v>31.3 && v<31.5, "weather: temperature parsed");
  ck(jsonNum(wx,"weather_code",v) && (int)v==3,          "weather: condition code parsed");
  String cur = jsonObj(wx,"current");
  ck(jsonNum(cur,"temperature_2m",v) && v>31.3 && v<31.5, "weather: scoped to the current object");
  String day = jsonObj(wx,"daily");
  ck(jsonNum(day,"temperature_2m_max",v) && v>34.0,      "weather: daily max parsed out of an array");
  String units = "{\"current_units\":{\"temperature_2m\":\"C\"},\"current\":{\"temperature_2m\":29.5}}";
  ck(jsonNum(jsonObj(units,"current"),"temperature_2m",v) && v>29.4 && v<29.6,
     "weather: current_units does not shadow current");
  String cg = "{\"bitcoin\":{\"usd\":98123.4,\"usd_24h_change\":-1.83},"
              "\"ethereum\":{\"usd\":3421.9,\"usd_24h_change\":2.4},"
              "\"solana\":{\"usd\":178.55,\"usd_24h_change\":0.9}}";
  int k = cg.indexOf("\"ethereum\""); String obj = cg.substring(k, cg.indexOf('}',k));
  ck(jsonNum(obj,"usd",v) && v>3421 && v<3422,           "crypto: ETH price scoped correctly");
  ck(jsonNum(obj,"usd_24h_change",v) && v>2.3 && v<2.5,  "crypto: ETH change parsed");
  String ip = "{\"status\":\"success\",\"city\":\"Hyderabad\",\"lat\":17.38,\"lon\":78.47}";
  ck(jsonStr(ip,"city",sv) && sv=="Hyderabad",           "location: city parsed");
  ck(jsonNum(ip,"lat",v) && v>17.3 && v<17.4,            "location: latitude parsed");
  ck(!jsonNum(ip,"missing",v),                            "absent key reports failure");

  printf("--- notes wrapping ---\n");
  S.noteText = "Buy milk and eggs from the shop\nCall Ravi about the PCB order";
  rebuildNoteLines();
  bool fits = true;
  for (size_t i=0;i<noteLines.size();i++) if (noteLines[i].length()>21) fits=false;
  ck(fits, "every wrapped line fits 21 characters");
  ck(noteLines.size()>=4, "long text wraps to several lines");

  printf("--- settings ---\n");
  S.gifTriple = 99; settingsClamp(ANIM_COUNT);
  ck(S.gifTriple==ANIM_NONE, "out-of-range triple clip becomes None");
  S.appMode = 7; settingsClamp(ANIM_COUNT);
  ck(S.appMode==APP_MOCHI, "invalid mode falls back to mochi");
  S.tz = ""; settingsClamp(ANIM_COUNT);
  ck(S.tz==DEF_TZ, "empty timezone falls back to the default");

  String j = buildStateJson();
  FILE* f=fopen("state.json","w"); fwrite(j.c_str(),1,j.length(),f); fclose(f);
  printf("   state JSON %u bytes\n", (unsigned)j.length());

  printf("\n%s  (%d failures)\n", fails?"SOME TESTS FAILED":"ALL TESTS PASSED", fails);
  return fails?1:0;
}
