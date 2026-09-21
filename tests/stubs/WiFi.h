#pragma once
#include <Arduino.h>
#define WIFI_AP 1
#define WIFI_STA 2
#define WL_CONNECTED 3
class WiFiClient { public: void setTimeout(int){} };
struct IPAddress { String toString() const { return String("192.168.4.1"); } };
class WiFiClass {
 public:
  String macAddress(){ return String("0C:4E:A0:66:E2:06"); }
  void mode(int){}
  void persistent(bool){}
  void setSleep(bool){}
  void disconnect(bool=false){}
  void reconnect(){}
  void begin(const char*,const char*){}
  int status(){ return WL_CONNECTED; }
  int RSSI(){ return -55; }
  bool softAP(const char*,const char*){ return true; }
  IPAddress softAPIP(){ return IPAddress(); }
  IPAddress localIP(){ return IPAddress(); }
};
extern WiFiClass WiFi;
