/**
 * @file       SimpleNBTime.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @copyright  Copyright (c) 2021 Henry Cheung 
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_TIME_H_
#define SRC_SIMPLE_NB_TIME_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_TIME

typedef struct {
  int year       = 0;
  int month      = 0;
  int day        = 0;
  int hour       = 0;
  int minute     = 0;
  int second     = 0;
  int timezone   = 0;
} DateTime_t;

// enum SimpleNBDateTimeFormat { DATE_FULL = 0, DATE_TIME = 1, DATE_DATE = 2 };

template <class modemType>
class SimpleNBTime {
 public:
  /*
   * Time functions
   */
  String getNetworkTime() {
    return thisModem().getNetworkTimeImpl();
  }
  bool getNetworkTime(DateTime_t& dt) {
    return thisModem().getNetworkTimeImpl(dt);
  }

  /*
   * CRTP Helper
   */
 protected:

  inline const modemType& thisModem() const {
    return static_cast<const modemType&>(*this);
  }

  inline modemType& thisModem() {
    return static_cast<modemType&>(*this);
  }

  String getNetworkTimeImpl() {
    thisModem().sendAT(GF("+CCLK?"));
    if (thisModem().waitResponse(2000L, GF("+CCLK: \"")) != 1) { return ""; }
    String res = thisModem().stream.readStringUntil('"');
    thisModem().waitResponse();  // Ends with OK
    return res;
  }

  bool getNetworkTimeImpl(DateTime_t& dt) {
    thisModem().sendAT(GF("+CCLK?"));
    if (thisModem().waitResponse(2000L, GF("+CCLK: \"")) != 1) { return false; }

    // Date & Time
    dt.year       = thisModem().streamGetIntBefore('/');
    if (dt.year < 2000) dt.year += 2000;
    dt.month      = thisModem().streamGetIntBefore('/');
    dt.day        = thisModem().streamGetIntBefore(',');
    dt.hour       = thisModem().streamGetIntBefore(':');
    dt.minute     = thisModem().streamGetIntBefore(':');
    dt.second     = thisModem().streamGetIntLength(2);
    char tzSign   = thisModem().stream.read();
    dt.timezone   = thisModem().streamGetIntBefore('\n');
    if (tzSign == '-') {
      dt.timezone = dt.timezone * -1;
    }
    // Final OK
    thisModem().waitResponse();
    return true;
  }
};

#endif  // SRC_SIMPLE_NB_TIME_H_
