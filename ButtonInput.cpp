#include "ButtonInput.h"

ButtonInput::ButtonInput(uint8_t pin, unsigned long debounceMs, bool activeLow)
  : _pin(pin),
    _debounceMs(debounceMs),
    _activeLow(activeLow),
    _lastRaw(true),
    _stable(true),
    _changed(false),
    _rose(false),
    _fell(false),
    _lastDebounceTime(0) {}

void ButtonInput::begin() {
  pinMode(_pin, INPUT_PULLUP);
  _lastRaw = digitalRead(_pin);
  _stable = _lastRaw;
}

void ButtonInput::update(unsigned long now) {
  _changed = false;
  _rose = false;
  _fell = false;

  const bool raw = (digitalRead(_pin) == HIGH);

  if (raw != _lastRaw) {
    _lastDebounceTime = now;
    _lastRaw = raw;
  }

  if ((now - _lastDebounceTime) > _debounceMs) {
    if (_stable != raw) {
      const bool prev = _stable;
      _stable = raw;
      _changed = true;
      _rose = (prev == false && _stable == true);
      _fell = (prev == true && _stable == false);
    }
  }
}

bool ButtonInput::changed() const { return _changed; }

bool ButtonInput::pressed() const {
  // stable LOW means pressed when activeLow=true
  return _activeLow ? (_stable == false) : (_stable == true);
}

bool ButtonInput::rose() const { return _rose; }
bool ButtonInput::fell() const { return _fell; }