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
    ledcWrite(_pin, 0);
  #else
    ledcSetup(_channel, 5000, 8);
    ledcAttachPin(_pin, _channel);
    ledcWrite(_channel, 0);
  #endif
#else
  pinMode(_pin, OUTPUT);
#endif
}

void PulseLED::setPeriod(unsigned long period) {
  _period = period;
}

void PulseLED::setRange(uint8_t minBrightness, uint8_t maxBrightness) {
  _minBrightness = minBrightness;
  _maxBrightness = maxBrightness;
}

void PulseLED::setBrightness(uint8_t brightness) {
#if defined(ARDUINO_ARCH_ESP32)
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(_pin, brightness);
  #else
    ledcWrite(_channel, brightness);
  #endif
#else
  analogWrite(_pin, brightness);
#endif
}

void PulseLED::update() {
  const unsigned long now = millis();
  const float phase = (float)(now % _period) / (float)_period;
  const float wave = (sin(phase * 2.0f * PI - PI / 2.0f) + 1.0f) * 0.5f;

  const uint8_t brightness =
      _minBrightness + (uint8_t)(wave * (_maxBrightness - _minBrightness));

  setBrightness(brightness);
}