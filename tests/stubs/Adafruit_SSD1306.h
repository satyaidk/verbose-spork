#pragma once
#include <Arduino.h>
#include <Wire.h>
#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
#define SSD1306_SWITCHCAPVCC 2
class Adafruit_SSD1306 {
 public:
  Adafruit_SSD1306(int,int,TwoWire*,int){}
  bool begin(uint8_t,uint8_t){ return true; }
  void clearDisplay(){}
  void display(){}
  void drawBitmap(int,int,uint8_t*,int,int,uint16_t){}
  void setRotation(uint8_t){}
  void invertDisplay(bool){}
  void dim(bool){}
  void setTextColor(uint16_t){}
  void setTextSize(uint8_t){}
  void setCursor(int,int){}
  void getTextBounds(const String&,int,int,int16_t*x,int16_t*y,uint16_t*w,uint16_t*h){*x=0;*y=0;*w=40;*h=8;}
  template<class T> void print(T){}
  void fillRect(int,int,int,int,uint16_t){}
  void drawRect(int,int,int,int,uint16_t){}
  void fillCircle(int,int,int,uint16_t){}
  void drawCircle(int,int,int,uint16_t){}
  void drawLine(int,int,int,int,uint16_t){}
  void drawFastHLine(int,int,int,uint16_t){}
  void drawPixel(int,int,uint16_t){}
  void fillTriangle(int,int,int,int,int,int,uint16_t){}
  uint8_t* getBuffer(){ static uint8_t b[1024]; return b; }
};
