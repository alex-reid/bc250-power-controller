#ifndef PULSE_LED_H
#define PULSE_LED_H

#include <Arduino.h>

class PulseLED {
public:
    explicit PulseLED(uint8_t pin);

    void begin();
    void update();
    void setBrightness(uint8_t brightness);
    void setFadeDuration(unsigned long fadeDurationMs);

    void setPeriod(unsigned long period);
    void setRange(uint8_t minBrightness, uint8_t maxBrightness);

private:
    uint8_t _pin;
    unsigned long _period = 2000;
    uint8_t _minBrightness = 0;
    uint8_t _maxBrightness = 255;
    uint8_t _currentBrightness = 0;
    uint8_t _targetBrightness = 0;
    uint8_t _fadeStartBrightness = 0;
    unsigned long _fadeStartMs = 0;
    unsigned long _fadeDurationMs = 300;
    bool _fadeActive = false;

    void writeRaw(uint8_t brightness);

#if defined(ARDUINO_ARCH_ESP32)
    uint8_t _channel = 0;

    static uint8_t nextChannel();
#endif
};

#endif