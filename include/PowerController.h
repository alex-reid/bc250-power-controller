#pragma once
#include <Arduino.h>
#include "PulseLED.h"
#include "ButtonInput.h"

enum class ButtonMode : uint8_t {
  Standard,
  Passthrough
};

enum class SystemState : uint8_t {
  Off,
  Booting,
  Bc250On,
  On,
  Sleeping,
  ShuttingDown,
  Bc250Off
};

class PowerController {
public:
  PowerController();
  void begin();
  void update();

private:
  // Core state
  SystemState _state;
  ButtonMode _buttonMode;

  // Components
  PulseLED _statusLed;
  ButtonInput _button;

  // Button press tracking
  bool _pressHandled;
  unsigned long _buttonPressStart;

  // BC250 pulse state
  bool _bc250PulseActive;
  unsigned long _bc250PulseStart;
  unsigned long _bc250PulseDuration;

  // Startup sequence state
  bool _startupSequenceActive;
  unsigned long _startupAtxOnTime;

  // Helpers
  void enterState(SystemState newState);

  void setAtxPower(bool on);
  void setBc250Button(bool pressed);
  bool readBc250Powered() const;

  void startPowerOnSequence();
  void serviceStartupSequence(unsigned long now);

  void pulseBc250Button(unsigned long pressMs);
  void serviceBc250Pulse(unsigned long now);

  void serviceSerial();
  bool shouldPollSerial() const;

  void serviceRailTracking();
  void serviceButton(unsigned long now);
  void serviceLed();
  void serviceFinalPowerDown();

  void hardOff();
};