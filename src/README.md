# bc250-power-controller

Arduino-based power controller for an AMD BC250 system using a Metalfish Flex500 PSU.

This project manages ATX power sequencing, BC250 button control, front-panel button passthrough, serial state signals from the host OS, and LED status indication via a non-blocking state machine.

## Features

- Non-blocking power control logic
- Explicit system state machine
- Front button debounce and long-press hard-off
- BC250 button pulse generation for startup
- Optional debug logging over hardware `Serial`
- Required BC250 command input over `SoftwareSerial`
- Modular structure (`.h/.cpp`) for maintainability

## Hardware Overview

Typical signal roles:

- `ATX_P_ON_PIN`: controls PSU on/off (via transistor/MOSFET interface)
- `BC_250_BUTTON_PIN`: simulates BC250 momentary power button (via transistor/MOSFET interface)
- `BC_250_POWERED_PIN`: senses BC250 3.3V rail (use safe level interfacing)
- `BUTTON_PIN`: front-panel momentary button (uses `INPUT_PULLUP`)
- `LED_PIN`: status LED (PWM-capable recommended for pulse effect)

> ⚠️ Electrical safety note: do not connect 3.3V/5V rails or ATX control lines directly without proper level shifting / transistor interfacing and common-ground planning.

## Serial Command Protocol

The controller listens for commands from the BC250 host side in this format:

- Prefix: `c:`
- Payload: 2 hex chars (one byte)

Examples:

- `c:00` → system booted (`On`)
- `c:20` → system sleeping (`Sleeping`)
- `c:ff` → system shutting down (`ShuttingDown`)

Parser behavior:

- Waits for `c:`
- Reads exactly 2 hex characters
- Converts to byte and dispatches state transition

## State Machine

Primary states:

- `Off`
- `Booting`
- `Bc250On`
- `On`
- `Sleeping`
- `ShuttingDown`
- `Bc250Off`

Behavior summary:

- `Off`: LED off, waits for button press
- `Booting`: ATX enabled, warmup timer running, then BC250 pulse
- `Bc250On`: BC250 rail is high, passthrough available
- `On`: system fully booted, LED solid on
- `Sleeping`: pulsing LED, passthrough active
- `ShuttingDown`: pulsing LED, passthrough active
- `Bc250Off`: BC250 rail dropped; ATX is turned off and returns to `Off`

### Serial polling window

Serial polling is intentionally limited to:

- `Bc250On`
- `On`
- `Sleeping`
- `ShuttingDown`

It is skipped in:

- `Off`
- `Booting`
- `Bc250Off`

## Button Behavior

- Debounced using a non-blocking debounce timer
- In `Standard` mode:
  - Press in `Off` starts power-on sequence
- In `Passthrough` mode:
  - Front button directly drives BC250 button line
- Global long press (`LONG_PRESS_MS`, default 6000 ms):
  - Forces hard-off in any state
  - Cancels active startup/pulse actions
  - Turns ATX off
  - Returns to `Off`

## Project Structure

- `bc250-power-v2.ino` – sketch entrypoint
- `PowerController.h/.cpp` – state machine and orchestration
- `ButtonInput.h/.cpp` – debounce + edge tracking
- `serialComms.h/.cpp` – `SoftwareSerial` command parser
- `PulseLED.h/.cpp` – non-blocking sine-wave LED pulse
- `config.h` – pins, timings, feature flags
- `Debug.h` – optional debug print macros

## Configuration

Edit constants in `config.h`:

- Pin assignments
- Debounce/press timing values
- `DEBUG_MODE` on/off
- LED pulse range/period

### UNO vs ATtiny84

Recommended workflow:

- Use **UNO** for easier bring-up and debug output
- Use **ATtiny84** for final embedded deployment

For ATtiny84:

- Keep `SoftwareSerial` enabled for BC250 input
- Hardware `Serial` debug is optional
- Verify your selected `LED_PIN` supports PWM in your board core/pin map
- Confirm pin numbering matches your installed ATtiny core conventions

## Build / Upload

1. Open project in Arduino IDE (or PlatformIO equivalent).
2. Select board and processor:
   - UNO for debug phase
   - ATtiny84 (with your chosen core) for final target
3. Verify `config.h` pin mapping matches your wiring.
4. Upload firmware.
5. Open serial monitor (if `DEBUG_MODE=1`) at `9600` baud for logs.

## Wiring Checklist

- Common ground between controller and BC250 interface circuitry
- Proper transistor/MOSFET stages for:
  - PSU `P_ON`
  - BC250 button emulation
- Safe voltage interfacing for BC250 rail sensing into MCU pin
- Front button wired to `BUTTON_PIN` with `INPUT_PULLUP` semantics (button to GND)

## Debugging Tips

- No state transitions:
  - Check BC250 rail sense polarity and pin mapping
- No serial command reactions:
  - Validate incoming frames are exactly `c:XX`
  - Confirm `SERIAL_RX` wiring and baud (`9600`)
- LED not pulsing:
  - Confirm `LED_PIN` is PWM-capable
- Random behavior on button:
  - Verify grounding and switch wiring
  - Increase `DEBOUNCE_MS` slightly if needed
- ATtiny-specific oddities:
  - Re-check board core pin numbering and timer/PWM capabilities

## Current Status

This controller is designed around non-blocking control flow and explicit state transitions so behavior remains deterministic and easier to extend as hardware integration evolves.
