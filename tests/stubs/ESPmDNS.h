#pragma once
#include <Arduino.h>
class MDNSClass { public: bool begin(const char*){return true;} void addService(const char*,const char*,int){} };
extern MDNSClass MDNS;
