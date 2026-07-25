#pragma once
#include <Arduino.h>

// ==========================================
// CONFIGURATION - EDIT HERE
// ==========================================
#define DEBUG_MODE 1

// ==========================================
// CONFIGURATION - ESP32 Dev Kit (NodeMCU-32S)
// ==========================================

// BC250 serial on ESP32 UART2 (Serial2)
// RX = from BC250 TX line
// TX can be any free GPIO (kept defined for UART configuration)
constexpr uint8_t SERIAL_RX = 16;
constexpr uint8_t SERIAL_TX = 17;

// GPIO assignments for NodeMCU-32S (ESP32 Dev Kit)
// Adjust to match your wiring.
constexpr uint8_t LED_PIN               = 32 ;  // pick a PWM-capable pin for PulseLED
constexpr uint8_t BC_250_POWERED_PIN    = 33;  // BC250 3.3V sense (through proper level/interface)
constexpr uint8_t ATX_P_ON_PIN          = 25;  // drives MOSFET for PSU P_ON
constexpr uint8_t BC_250_BUTTON_PIN     = 26;  // drives MOSFET for BC250 power button
constexpr uint8_t BUTTON_PIN            = 27; // front panel button (INPUT_PULLUP)

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
