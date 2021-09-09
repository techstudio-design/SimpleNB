/**
 * @file       SimpleNBCommon.h
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_COMMON_H_
#define SRC_SIMPLE_NB_COMMON_H_

// The current library version number
#define SIMPLENB_VERSION "1.0.0"

#if defined(SPARK) || defined(PARTICLE)
#include "Particle.h"
#elif defined(ARDUINO)
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#endif

#if defined(ARDUINO_DASH)
#include <ArduinoCompat/Client.h>
#else
#include <Client.h>
#endif

#ifndef SIMPLE_NB_YIELD_MS
#define SIMPLE_NB_YIELD_MS 0
#endif

#ifndef SIMPLE_NB_YIELD
#define SIMPLE_NB_YIELD() \
  { delay(SIMPLE_NB_YIELD_MS); }
#endif

#define SIMPLE_NB_ATTR_NOT_AVAILABLE \
  __attribute__((error("Not available on this modem type")))
#define SIMPLE_NB_ATTR_NOT_IMPLEMENTED __attribute__((error("Not implemented")))

#if defined(__AVR__) && !defined(__AVR_ATmega4809__)
#define SIMPLE_NB_PROGMEM PROGMEM
typedef const __FlashStringHelper* GsmConstStr;
#define GFP(x) (reinterpret_cast<GsmConstStr>(x))
#define GF(x) F(x)
#else
#define SIMPLE_NB_PROGMEM
typedef const char* GsmConstStr;
#define GFP(x) x
#define GF(x) x
#endif

#ifdef SIMPLE_NB_DEBUG
namespace {
template <typename T>
static void DBG_PLAIN(T last) {
  SIMPLE_NB_DEBUG.println(last);
}

template <typename T, typename... Args>
static void DBG_PLAIN(T head, Args... tail) {
  SIMPLE_NB_DEBUG.print(head);
  SIMPLE_NB_DEBUG.print(' ');
  DBG_PLAIN(tail...);
}

template <typename... Args>
static void DBG(Args... args) {
  SIMPLE_NB_DEBUG.print(GF("["));
  SIMPLE_NB_DEBUG.print(millis());
  SIMPLE_NB_DEBUG.print(GF("] "));
  DBG_PLAIN(args...);
}
}  // namespace
#else
#define DBG_PLAIN(...)
#define DBG(...)
#endif

template <class T>
const T& SimpleNBMin(const T& a, const T& b) {
  return (b < a) ? b : a;
}

template <class T>
const T& SimpleNBMax(const T& a, const T& b) {
  return (b < a) ? a : b;
}

template <class T>
uint32_t SimpleNBAutoBaud(T& SerialAT, uint32_t minimum = 9600,
                         uint32_t maximum = 115200) {
  static uint32_t rates[] = {115200, 57600,  38400, 19200, 9600,  74400, 74880,
                             230400, 460800, 2400,  4800,  14400, 28800};

  for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
    uint32_t rate = rates[i];
    if (rate < minimum || rate > maximum) continue;

    DBG("Trying baud rate", rate, "...");
    SerialAT.begin(rate);
    delay(10);
    for (int j = 0; j < 10; j++) {
      SerialAT.print("AT\r\n");
      String input = SerialAT.readString();
      if (input.indexOf("OK") >= 0) {
        DBG("Modem responded at rate", rate);
        return rate;
      }
    }
  }
  SerialAT.begin(minimum);
  return 0;
}

#endif  // SRC_SIMPLE_NB_COMMON_H_
