#include "PowerController.h"
#include "config.h"
#include "serialComms.h"
#include "Debug.h"

PowerController::PowerController()
  : _state(SystemState::Off),
    _buttonMode(ButtonMode::Standard),
    _statusLed(LED_PIN),
    _button(BUTTON_PIN, DEBOUNCE_MS, true),
    _pressHandled(false),
    _buttonPressStart(0),
    _bc250PulseActive(false),
    _bc250PulseStart(0),
    _bc250PulseDuration(0),
    _startupSequenceActive(false),
    _startupAtxOnTime(0) {}

void PowerController::begin() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BC_250_POWERED_PIN, INPUT);
  pinMode(ATX_P_ON_PIN, OUTPUT);
  pinMode(BC_250_BUTTON_PIN, OUTPUT);

  digitalWrite(ATX_P_ON_PIN, LOW);
  digitalWrite(BC_250_BUTTON_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  _statusLed.begin();
  _statusLed.setPeriod(LED_PULSE_MS);
  _statusLed.setRange(LED_MIN_BRIGHTNESS, LED_MAX_BRIGHTNESS);

  _button.begin();

  // Optional debug console
  DBG_BEGIN(115200);
  #if DEBUG_MODE
    DBG_PRINTLN(F("--- DEBUG MODE ACTIVE ---"));
  #endif

  // Required BC250 serial receiver
  initSerialComms(SERIAL_RX, SERIAL_TX, DEBUG_MODE);

  enterState(SystemState::Off);
}

void PowerController::update() {
  const unsigned long now = millis();

  serviceBc250Pulse(now);
  serviceStartupSequence(now);
  serviceRailTracking();
  serviceButton(now);

  if (shouldPollSerial()) {
    serviceSerial();
  }

  serviceLed();
  serviceFinalPowerDown();
}

bool PowerController::shouldPollSerial() const {
  switch (_state) {
    case SystemState::Bc250On:
    case SystemState::On:
    case SystemState::Sleeping:
    case SystemState::ShuttingDown:
      return true;
    default:
      return false;
  }
}

void PowerController::serviceSerial() {
  const int command = checkSerialCommand();
  if (command < 0) return;

  switch ((byte)command) {
    case 0x00:
      enterState(SystemState::On);
      break;
    case 0xFF:
      enterState(SystemState::ShuttingDown);
      break;
    case 0x20:
      enterState(SystemState::Sleeping);
      break;
    default:
      break;
  }
}

void PowerController::serviceRailTracking() {
  const bool bc250Powered = readBc250Powered();

  if (!bc250Powered &&
      (_state == SystemState::Bc250On ||
       _state == SystemState::On ||
       _state == SystemState::Sleeping ||
       _state == SystemState::ShuttingDown)) {
    enterState(SystemState::Bc250Off);
  }

  if (bc250Powered && _state == SystemState::Booting) {
    enterState(SystemState::Bc250On);
  }
}

void PowerController::serviceButton(unsigned long now) {
  _button.update(now);

  if (_button.changed()) {
    if (_button.fell()) {
      _buttonPressStart = now;
      _pressHandled = false;
      #if DEBUG_MODE
        DBG_PRINT(F("[Button] Pressed on pin "));
        DBG_PRINTLN(BUTTON_PIN);
      #endif
    } else if (_button.rose()) {
      const unsigned long pressDuration = now - _buttonPressStart;
      _pressHandled = false;
      #if DEBUG_MODE
        DBG_PRINT(F("[Button] Released on pin "));
        DBG_PRINT(BUTTON_PIN);
        DBG_PRINT(F(" | duration: "));
        DBG_PRINT(pressDuration);
        DBG_PRINTLN(F(" ms"));
      #endif
    }
  }

  const bool pressed = _button.pressed();

  // Global long-press hard off in all states
//  if (pressed && !_pressHandled && (now - _buttonPressStart >= LONG_PRESS_MS)) {
  if (pressed && (now - _buttonPressStart >= LONG_PRESS_MS)) {
    #if DEBUG_MODE
      DBG_PRINTLN(F("[Power] Global long press -> Hard OFF"));
    #endif
    _pressHandled = true;
    hardOff();
    return;
  }

  if (_pressHandled) return;

  if (_buttonMode == ButtonMode::Standard) {
    if (_state == SystemState::Off && pressed) {
      startPowerOnSequence();
      _pressHandled = true;
    }
  } else {
    // Passthrough mode, unless we're running an internal pulse.
    if (!_bc250PulseActive) {
      setBc250Button(pressed);
    }
  }
}

void PowerController::serviceLed() {
  switch (_state) {
    case SystemState::Off:
      digitalWrite(LED_PIN, LOW);
      break;
    case SystemState::On:
      digitalWrite(LED_PIN, HIGH);
      break;
    case SystemState::Booting:
    case SystemState::Bc250On:
    case SystemState::Sleeping:
    case SystemState::ShuttingDown:
    case SystemState::Bc250Off:
      _statusLed.update();
      break;
  }
}

void PowerController::serviceFinalPowerDown() {
  if (_state == SystemState::Bc250Off) {
    setAtxPower(false);
    enterState(SystemState::Off);
  }
}

void PowerController::enterState(SystemState newState) {
  if (_state == newState) return;

  #if DEBUG_MODE
    DBG_PRINT(F("[State] "));
    DBG_PRINT((int)_state);
    DBG_PRINT(F(" -> "));
    DBG_PRINTLN((int)newState);
  #endif

  _state = newState;

  switch (_state) {
    case SystemState::Off:
    case SystemState::Booting:
    case SystemState::Bc250Off:
      _buttonMode = ButtonMode::Standard;
      break;
    case SystemState::Bc250On:
    case SystemState::On:
    case SystemState::Sleeping:
    case SystemState::ShuttingDown:
      _buttonMode = ButtonMode::Passthrough;
      break;
  }
}

void PowerController::setAtxPower(bool on) {
  digitalWrite(ATX_P_ON_PIN, on ? HIGH : LOW);
  #if DEBUG_MODE
    DBG_PRINT(F("[Power] ATX "));
    DBG_PRINTLN(on ? F("ON") : F("OFF"));
  #endif
}

void PowerController::setBc250Button(bool pressed) {
  digitalWrite(BC_250_BUTTON_PIN, pressed ? HIGH : LOW);
}

bool PowerController::readBc250Powered() const {
  return digitalRead(BC_250_POWERED_PIN) == HIGH;
}

void PowerController::startPowerOnSequence() {
  setAtxPower(true);
  _startupAtxOnTime = millis();
  _startupSequenceActive = true;
  enterState(SystemState::Booting);

  #if DEBUG_MODE
    DBG_PRINT(F("[Power] Startup sequence begun, warmup "));
    DBG_PRINT(ATX_WARMUP_MS);
    DBG_PRINTLN(F("ms"));
  #endif
}

void PowerController::serviceStartupSequence(unsigned long now) {
  if (!_startupSequenceActive) return;

  if (now - _startupAtxOnTime >= ATX_WARMUP_MS) {
    pulseBc250Button(BC250_PRESS_MS);
    _startupSequenceActive = false;

    #if DEBUG_MODE
      DBG_PRINTLN(F("[Power] Warmup complete, BC250 pulse triggered"));
    #endif
  }
}

void PowerController::pulseBc250Button(unsigned long pressMs) {
  _bc250PulseActive = true;
  _bc250PulseStart = millis();
  _bc250PulseDuration = pressMs;
  setBc250Button(true);

  #if DEBUG_MODE
    DBG_PRINT(F("[Power] BC250 pulse start "));
    DBG_PRINT(pressMs);
    DBG_PRINTLN(F("ms"));
  #endif
}

void PowerController::serviceBc250Pulse(unsigned long now) {
  if (!_bc250PulseActive) return;

  if (now - _bc250PulseStart >= _bc250PulseDuration) {
    setBc250Button(false);
    _bc250PulseActive = false;

    #if DEBUG_MODE
      DBG_PRINTLN(F("[Power] BC250 pulse end"));
    #endif
  }
}

void PowerController::hardOff() {
  setBc250Button(false);
  _bc250PulseActive = false;
  _startupSequenceActive = false;
  setAtxPower(false);
  enterState(SystemState::Off);
}
