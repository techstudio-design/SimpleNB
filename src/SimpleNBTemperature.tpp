/**
 * @file       SimpleNBTemperature.tpp
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_TEMPERATURE_H_
#define SRC_SIMPLE_NB_TEMPERATURE_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_TEMPERATURE

template <class modemType>
class SimpleNBTemperature {
 public:
  /*
   * Temperature functions
   */
  float getTemperature() {
    return thisModem().getTemperatureImpl();
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

  float getTemperatureImpl() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
};

#endif  // SRC_SIMPLE_NB_TEMPERATURE_H_
