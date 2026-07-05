#ifndef SERIAL_COMMS_H
#define SERIAL_COMMS_H

#include <Arduino.h>

// Sets up SoftwareSerial used to receive BC250 state commands.
void initSerialComms(uint8_t rxPin, uint8_t txPin, bool debugEnable);

// Returns parsed command byte (0x00, 0x20, 0xFF, etc.) or -1 if no command.
int checkSerialCommand();

#endif