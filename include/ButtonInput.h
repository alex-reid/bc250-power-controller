#pragma once
#include <Arduino.h>

class ButtonInput {
public:
  ButtonInput(uint8_t pin, unsigned long debounceMs, bool activeLow = true);

  void begin();
  void update(unsigned long now);

  bool changed() const;
  bool pressed() const;
  bool rose() const;   // released edge
  bool fell() const;   // pressed edge

private:
  uint8_t _pin;
  unsigned long _debounceMs;
  bool _activeLow;

  bool _lastRaw;
  bool _stable;
  bool _changed;
  bool _rose;
  bool _fell;

  unsigned long _lastDebounceTime;
};