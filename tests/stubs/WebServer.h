#pragma once
#include <Arduino.h>
#define HTTP_GET 1
#define HTTP_POST 2
typedef void (*THandler)();
class WebServer {
 public:
  WebServer(int){}
  void on(const char*,int,THandler){}
  void on(const char*,THandler){}
  void onNotFound(THandler){}
  void begin(){}
  void handleClient(){}
  void send(int,const char*,const String& =String()){}
  void send(int){}
  void send_P(int,const char*,const char*){}
  void sendHeader(const char*,const String&){}
  String arg(const char*){ return String(); }
  bool hasArg(const char*){ return false; }
};
