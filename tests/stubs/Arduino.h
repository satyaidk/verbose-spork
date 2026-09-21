#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>
#include <string>
#include <cstdint>

#define PROGMEM
#define pgm_read_byte(p) (*(const uint8_t*)(p))
#define pgm_read_word(p) (*(const uint16_t*)(p))
#define F(x) x
#define FPSTR(x) x
#define HEX 16
#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0

class String {
 public:
  std::string s;
  String() {}
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& x) : s(x) {}
  String(char c) { s = std::string(1, c); }
  String(int v) { char b[24]; snprintf(b,24,"%d",v); s=b; }
  String(unsigned int v) { char b[24]; snprintf(b,24,"%u",v); s=b; }
  String(long v) { char b[32]; snprintf(b,32,"%ld",v); s=b; }
  String(unsigned long v) { char b[32]; snprintf(b,32,"%lu",v); s=b; }
  String(unsigned char v, int base) { char b[16]; if(base==16) snprintf(b,16,"%x",v); else snprintf(b,16,"%u",v); s=b; }
  String(double v, int d=2) { char b[32]; snprintf(b,32,"%.*f",d,v); s=b; }
  size_t length() const { return s.size(); }
  char charAt(size_t i) const { return i<s.size()? s[i] : 0; }
  String substring(int a) const { if((size_t)a>=s.size()) return String(); return String(s.substr(a)); }
  String substring(int a,int b) const { if((size_t)a>=s.size()) return String(); return String(s.substr(a, b-a)); }
  int indexOf(char c,int from=0) const { auto p=s.find(c,from); return p==std::string::npos?-1:(int)p; }
  int indexOf(const char* c,int from=0) const { auto p=s.find(c,from); return p==std::string::npos?-1:(int)p; }
  int indexOf(const String& c,int from=0) const { auto p=s.find(c.s,from); return p==std::string::npos?-1:(int)p; }
  int lastIndexOf(char c) const { auto p=s.rfind(c); return p==std::string::npos?-1:(int)p; }
  char operator[](int i) const { return (i>=0 && i<(int)s.size())? s[i] : 0; }
  void replace(const String&a,const String&b){ size_t p=0; while((p=s.find(a.s,p))!=std::string::npos){ s.replace(p,a.s.size(),b.s); p+=b.s.size(); } }
  void replace(const char*a,const char*b){ replace(String(a),String(b)); }
  void trim(){ size_t a=s.find_first_not_of(" \t\r\n"); size_t b=s.find_last_not_of(" \t\r\n");
               s = (a==std::string::npos)? "" : s.substr(a,b-a+1); }
  long toInt() const { return strtol(s.c_str(),nullptr,10); }
  const char* c_str() const { return s.c_str(); }
  void reserve(size_t n){ s.reserve(n); }
  bool startsWith(const char* p) const { return s.rfind(p,0)==0; }
  String& operator+=(const String& o){ s+=o.s; return *this; }
  String& operator+=(const char* o){ s+=o; return *this; }
  String& operator+=(char o){ s+=o; return *this; }
  bool operator==(const char* o) const { return s==o; }
  bool operator==(const String& o) const { return s==o.s; }
  bool operator!=(const String& o) const { return s!=o.s; }
};
inline String operator+(const String&a,const String&b){ String r=a; r+=b; return r; }
inline String operator+(const String&a,const char*b){ String r=a; r+=b; return r; }
inline String operator+(const char*a,const String&b){ String r=String(a); r+=b; return r; }

inline bool isDigit(char c){ return c>='0'&&c<='9'; }
unsigned long millis();
inline void delay(unsigned long){}
inline void pinMode(int,int){}
inline int digitalRead(int){ return 0; }
inline void tone(int,unsigned int){}
inline void tone(int,unsigned int,unsigned long){}
inline void noTone(int){}
inline long random(long n){ return n?rand()%n:0; }
inline long random(long a,long b){ return a+(rand()%(b-a)); }
inline void randomSeed(unsigned long){}
inline uint32_t esp_random(){ return rand(); }

struct SerialT {
  void begin(unsigned long){}
  void println(){ }
  template<class T> void println(T){ }
  template<class T> void print(T){ }
  int printf(const char*,...){ return 0; }
  int available(){ return 0; }
  String readStringUntil(char){ return String(); }
};
extern SerialT Serial;

struct ESPClass { unsigned long getFreeHeap(){return 200000;} void restart(){} };
extern ESPClass ESP;


/* --- time.h surface used by clock_app.h (uses the real struct tm) --- */
#include <time.h>
inline bool getLocalTime(struct tm* t, unsigned long = 0){
  time_t now = 1757500000; localtime_r(&now, t); return true; }
inline void configTzTime(const char*, const char*, const char* = 0, const char* = 0){}
#ifndef PI
#define PI 3.14159265358979323846
#endif
