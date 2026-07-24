#include "PulseLED.h"
#include <math.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include <esp_arduino_version.h>
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif

PulseLED::PulseLED(uint8_t pin) : _pin(pin) {}

#if defined(ARDUINO_ARCH_ESP32)
uint8_t PulseLED::nextChannel() {
  static uint8_t channel = 0;
  const uint8_t out = channel;
  channel = (channel + 1U) % 16U;
  return out;
}
#endif

void PulseLED::begin() {
#if defined(ARDUINO_ARCH_ESP32)
  _channel = nextChannel();

  #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(_pin, 5000, 8);
  #else
    ledcSetup(_channel, 5000, 8);
    ledcAttachPin(_pin, _channel);
  #endif
#else
  pinMode(_pin, OUTPUT);
#endif

  writeRaw(0);
}

void PulseLED::setPeriod(unsigned long period) {
  _period = period;
}

void PulseLED::setRange(uint8_t minBrightness, uint8_t maxBrightness) {
  _minBrightness = minBrightness;
  _maxBrightness = maxBrightness;
}

void PulseLED::setFadeDuration(unsigned long fadeDurationMs) {
  _fadeDurationMs = fadeDurationMs;
}

void PulseLED::writeRaw(uint8_t brightness) {
#if defined(ARDUINO_ARCH_ESP32)
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(_pin, brightness);
  #else
    ledcWrite(_channel, brightness);
  #endif
#else
  analogWrite(_pin, brightness);
#endif

  _currentBrightness = brightness;
}

void PulseLED::setBrightness(uint8_t brightness) {
  const unsigned long now = millis();

  if (brightness != _targetBrightness) {
    _fadeStartBrightness = _currentBrightness;
    _targetBrightness = brightness;
    _fadeStartMs = now;
    _fadeActive = (_fadeDurationMs > 0) && (_fadeStartBrightness != _targetBrightness);

    if (!_fadeActive) {
      writeRaw(_targetBrightness);
      return;
    }
  }

  if (_fadeActive) {
    const unsigned long elapsed = now - _fadeStartMs;
    if (elapsed >= _fadeDurationMs) {
      _fadeActive = false;
      writeRaw(_targetBrightness);
      return;
    }

    const int start = (int)_fadeStartBrightness;
    const int delta = (int)_targetBrightness - start;
    const int stepped = start + (int)((delta * (int)elapsed) / (int)_fadeDurationMs);
    writeRaw((uint8_t)stepped);
    return;
  }

  if (_currentBrightness != _targetBrightness) {
    writeRaw(_targetBrightness);
  }
}

void PulseLED::update() {
  const unsigned long now = millis();
  const float phase = (float)(now % _period) / (float)_period;
  const float wave = (sin(phase * 2.0f * PI - PI / 2.0f) + 1.0f) * 0.5f;

  const uint8_t brightness =
      _minBrightness + (uint8_t)(wave * (_maxBrightness - _minBrightness));

  _fadeActive = false;
  _targetBrightness = brightness;
  writeRaw(brightness);
}