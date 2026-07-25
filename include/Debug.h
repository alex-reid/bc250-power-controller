#pragma once
#include <Arduino.h>
#include "config.h"

#if DEBUG_MODE
  #define DBG_BEGIN(baud)            Serial.begin(baud)
  #define DBG_PRINT(x)               Serial.print(x)
  #define DBG_PRINTLN(x)             Serial.println(x)
  #define DBG_PRINT_FMT(x, fmt)      Serial.print((x), (fmt))
  #define DBG_PRINTLN_FMT(x, fmt)    Serial.println((x), (fmt))
#else
  #define DBG_BEGIN(baud)            ((void)0)
  #define DBG_PRINT(x)               ((void)0)
  #define DBG_PRINTLN(x)             ((void)0)
  #define DBG_PRINT_FMT(x, fmt)      ((void)0)
  #define DBG_PRINTLN_FMT(x, fmt)    ((void)0)
#endif