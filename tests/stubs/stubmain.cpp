#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
SerialT Serial;
ESPClass ESP;
TwoWire Wire;
WiFiClass WiFi;
MDNSClass MDNS;
static unsigned long _t = 0;
unsigned long millis(){ return _t += 1; }
void setup(); void loop();
int main(){ setup(); for(int i=0;i<50;i++) loop(); printf("ran setup + 50 loops OK\n"); return 0; }
