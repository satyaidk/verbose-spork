/* =============================================================================
   melody.h  -  the "C4 100 20" melody language, and a non-blocking player
   -----------------------------------------------------------------------------
   FORMAT
       A melody is a flat list of triplets separated by whitespace:

           NOTE  DURATION  REST     NOTE  DURATION  REST     ...
           C4    100       20       D4S   100       20

       NOTE      letter A-G, optional S or # for sharp, then an octave digit
                 0 to 8.  R or P means a silent rest.
       DURATION  milliseconds the note sounds
       REST      milliseconds of silence after it

       Example:  C4 100 20 D4S 100 20 A4S 100 20

   This is the same grammar the original Mochi controller uses, so melodies
   written for it paste straight in.

   WHY IT IS PARSED LAZILY
       The player holds the melody as a String and walks it one triplet at a
       time from tick(). Nothing is pre-expanded into an array, so a 300
       character melody costs 300 bytes of RAM rather than a note table, and
       tick() never blocks. That matters because the animation engine needs
       the CPU every few milliseconds.
   ============================================================================= */
#pragma once

#include <Arduino.h>
#include "config.h"
#include "settings.h"   // playMelody() reads S.soundOn / S.pinBuzzer

/* ------------------------------------------------------------ note -> freq */
/* MIDI note number for the letter, before octave and sharp are applied.     */
static int noteBase(char c) {
  switch (c) {
    case 'C': return 0;
    case 'D': return 2;
    case 'E': return 4;
    case 'F': return 5;
    case 'G': return 7;
    case 'A': return 9;
    case 'B': return 11;
    default:  return -1;
  }
}

/* Returns 0 for a rest or anything unparseable, which the player treats as
   silence rather than an error. A typo in one note should not kill a melody. */
unsigned int noteToFreq(const String& tok) {
  if (tok.length() == 0) return 0;

  char c = toupper(tok.charAt(0));
  if (c == 'R' || c == 'P') return 0;             // explicit rest

  int base = noteBase(c);
  if (base < 0) return 0;

  /* The accidental may sit on either side of the octave digit. The original
     Mochi controller writes it after (A4S, D4S); most other notation writes
     it before (A#4). Both are accepted, so melodies copied from either place
     work unchanged. A trailing lowercase b means flat.                      */
  int  octave = 4;                                 // default if none given
  bool sharp = false, flat = false;

  for (size_t i = 1; i < tok.length(); i++) {
    char d = tok.charAt(i);
    if (d == 'S' || d == 's' || d == '#') sharp = true;
    else if (d == 'b')                    flat  = true;
    else if (isDigit(d))                  octave = d - '0';
  }
  if (sharp) base += 1;
  if (flat)  base -= 1;

  if (octave < 0) octave = 0;
  if (octave > 8) octave = 8;

  int midi = (octave + 1) * 12 + base;             // C4 = 60, A4 = 69
  double f = 440.0 * pow(2.0, (midi - 69) / 12.0);

  if (f < MELODY_MIN_HZ || f > MELODY_MAX_HZ) return 0;
  return (unsigned int)(f + 0.5);
}

/* ---------------------------------------------------------------- player */
class MelodyPlayer {
 public:
  void begin(uint8_t pin) { _pin = pin; }

  void setPin(uint8_t pin) {
    stop();
    _pin = pin;
  }

  void play(const String& seq) {
    stop();
    if (_pin == PIN_NONE) return;
    _seq = seq;
    _pos = 0;
    _phase = PH_NEXT;
    _at = millis();
  }

  void stop() {
    if (_phase != PH_IDLE && _pin != PIN_NONE) noTone(_pin);
    _phase = PH_IDLE;
    _seq = "";
    _pos = 0;
  }

  bool busy() const { return _phase != PH_IDLE; }

  /* Call from loop() as often as you like. Never blocks. */
  void tick() {
    if (_phase == PH_IDLE) return;
    unsigned long now = millis();

    if (_phase == PH_TONE) {
      if (now < _at) return;
      if (_pin != PIN_NONE) noTone(_pin);
      _phase = PH_REST;
      _at = now + _rest;
      return;
    }
    if (_phase == PH_REST) {
      if (now < _at) return;
      _phase = PH_NEXT;
    }
    if (_phase == PH_NEXT) {
      String note;
      long dur, rest;
      if (!nextTriplet(note, dur, rest)) { stop(); return; }

      unsigned int f = noteToFreq(note);
      if (f > 0 && _pin != PIN_NONE) tone(_pin, f);
      _rest  = (rest < 0) ? 0 : (unsigned long)rest;
      _phase = PH_TONE;
      _at    = now + ((dur < 1) ? 1 : (unsigned long)dur);
    }
  }

 private:
  enum Phase { PH_IDLE, PH_NEXT, PH_TONE, PH_REST };

  /* Pull the next whitespace-delimited token. Returns false at end of string. */
  bool nextToken(String& out) {
    while (_pos < _seq.length() && isspace((unsigned char)_seq.charAt(_pos))) _pos++;
    if (_pos >= _seq.length()) return false;
    int start = _pos;
    while (_pos < _seq.length() && !isspace((unsigned char)_seq.charAt(_pos))) _pos++;
    out = _seq.substring(start, _pos);
    return true;
  }

  /* A triplet is note + duration + rest. A truncated triplet at the end of
     the string ends the melody rather than playing a half-defined note.     */
  bool nextTriplet(String& note, long& dur, long& rest) {
    String a, b, c;
    if (!nextToken(a)) return false;
    if (!nextToken(b)) return false;
    if (!nextToken(c)) return false;
    note = a;
    dur  = b.toInt();
    rest = c.toInt();
    return true;
  }

  uint8_t       _pin   = PIN_NONE;
  String        _seq;
  unsigned int  _pos   = 0;
  Phase         _phase = PH_IDLE;
  unsigned long _at    = 0;
  unsigned long _rest  = 0;
};

MelodyPlayer melody;

/* ------------------------------------------------------------- presets ---- */
/* Shown as the "Sound sample" buttons in the web UI. All written for this
   project; none are transcriptions of existing tunes.                       */
struct MelodyPreset { const char* name; const char* seq; };

const MelodyPreset MELODY_PRESETS[] = {
  { "Ding",     "A5 120 40 E6 240 200" },
  { "Pop",      "C6 40 10 G6 60 100" },
  { "Chime",    "E5 90 10 G5 90 10 C6 200 150" },
  { "Tick",     "C7 25 10" },
  { "Wosh",     "G4 40 5 C5 40 5 G5 40 5 C6 60 80" },
  { "Click",    "E7 20 8" },
  { "Ping",     "B5 60 20 B6 120 120" },
  { "Bounce",   "C5 60 10 G4 50 10 E4 40 10 C4 90 100" },
  { "Alert",    "A5 100 60 A5 100 60 A5 150 200" },
  { "Startup",  "C5 90 20 E5 90 20 G5 90 20 C6 220 200" },
  { "Arpeggio", "C4 80 20 E4 80 20 G4 80 20 C5 80 20 E5 80 20 G5 160 160" },
  { "Fanfare",  "G4 100 20 C5 100 20 E5 100 20 G5 200 200" },
  { "Descend",  "G5 90 20 E5 90 20 C5 90 20 G4 200 200" },
  { "Question", "D5 100 30 G5 180 150" },
  { "Mystery",  "A4 140 40 C5 140 40 D5S 140 40 F5 260 220" },
  { "Soar",     "C5 70 10 D5 70 10 E5 70 10 G5 70 10 A5 70 10 C6 250 200" },
  { "Modern",   "F5 70 30 A5S 70 30 D6 70 30 F6 180 180" },
  { "Waltz",    "C5 120 40 G5 120 40 E5 240 200" },
};
const uint8_t MELODY_PRESET_COUNT = sizeof(MELODY_PRESETS) / sizeof(MELODY_PRESETS[0]);

/* Convenience wrappers used by the event handlers. */
inline void playMelody(const String& seq) {
  if (S.soundOn && S.pinBuzzer != PIN_NONE) melody.play(seq);
}
