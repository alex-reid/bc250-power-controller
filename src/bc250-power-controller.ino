#include "PowerController.h"
#include "ControllerWakeupLib.h"
#include "esp_log.h"

// Wake detector instance in pure detection mode (no internal pin driving).
ControllerWakeupLib wakeup(true);

PowerController controller;

// Callback path: called immediately when an allowed controller attempts to connect.
// This is the trigger to power on the system from an "off" state.
void onControllerWake(const MacAddress &controllerMac) {
    controller.startPowerOnSequence();
    Serial.print("[APP] Wake detected from ");
    char macBuf[18];
    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
             controllerMac.bytes[0], controllerMac.bytes[1], controllerMac.bytes[2],
             controllerMac.bytes[3], controllerMac.bytes[4], controllerMac.bytes[5]);
    Serial.println(macBuf);
}

void setup() {
  controller.begin();
      // Load persisted config and register app-level wake callback.
    wakeup.begin();
    wakeup.setWakeCallback(onControllerWake);
    wakeup.updatePcMAC({{0xAC, 0xA7, 0xF1, 0xE4, 0x55, 0xB9}}); // PC's mac address.
    wakeup.clearControllers();
    wakeup.addController({{0x0C, 0x27, 0x56, 0x3D, 0xB1, 0x44}}); // PS5 controller mac address.
    wakeup.addController({{0xD0, 0x55, 0x09, 0x29, 0xCB, 0x92}}); // Switch Pro controller mac address.
}

void loop() {
  // If the system is off, enable Bluetooth to listen for controller wake attempts.
  // otherwise disable Bluetooth to avoid competing for ownership with the OS.
  if (controller._state == SystemState::Off) {
    wakeup.enableBluetooth();
  }else{
    wakeup.disableBluetooth();
  }
  controller.update();
  wakeup.process();
  delay(1);
}