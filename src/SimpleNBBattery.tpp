/**
 * @file       SimpleNBBattery.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy.
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_BATTERY_H_
#define SRC_SIMPLE_NB_BATTERY_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_BATTERY

typedef struct {
  uint8_t  chargeState = 0;
  uint8_t  percent     = 0;
  uint16_t milliVolts  = 0;
} Battery_t;

template <class modemType>
class SimpleNBBattery {
 public:
  /*
   * Battery functions
   */
  bool getBatteryStatus(Battery_t& batt) {
    return thisModem().getBatteryStatusImpl(batt);
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

  bool getBatteryStatusImpl(Battery_t& batt) {
    thisModem().sendAT(GF("+CBC"));
    if (thisModem().waitResponse(GF("+CBC:")) != 1) { return false; }
    batt.chargeState = thisModem().streamGetIntBefore(',');
    batt.percent     = thisModem().streamGetIntBefore(',');
    batt.milliVolts  = thisModem().streamGetIntBefore('\n');
    thisModem().waitResponse();
    return true;
  }
};

#endif  // SRC_SIMPLE_NB_BATTERY_H_
