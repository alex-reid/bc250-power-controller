#ifndef PULSE_LED_H
#define PULSE_LED_H

#include <Arduino.h>

class PulseLED {
public:
    explicit PulseLED(uint8_t pin);

    void begin();
    void update();

    void setPeriod(unsigned long period);
    void setRange(uint8_t minBrightness, uint8_t maxBrightness);

private:
    uint8_t _pin;
    unsigned long _period = 2000;
    uint8_t _minBrightness = 0;
    uint8_t _maxBrightness = 255;
};

#endif