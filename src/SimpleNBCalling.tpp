/**
 * @file       SimpleNBCalling.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy.
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SIMPLE_NB_CALLING_H_
#define SIMPLE_NB_CALLING_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_CALLING

template <class modemType>
class SimpleNBCalling {
 public:
  /*
   * Phone Call functions
   */
  bool callAnswer() {
    return thisModem().callAnswerImpl();
  }
  bool callNumber(const String& number) {
    return thisModem().callNumberImpl(number);
  }
  bool callHangup() {
    return thisModem().callHangupImpl();
  }
  bool dtmfSend(char cmd, int duration_ms = 100) {
    return thisModem().dtmfSendImpl(cmd, duration_ms);
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

  /*
   * Phone Call functions
   */
 protected:
  bool callAnswerImpl() {
    thisModem().sendAT(GF("A"));
    return thisModem().waitResponse() == 1;
  }

  // Returns true on pick-up, false on error/busy
  bool callNumberImpl(const String& number) {
    if (number == GF("last")) {
      thisModem().sendAT(GF("DL"));
    } else {
      thisModem().sendAT(GF("D"), number, ";");
    }
    int8_t status = thisModem().waitResponse(60000L, GF("OK"), GF("BUSY"),
                                             GF("NO ANSWER"), GF("NO CARRIER"));
    switch (status) {
      case 1: return true;
      case 2:
      case 3: return false;
      default: return false;
    }
  }

  bool callHangupImpl() {
    thisModem().sendAT(GF("H"));
    return thisModem().waitResponse() == 1;
  }

  // 0-9,*,#,A,B,C,D
  bool dtmfSendImpl(char cmd, int duration_ms = 100) {
    duration_ms = constrain(duration_ms, 100, 1000);

    thisModem().sendAT(GF("+VTD="),
                       duration_ms / 100);  // VTD accepts in 1/10 of a second
    thisModem().waitResponse();

    thisModem().sendAT(GF("+VTS="), cmd);
    return thisModem().waitResponse(10000L) == 1;
  }
};

#endif  // SIMPLE_NB_CALLING_H_
