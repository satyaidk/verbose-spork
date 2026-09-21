#pragma once
#include <Arduino.h>
#include <WiFi.h>
#define HTTP_CODE_OK 200
class HTTPClient {
 public:
  bool begin(WiFiClient&, const String&){ return true; }
  void setTimeout(int){}
  void setUserAgent(const String&){}
  int  GET(){ return HTTP_CODE_OK; }
  String getString(){ return String("{}"); }
  void end(){}
};
