/**
 * @file       SimpleNBGSMLocation.h
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_GSMLOCATION_H_
#define SRC_SIMPLE_NB_GSMLOCATION_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_GSM_LOCATION

typedef struct {
  float lat      = 0;
  float lon      = 0;
  float accuracy = 0;
  int year       = 0;
  int month      = 0;
  int day        = 0;
  int hour       = 0;
  int minute     = 0;
  int second     = 0;
} CellLBS_t;

template <class modemType>
class SimpleNBGSMLocation {
 public:
  /*
   * GSM Location functions
   */
  String getGsmLocation() {
    return thisModem().getGsmLocationImpl();
  }

  bool getGsmLocation(CellLBS_t& lbs) {
    return thisModem().getGsmLocationImpl(lbs);
  };

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

  String getGsmLocationImpl() {
    // AT+CLBS=<type>,<cid>
    // <type> 1 = location using 3 cell's information
    //        3 = get number of times location has been accessed
    //        4 = Get longitude latitude and date time
    thisModem().sendAT(GF("+CLBS=1,1"));
    // Should get a location code of "0" indicating success
    if (thisModem().waitResponse(120000L, GF("+CLBS: ")) != 1) { return ""; }
    int8_t locationCode = thisModem().streamGetIntLength(2);
    // 0 = success, else, error
    if (locationCode != 0) {
      thisModem().waitResponse();  // should be an ok after the error
      return "";
    }
    String res = thisModem().stream.readStringUntil('\n');
    thisModem().waitResponse();
    res.trim();
    return res;
  }

  bool getGsmLocationImpl(CellLBS_t& lbs) {
    // AT+CLBS=<type>,<cid>
    // <type> 1 = location using 3 cell's information
    //        3 = get number of times location has been accessed
    //        4 = Get longitude latitude and date time
    thisModem().sendAT(GF("+CLBS=4,1"));
    // Should get a location code of "0" indicating success
    if (thisModem().waitResponse(120000L, GF("+CLBS: ")) != 1) { return false; }
    int8_t locationCode = thisModem().streamGetIntLength(2);
    // 0 = success, else, error
    if (locationCode != 0) {
      thisModem().waitResponse();  // should be an ok after the error
      return false;
    }

    lbs.lat      = thisModem().streamGetFloatBefore(',');  // Latitude
    lbs.lon      = thisModem().streamGetFloatBefore(',');  // Longitude
    lbs.accuracy = thisModem().streamGetIntBefore(',');    // Positioning accuracy

    // Date & Time
    lbs.year     = thisModem().streamGetIntBefore('/');
    if (lbs.year < 2000) lbs.year += 2000;
    lbs.month    = thisModem().streamGetIntBefore('/');
    lbs.day      = thisModem().streamGetIntBefore(',');
    lbs.hour     = thisModem().streamGetIntBefore(':');
    lbs.minute   = thisModem().streamGetIntBefore(':');
    lbs.second   = thisModem().streamGetIntBefore('\n');

    // Final OK
    thisModem().waitResponse();
    return true;
  }
};

#endif  // SRC_SIMPLE_NB_GSMLOCATION_H_
