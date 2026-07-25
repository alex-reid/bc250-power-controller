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

## Bluetooth Controller Wake

When the system is `Off`, the firmware enables Classic Bluetooth listener mode and waits for controller ACL connection attempts.

How it works:

- `ControllerWakeupLib` is initialized during startup and configured with:
  - A spoofed PC Bluetooth MAC (`updatePcMAC(...)`) so controllers target this device
  - An allow-list of controller MACs (`addController(...)`)
- In the main loop:
  - `Off` state: Bluetooth is enabled (`enableBluetooth()`)
  - Any non-`Off` state: Bluetooth is disabled (`disableBluetooth()`) to free BT resources
- On `ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT`:
  - Incoming controller MAC is checked against the allow-list
  - If allowed, wake is triggered through the registered callback
  - `PowerController` starts the normal startup sequence (`ATX` on, warmup, BC250 button pulse)

Notes:

- Wake is edge-triggered per connection attempt and only accepted from allowed controllers.
- Default controller MAC placeholders in `ControllerWakeupLib` are `00:00:00:00:00:00`; real MACs are set in `PowerController` setup.
- If wake does not trigger, verify both spoofed PC MAC and controller allow-list MACs match your actual hardware.

## Shell Scripts

There are some helper scripts in the `./shell_scripts` directory for finding controller and pc mac addresses.

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

## Future state TODOS

**1. Serial update MAC addresses** 

At the moment, the mac addresses for the bc250's bluetooth dongle and the wireless controllers are hardcoded in the `PowerController` library.

I would like to add a way for them to be updated via serial. My standard approach to serial commands (a `c:` followed by a single byte) will not suffice for this. the main flaw being if I was to send a mac address that contained a `c:**`, it would trigger the current code and parse the `**` as an instruction.

My thoughts are that I could add in a setup mode where when the command `c:40` is received, the esp switches the serial to a setup mode where a string representing the bc's mac and controller macs could be added. an example representation could be
```1a:2a:3a:4a:5a:6a;1b:2b:3b:4b:5b:6b;1c:2c:3c:4c:5c:6c```
where the `*a` values represent the bc's mac address and any following (`*b` and `*c`) values are controller MACs to be added.

this may be better handled with individual commands, something like
```pc=1a:2a:3a:4a:5a:6```
```ctl=1b:2b:3b:4b:5b:6b```

Also I'm sure there is a more logical way to send the mac addresses over seeing as they are 6 single byte values, but I'm not quite versed enough in serial comms to know what that would be.

**2. Return current setup via serial**

Currently the power controller is only set up to receive serial messages. This is mainly due to the limitation of the onboard serial on the BC250. There isn't a proper interrupt set for the serial on IRQ4, so messsages are not received by the BC250. It is possible to switch to polling to receive messages on IRQ0. This takes up cpu cycles and isn't desirable for long term use. However it could be appropriate for a setup program.

If a bash script was set up right it could:
1. turn on poll based serial rx on the BC250
2. send a query `c:41` to the esp32 to trigger a MAC dump function
3. listen and echo the response
4. turn off the serial polling and exit

**3. Add a setting for max LED brightness**

It would be nice to be able to set a maximum brightness for the power indicator LED via a serial command. This should be persisted to non-volatile memory.

**4. create a bash based setup program**

It would be good to wrap up everything into a nice bash setup script where users can set the max led brightness, change the pc mac address and add or remove controllers from the controller wakeup list. 
The BC250s mac address can be found with the `shell_scripts/get_pc_mac.sh` script, and `shell_script/find_controllers` is an example of how controllers could be discovered.

