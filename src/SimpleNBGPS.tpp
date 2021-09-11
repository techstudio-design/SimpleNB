/**
 * @file       SimpleNBGPS.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @copyright  Copyright (c) 2021 Henry Cheung
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_GPS_H_
#define SRC_SIMPLE_NB_GPS_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_GPS

typedef struct {
  float lat      = 0;
  float lon      = 0;
  float speed    = 0;
  float alt      = 0;
  int   vsat     = 0;
  int   usat     = 0;
  float accuracy = 0;
  int   year     = 0;
  int   month    = 0;
  int   day      = 0;
  int   hour     = 0;
  int   minute   = 0;
  int   second   = 0;
} GPS_t;

template <class modemType>
class SimpleNBGPS {
 public:
  /*
   * GPS/GNSS/GLONASS location functions
   */
  bool enableGPS() {
    return thisModem().enableGPSImpl();
  }
  bool disableGPS() {
    return thisModem().disableGPSImpl();
  }
  String getGPS() {
    return thisModem().getGPSImpl();
  }
  bool getGPS(GPS_t& gps) {
    return thisModem().getGPSImpl(gps);
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
   * GPS/GNSS/GLONASS location functions
   */

  bool    enableGPSImpl() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
  bool    disableGPSImpl() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
  String  getGPSImpl() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
  bool    getGPSImpl(GPS_t& gps) SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
};


#endif  // SRC_SIMPLE_NB_GPS_H_
