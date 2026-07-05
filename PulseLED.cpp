#include "PulseLED.h"
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

PulseLED::PulseLED(uint8_t pin) : _pin(pin) {}

void PulseLED::begin() {
  pinMode(_pin, OUTPUT);
}

void PulseLED::setPeriod(unsigned long period) {
  _period = period;
}

void PulseLED::setRange(uint8_t minBrightness, uint8_t maxBrightness) {
  _minBrightness = minBrightness;
  _maxBrightness = maxBrightness;
}

void PulseLED::update() {
  const unsigned long now = millis();
  const float phase = (float)(now % _period) / (float)_period;
  const float wave = (sin(phase * 2.0f * PI - PI / 2.0f) + 1.0f) * 0.5f;

  const uint8_t brightness =
      _minBrightness + (uint8_t)(wave * (_maxBrightness - _minBrightness));

  analogWrite(_pin, brightness);
}