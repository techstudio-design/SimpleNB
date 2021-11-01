/**
 * @file       SimpleNBSIM70xx.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy.
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_CLIENTSIM70XX_H_
#define SRC_SIMPLE_NB_CLIENTSIM70XX_H_

// #define SIMPLE_NB_DEBUG Serial
// #define SIMPLE_NB_USE_HEX

#include "SimpleNBBattery.tpp"
#include "SimpleNBModem.tpp"
#include "SimpleNBSMS.tpp"
#include "SimpleNBTime.tpp"
#include "SimpleNBNTP.tpp"

#define ACK_NL "\r\n"
static const char ACK_OK[] SIMPLE_NB_PROGMEM    = "OK" ACK_NL;
static const char ACK_ERROR[] SIMPLE_NB_PROGMEM = "ERROR" ACK_NL;
#if defined       SIMPLE_NB_DEBUG
static const char ACK_CME_ERROR[] SIMPLE_NB_PROGMEM = ACK_NL "+CME ERROR:";
static const char ACK_CMS_ERROR[] SIMPLE_NB_PROGMEM = ACK_NL "+CMS ERROR:";
#endif

enum RegStatus {
  REG_NO_RESULT    = -1,
  REG_UNREGISTERED = 0,
  REG_SEARCHING    = 2,
  REG_DENIED       = 3,
  REG_OK_HOME      = 1,
  REG_OK_ROAMING   = 5,
  REG_UNKNOWN      = 4,
};

template <class modemType>
class SimpleNBSim70xx:public SimpleNBModem<SimpleNBSim70xx<modemType>>,
                      public SimpleNBSMS<SimpleNBSim70xx<modemType>>,
                      public SimpleNBTime<SimpleNBSim70xx<modemType>>,
                      public SimpleNBNTP<SimpleNBSim70xx<modemType>>,
                      public SimpleNBBattery<SimpleNBSim70xx<modemType>>
{
  friend class SimpleNBModem<SimpleNBSim70xx<modemType>>;
  friend class SimpleNBSMS<SimpleNBSim70xx<modemType>>;
  friend class SimpleNBTime<SimpleNBSim70xx<modemType>>;
  friend class SimpleNBNTP<SimpleNBSim70xx<modemType>>;
  friend class SimpleNBBattery<SimpleNBSim70xx<modemType>>;

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
   * Constructor
   */
 public:
  explicit SimpleNBSim70xx(Stream& stream) : stream(stream) {}

  /*
   * Basic functions
   */
 protected:
  bool initImpl(const char* pin = NULL) {
    return thisModem().initImpl(pin);
  }

  String getModemNameImpl() {
    String name = "SIMCom SIM7000";

    thisModem().sendAT(GF("+GMM"));
    String res2;
    if (thisModem().waitResponse(5000L, res2) != 1) { return name; }
    res2.replace(ACK_NL "OK" ACK_NL, "");
    res2.replace("_", " ");
    res2.trim();

    name = res2;
    return name;
  }

  bool factoryDefaultImpl() {           // these commands aren't supported
    thisModem().sendAT(GF("&FZE0&W"));  // Factory + Reset + Echo Off + Write
    thisModem().waitResponse();
    thisModem().sendAT(GF("+IPR=0"));  // Auto-baud
    thisModem().waitResponse();
    thisModem().sendAT(GF("+IFC=0,0"));  // No Flow Control
    thisModem().waitResponse();
    thisModem().sendAT(GF("+ICF=3,3"));  // 8 data 0 parity 1 stop
    thisModem().waitResponse();
    thisModem().sendAT(GF("+CSCLK=0"));  // Disable Slow Clock
    thisModem().waitResponse();
    thisModem().sendAT(GF("&W"));  // Write configuration
    return thisModem().waitResponse() == 1;
  }

  /*
   * Power functions
   */
protected:
  bool restartImpl(const char* pin=NULL) {
    thisModem().sendAT(GF("E0"));  // Echo Off
    thisModem().waitResponse();
    if (!thisModem().setPhoneFunctionality(0)) { return false; }
    if (!thisModem().setPhoneFunctionality(1, true)) { return false; }
    thisModem().waitResponse(30000L, GF("SMS Ready"));
    return initImpl(pin);
  }

  bool powerOffImpl() {
    thisModem().sendAT(GF("+CPOWD=1"));
    return thisModem().waitResponse(GF("NORMAL POWER DOWN")) == 1;
  }

  // During sleep, the SIM70xx module has its serial communication disabled.
  // In order to reestablish communication pull the DRT-pin of the SIM70xx
  // module LOW for at least 50ms. Then use this function to disable sleep
  // mode. The DTR-pin can then be released again.
  bool sleepEnableImpl(bool enable = true) {
    thisModem().sendAT(GF("+CSCLK="), enable);
    return thisModem().waitResponse() == 1;
  }

  bool setPhoneFunctionalityImpl(uint8_t fun, bool reset = false) {
    thisModem().sendAT(GF("+CFUN="), fun, reset ? ",1" : "");
    return thisModem().waitResponse(10000L) == 1;
  }

  /*
   * Generic network functions
   */
 public:
  RegStatus getRegistrationStatus() {
    RegStatus epsStatus =
        (RegStatus)thisModem().getRegistrationStatusXREG("CEREG");
    // If we're connected on EPS, great!
    if (epsStatus == REG_OK_HOME || epsStatus == REG_OK_ROAMING) {
      return epsStatus;
    } else {
      // Otherwise, check GPRS network status
      // We could be using GPRS fall-back or the board could be being moody
      return (RegStatus)thisModem().getRegistrationStatusXREG("CGREG");
    }
  }

 protected:
  bool isNetworkRegisteredImpl() {
    RegStatus s = getRegistrationStatus();
    return (s == REG_OK_HOME || s == REG_OK_ROAMING);
  }

  /*
   * GPRS functions
   */
 protected:
  // should implement in sub-classes
  bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) {
    return gprsConnectImpl(apn, user, pwd);
  }

  bool gprsDisconnectImpl() {
    return gprsDisconnectImpl();
  }

  /*
   * SIM card functions
   */
 protected:
  // Doesn't return the "+CCID" before the number
  String getSimCCIDImpl() {
    thisModem().sendAT(GF("+CCID"));
    if (thisModem().waitResponse(GF(ACK_NL)) != 1) { return ""; }
    String res = stream.readStringUntil('\n');
    thisModem().waitResponse();
    res.trim();
    return res;
  }

  /*
   * Messaging functions
   */
 protected:
  // Follows all messaging functions per template

  /*
   * GPS/GNSS/GLONASS location functions
   */

  /*
   * Time functions
   */
  // Can follow CCLK as per template

  /*
   * NTP server functions
   */
  // Can sync with server using CNTP as per template

  /*
   * Battery functions
   */
 protected:
  // Follows all battery functions per template

  /*
   * Client related functions
   */
  // should implement in sub-classes

  /*
   * Utilities
   */
 public:
  // should implement in sub-classes
  int8_t waitResponse(uint32_t timeout_ms, String& data,
                      GsmConstStr r1 = GFP(ACK_OK),
                      GsmConstStr r2 = GFP(ACK_ERROR),
#if defined SIMPLE_NB_DEBUG
                      GsmConstStr r3 = GFP(ACK_CME_ERROR),
                      GsmConstStr r4 = GFP(ACK_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    return thisModem().waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
  }

  int8_t waitResponse(uint32_t timeout_ms, GsmConstStr r1 = GFP(ACK_OK),
                      GsmConstStr r2 = GFP(ACK_ERROR),
#if defined SIMPLE_NB_DEBUG
                      GsmConstStr r3 = GFP(ACK_CME_ERROR),
                      GsmConstStr r4 = GFP(ACK_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    String data;
    return thisModem().waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
  }

  int8_t waitResponse(GsmConstStr r1 = GFP(ACK_OK),
                      GsmConstStr r2 = GFP(ACK_ERROR),
#if defined SIMPLE_NB_DEBUG
                      GsmConstStr r3 = GFP(ACK_CME_ERROR),
                      GsmConstStr r4 = GFP(ACK_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    return waitResponse(1000, r1, r2, r3, r4, r5);
  }

 public:
  Stream& stream;

 protected:
  const char* gsmNL = ACK_NL;
};

#endif  // SRC_SIMPLE_NB_CLIENTSIM70XX_H_
