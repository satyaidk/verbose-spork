#pragma once
#include <Arduino.h>
class Preferences {
 public:
  bool begin(const char*,bool=false){ return true; }
  void end(){}
  void clear(){}
  uint16_t getUShort(const char*,uint16_t d=0){ return d; }
  uint8_t  getUChar (const char*,uint8_t d=0){ return d; }
  uint32_t getUInt  (const char*,uint32_t d=0){ return d; }
  bool     getBool  (const char*,bool d=false){ return d; }
  String   getString(const char*,String d=String()){ return d; }
  void putUShort(const char*,uint16_t){}
  void putUChar (const char*,uint8_t){}
  void putUInt  (const char*,uint32_t){}
  void putBool  (const char*,bool){}
  void putString(const char*,const String&){}
};
