/**
 * @file       SimpleNBNTP.tpp
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_NTP_H_
#define SRC_SIMPLE_NB_NTP_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_NTP

template <class modemType>
class SimpleNBNTP {
 public:
  /*
   * NTP server functions
   */

 public:
  bool SimpleNBIsValidNumber(String str) {
    if (!(str.charAt(0) == '+' || str.charAt(0) == '-' ||
          isDigit(str.charAt(0))))
      return false;

    for (byte i = 1; i < str.length(); i++) {
      if (!(isDigit(str.charAt(i)) || str.charAt(i) == '.')) { return false; }
    }
    return true;
  }

  byte NTPServerSync(String server = "pool.ntp.org", byte TimeZone = 3) {
    return thisModem().NTPServerSyncImpl(server, TimeZone);
  }
  String ShowNTPError(byte error) {
    return thisModem().ShowNTPErrorImpl(error);
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
   * NTP server functions
   */
 protected:
  byte NTPServerSyncImpl(String server = "pool.ntp.org", byte TimeZone = 0) {
    // Set GPRS bearer profile to associate with NTP sync
    // this may fail, it's not supported by all modules
    thisModem().sendAT(GF("+CNTPCID=1"));
    thisModem().waitResponse(10000L);

    // Set NTP server and timezone
    thisModem().sendAT(GF("+CNTP=\""), server, "\",", String(TimeZone));
    if (thisModem().waitResponse(10000L) != 1) { return -1; }

    // Request network synchronization
    thisModem().sendAT(GF("+CNTP"));
    if (thisModem().waitResponse(10000L, GF("+CNTP:"))) {
      String result = thisModem().stream.readStringUntil('\n');
      result.trim();
      if (SimpleNBIsValidNumber(result)) { return result.toInt(); }
    } else {
      return -1;
    }
    return -1;
  }

  String ShowNTPErrorImpl(byte error) {
    switch (error) {
      case 1: return "Network time synchronization is successful";
      case 61: return "Network error";
      case 62: return "DNS resolution error";
      case 63: return "Connection error";
      case 64: return "Service response error";
      case 65: return "Service response timeout";
      default: return "Unknown error: " + String(error);
    }
  }
};

#endif  // SRC_SIMPLE_NB_NTP_H_
