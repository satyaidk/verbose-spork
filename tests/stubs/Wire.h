#pragma once
#include <Arduino.h>
class TwoWire {
 public:
  bool begin(int,int,uint32_t=100000){ return true; }
  void end(){}
  void setClock(uint32_t){}
  void beginTransmission(uint8_t){}
  uint8_t endTransmission(){ return 0; }
};
extern TwoWire Wire;
