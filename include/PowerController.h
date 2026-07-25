#pragma once
#include <Arduino.h>
#include "PulseLED.h"
#include "ButtonInput.h"
#include "ControllerWakeupLib.h"

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
  SystemState _state;
  static PowerController* _instance;
  
  // Core state
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
  
  // Controller wake detector
  ControllerWakeupLib _wakeup;
  
  // Helpers
  void enterState(SystemState newState);
  
  void startPowerOnSequence();
  void setAtxPower(bool on);
  void setBc250Button(bool pressed);
  bool readBc250Powered() const;

  void serviceStartupSequence(unsigned long now);

  void pulseBc250Button(unsigned long pressMs);
  void serviceBc250Pulse(unsigned long now);

  void serviceSerial();
  bool shouldPollSerial() const;
  void serviceWakeup();

  static void onControllerWake(const MacAddress &controllerMac);
  void handleControllerWake(const MacAddress &controllerMac);

  void serviceRailTracking();
  void serviceButton(unsigned long now);
  void serviceLed();
  void serviceFinalPowerDown();

  void hardOff();
};