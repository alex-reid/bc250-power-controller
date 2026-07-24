#pragma once
#include <Arduino.h>

// ==========================================
// CONFIGURATION - EDIT HERE
// ==========================================
#define DEBUG_MODE 0

// ==========================================
// CONFIGURATION - ESP32
// ==========================================

// Serial from BC250 (Hardware Serial2)
// RX = from BC250 TX line (GPIO 16)
// TX = to BC250 RX line  (GPIO 17) - not currently used by the parser
constexpr uint8_t SERIAL_RX = 16;
constexpr uint8_t SERIAL_TX = 17;

// Pins (ESP32 DevKit GPIO numbers)
constexpr uint8_t LED_PIN               = 25;  // PWM-capable pin for PulseLED
constexpr uint8_t BC_250_POWERED_PIN    = 34;  // BC250 3.3V sense (GPIO 34-39 are input-only; suitable here)
constexpr uint8_t ATX_P_ON_PIN          = 26;  // drives MOSFET for PSU P_ON
constexpr uint8_t BC_250_BUTTON_PIN     = 27;  // drives MOSFET for BC250 power button
constexpr uint8_t BUTTON_PIN            = 32;  // front panel button (INPUT_PULLUP)

/*
// ==========================================
// CONFIGURATION - ATTINY84
// ==========================================

// Serial from BC250 (SoftwareSerial)
// RX = from BC250 TX line
// TX is not used by your parser; keep assigned to any free pin
constexpr uint8_t SERIAL_RX = 8;
constexpr uint8_t SERIAL_TX = 9;

// Pins (ATTinyCore "Arduino pin numbers" for ATtiny84)
// NOTE: adjust if your core/package uses a different numbering map.
constexpr uint8_t LED_PIN               = 5 ;  // pick a PWM-capable pin for PulseLED
constexpr uint8_t BC_250_POWERED_PIN    = 2;  // BC250 3.3V sense (through proper level/interface)
constexpr uint8_t ATX_P_ON_PIN          = 1;  // drives MOSFET for PSU P_ON
constexpr uint8_t BC_250_BUTTON_PIN     = 0;  // drives MOSFET for BC250 power button
constexpr uint8_t BUTTON_PIN            = 10; // front panel button (INPUT_PULLUP)
*/

/*
// ==========================================
// CONFIGURATION - Arduino Uno
// ==========================================

// Serial from BC250 (SoftwareSerial)
constexpr uint8_t SERIAL_RX = 8;
constexpr uint8_t SERIAL_TX = 255; // "unused" placeholder; not connected

// Pins
constexpr uint8_t LED_PIN               = 9;
constexpr uint8_t BC_250_POWERED_PIN    = 5;
constexpr uint8_t ATX_P_ON_PIN          = 6;
constexpr uint8_t BC_250_BUTTON_PIN     = 4;
constexpr uint8_t BUTTON_PIN            = 7;
*/

// Timing constants
constexpr unsigned long DEBOUNCE_MS     = 30;
constexpr unsigned long BC250_PRESS_MS  = 100;
constexpr unsigned long LONG_PRESS_MS   = 6000;
constexpr unsigned long ATX_WARMUP_MS   = 400;

// LED pulse config
constexpr unsigned long LED_PULSE_MS    = 3000;
constexpr uint8_t LED_MIN_BRIGHTNESS    = 20;
constexpr uint8_t LED_MAX_BRIGHTNESS    = 255;
