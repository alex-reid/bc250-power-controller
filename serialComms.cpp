#include "serialComms.h"
#include "Debug.h"
#include <SoftwareSerial.h>
#include <ctype.h>
#include <stdlib.h>

static SoftwareSerial* g_port = nullptr;
static bool g_debugMode = false;

// No heap allocation: static instance created on first init call.
static SoftwareSerial* acquirePort(uint8_t rxPin, uint8_t txPin) {
  static SoftwareSerial port(rxPin, txPin);
  return &port;
}

void initSerialComms(uint8_t rxPin, uint8_t txPin, bool debugEnable) {
  g_debugMode = debugEnable;
  g_port = acquirePort(rxPin, txPin);
  g_port->begin(9600);
}

int checkSerialCommand() {
  if (g_port == nullptr) return -1;

  static bool control = false;
  static bool startRx = false;
  static char hexBuffer[3];
  static uint8_t hexCount = 0;

  g_port->listen();

  while (g_port->available() > 0) {
    const char inByte = (char)g_port->read();

    if (inByte == 'c') {
      control = true;
      startRx = false;
      hexCount = 0;
      continue;
    }

    if (control && !startRx) {
      if (inByte == ':') {
        startRx = true;
        if (g_debugMode) DBG_PRINTLN(F("[Debug] c: prefix matched. Catching Hex..."));
      } else {
        control = false;
      }
      continue;
    }

    if (control && startRx) {
      if (!isxdigit((unsigned char)inByte)) {
        // Malformed frame, reset parser state.
        control = false;
        startRx = false;
        hexCount = 0;
        continue;
      }

      hexBuffer[hexCount++] = inByte;

      if (hexCount == 2) {
        hexBuffer[2] = '\0';
        const byte commandValue = (byte)strtol(hexBuffer, nullptr, 16);

        if (g_debugMode) {
          DBG_PRINT(F("[Debug] Parsed string '"));
          DBG_PRINT(hexBuffer);
          DBG_PRINT(F("' to Byte: 0x"));
          DBG_PRINTLN_FMT(commandValue, HEX);
        }

        control = false;
        startRx = false;
        hexCount = 0;

        return (int)commandValue;
      }
    }
  }

  return -1;
}