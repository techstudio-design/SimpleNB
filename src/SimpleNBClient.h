/**
 * @file       SimpleNBClient.h
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_CLIENT_H_
#define SRC_SIMPLE_NB_CLIENT_H_

#if defined(SIMPLE_NB_MODEM_SIM7000)
#include "SimpleNBClientSIM7000.h"
typedef SimpleNBSim7000                   SimpleNB;
typedef SimpleNBSim7000::GsmClientSim7000 SimpleNBClient;

#elif defined(SIMPLE_NB_MODEM_SIM7000SSL)
#include "SimpleNBClientSIM7000SSL.h"
typedef SimpleNBSim7000SSL                            SimpleNB;
typedef SimpleNBSim7000SSL::GsmClientSim7000SSL       SimpleNBClient;
typedef SimpleNBSim7000SSL::GsmClientSecureSIM7000SSL SimpleNBClientSecure;

#elif defined(SIMPLE_NB_MODEM_SIM7070) || defined(SIMPLE_NB_MODEM_SIM7080) || \
    defined(SIMPLE_NB_MODEM_SIM7090)
#include "SimpleNBClientSIM7080.h"
typedef SimpleNBSim7080                         SimpleNB;
typedef SimpleNBSim7080::GsmClientSim7080       SimpleNBClient;
typedef SimpleNBSim7080::GsmClientSecureSIM7080 SimpleNBClientSecure;

#elif defined(SIMPLE_NB_MODEM_UBLOX)
#include "SimpleNBClientUBLOX.h"
typedef SimpleNBUBLOX                       SimpleNB;
typedef SimpleNBUBLOX::GsmClientUBLOX       SimpleNBClient;
typedef SimpleNBUBLOX::GsmClientSecureUBLOX SimpleNBClientSecure;

#elif defined(SIMPLE_NB_MODEM_SARAR4)
#include "SimpleNBClientSaraR4.h"
typedef SimpleNBSaraR4                    SimpleNB;
typedef SimpleNBSaraR4::GsmClientSaraR4   SimpleNBClient;
typedef SimpleNBSaraR4::GsmClientSecureR4 SimpleNBClientSecure;

#elif defined(SIMPLE_NB_MODEM_XBEE)
#define SIMPLE_NB_SUPPORT_WIFI
#include "SimpleNBClientXBee.h"
typedef SimpleNBXBee                      SimpleNB;
typedef SimpleNBXBee::GsmClientXBee       SimpleNBClient;
typedef SimpleNBXBee::GsmClientSecureXBee SimpleNBClientSecure;

#elif defined(SIMPLE_NB_MODEM_SEQUANS_MONARCH)
#include "SimpleNBClientSequansMonarch.h"
typedef SimpleNBSequansMonarch                          SimpleNB;
typedef SimpleNBSequansMonarch::GsmClientSequansMonarch SimpleNBClient;
typedef SimpleNBSequansMonarch::GsmClientSecureSequansMonarch
    SimpleNBClientSecure;

#else
#error "Please define GSM modem model"
#endif

#endif  // SRC_SIMPLE_NB_CLIENT_H_