/**
 * @file       SimpleNBSIM7020.tpp
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Sep 2021
 */

#ifndef SRC_SIMPLE_NB_CLIENTSIM7020_H_
#define SRC_SIMPLE_NB_CLIENTSIM7020_H_

#define SIMPLE_NB_MUX_COUNT 1
#define SIMPLE_NB_BUFFER_READ_AND_CHECK_SIZE

#include "SimpleNBClientSIM70xx.h"
#include "SimpleNBTCP.tpp"


class SimpleNBSim7020
  : public SimpleNBSim70xx<SimpleNBSim7020>,
    public SimpleNBTCP<SimpleNBSim7020, SIMPLE_NB_MUX_COUNT> {
  friend class SimpleNBSim70xx<SimpleNBSim7020>;
  friend class SimpleNBTCP<SimpleNBSim7020, SIMPLE_NB_MUX_COUNT>;

  /*
   * Inner Client
   */
 public:
  class GsmClientSim7020 : public GsmClient {
    friend class SimpleNBSim7020;

   public:
    GsmClientSim7020() {}

    explicit GsmClientSim7020(SimpleNBSim7020& modem, uint8_t mux = 0) {
      init(&modem, mux);
    }

    bool init(SimpleNBSim7020* modem, uint8_t mux = 0) {
      this->at       = modem;
      sock_available = 0;
      prev_check     = 0;
      sock_connected = false;
      got_data       = false;

      if (mux < SIMPLE_NB_MUX_COUNT) {
        this->mux = mux;
      } else {
        this->mux = (mux % SIMPLE_NB_MUX_COUNT);
      }
      at->sockets[this->mux] = this;

      return true;
    }

   public:
    virtual int connect(const char* host, uint16_t port, int timeout_s) {
      stop();
      SIMPLE_NB_YIELD();
      rx.clear();
      sock_connected = at->modemConnect(host, port, mux, false, timeout_s);
      return sock_connected;
    }
    SIMPLE_NB_CLIENT_CONNECT_OVERRIDES

    void stop(uint32_t maxWaitMs) {
      dumpModemBuffer(maxWaitMs);
      at->sendAT(GF("+CIPCLOSE="), mux);
      sock_connected = false;
      at->waitResponse(GF("CLOSE OK"));
    }
    void stop() override {
      stop(15000L);
    }

    /*
     * Extended API
     */

    String remoteIP() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
  };

  /*
   * Inner Secure Client
   */
  // NOTE:  Use modem SimpleNBSim7020SSL for a secure client!

  /*
   * Constructor
   */
 public:
  explicit SimpleNBSim7020(Stream& stream)
      : SimpleNBSim70xx<SimpleNBSim7020>(stream) {
    memset(sockets, 0, sizeof(sockets));
  }

  /*
   * Basic functions
   */
 protected:
  bool initImpl(const char* pin = NULL) {
    DBG(GF("### SimpleNB Version:"), SIMPLENB_VERSION);
    DBG(GF("### SimpleNB Compiled Module:  SimpleNBClientSim7020"));

    if (!testAT()) { return false; }

    sendAT(GF("E0"));  // Echo Off
    if (waitResponse() != 1) { return false; }

#ifdef SIMPLE_NB_DEBUG
    sendAT(GF("+CMEE=2"));  // turn on verbose error codes
#else
    sendAT(GF("+CMEE=0"));  // turn off error codes
#endif
    waitResponse();

    DBG(GF("### Modem:"), getModemName());

    // Enable Local Time Stamp for getting network time
    sendAT(GF("+CLTS=1"));
    if (waitResponse(10000L) != 1) { return false; }

    // Enable battery checks
    sendAT(GF("+CBATCHK=1"));
    if (waitResponse() != 1) { return false; }

    SimStatus ret = getSimStatus();
    // if the sim isn't ready and a pin has been provided, try to unlock the sim
    if (ret != SIM_READY && pin != NULL && strlen(pin) > 0) {
      simUnlock(pin);
      return (getSimStatus() == SIM_READY);
    } else {
      // if the sim is ready, or it's locked but no pin has been provided,
      // return true
      return (ret == SIM_READY || ret == SIM_LOCKED);
    }
  }

  bool factoryDefaultImpl()
  {
      sendAT(GF("&F0"));     // Factory
      waitResponse();
      sendAT(GF("Z0"));     // Reset
      waitResponse();
      sendAT(GF("E0"));     // Echo Off
      waitResponse();
      sendAT(GF("+CSOSENDFLAG=0"));     // Disable TCP Send Flag
      waitResponse();
      sendAT(GF("+IPR=0"));     // Auto-baud
      waitResponse();
      sendAT(GF("+IFC=0,0"));     // No Flow Control
      waitResponse();
      sendAT(GF("+ICF=3,3"));     // 8 data 0 parity 1 stop
      waitResponse();
      sendAT(GF("+CSCLK=0"));     // Disable Slow Clock
      waitResponse();
      sendAT(GF("&W"));     // Write configuration
      return waitResponse() == 1;
  }

  /*
   * Power functions
   */

  // Follows the SIM70xx template

  /*
   * Generic network functions
   */
public:
  // the following functions implemented in Sim70xx need double-check against SIM7020 documentation
  // String getNetworkModes() {
  //   // Get the help string, not the setting value
  //   thisModem().sendAT(GF("+CNMP=?"));
  //   if (thisModem().waitResponse(GF(ACK_NL "+CNMP:")) != 1) { return ""; }
  //   String res = stream.readStringUntil('\n');
  //   thisModem().waitResponse();
  //   return res;
  // }
  //
  // int16_t getNetworkMode() {
  //   thisModem().sendAT(GF("+CNMP?"));
  //   if (thisModem().waitResponse(GF(ACK_NL "+CNMP:")) != 1) { return false; }
  //   int16_t mode = thisModem().streamGetIntBefore('\n');
  //   thisModem().waitResponse();
  //   return mode;
  // }
  //
  // bool setNetworkMode(uint8_t mode) {
  //   // 2 Automatic
  //   // 13 GSM only
  //   // 38 LTE only
  //   // 51 GSM and LTE only
  //   thisModem().sendAT(GF("+CNMP="), mode);
  //   return thisModem().waitResponse() == 1;
  // }
  //
  // String getPreferredModes() {
  //   // Get the help string, not the setting value
  //   thisModem().sendAT(GF("+CMNB=?"));
  //   if (thisModem().waitResponse(GF(ACK_NL "+CMNB:")) != 1) { return ""; }
  //   String res = stream.readStringUntil('\n');
  //   thisModem().waitResponse();
  //   return res;
  // }
  //
  // int16_t getPreferredMode() {
  //   thisModem().sendAT(GF("+CMNB?"));
  //   if (thisModem().waitResponse(GF(ACK_NL "+CMNB:")) != 1) { return false; }
  //   int16_t mode = thisModem().streamGetIntBefore('\n');
  //   thisModem().waitResponse();
  //   return mode;
  // }
  //
  // bool setPreferredMode(uint8_t mode) {
  //   // 1 CAT-M
  //   // 2 NB-IoT
  //   // 3 CAT-M and NB-IoT
  //   thisModem().sendAT(GF("+CMNB="), mode);
  //   return thisModem().waitResponse() == 1;
  // }
  //
  // bool getNetworkSystemMode(bool& n, int16_t& stat) {
  //   // n: whether to automatically report the system mode info
  //   // stat: the current service. 0 if it not connected
  //   thisModem().sendAT(GF("+CNSMOD?"));
  //   if (thisModem().waitResponse(GF(ACK_NL "+CNSMOD:")) != 1) { return false; }
  //   n    = thisModem().streamGetIntBefore(',') != 0;
  //   stat = thisModem().streamGetIntBefore('\n');
  //   thisModem().waitResponse();
  //   return true;
  // }
  //
  // bool setNetworkSystemMode(bool n) {
  //   // n: whether to automatically report the system mode info
  //   thisModem().sendAT(GF("+CNSMOD="), int8_t(n));
  //   return thisModem().waitResponse() == 1;
  // }
  // String getLocalIPImpl() {
  //   sendAT(GF("+CIFSR;E0"));
  //   String res;
  //   if (waitResponse(10000L, res) != 1) { return ""; }
  //   res.replace(ACK_NL "OK" ACK_NL, "");
  //   res.replace(ACK_NL, "");
  //   res.trim();
  //   return res;
  // }

  /*
   * GPRS functions
   */
 protected:
  bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) SIMPLE_NB_ATTR_NOT_IMPLEMENTED;

  bool gprsDisconnectImpl() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;

  /*
   * SIM card functions
   */
 protected:
  // Follows the SIM70xx template

  /*
   * Messaging functions
   */
 public:
  String sendUSSD(const String& code) SIMPLE_NB_ATTR_NOT_AVAILABLE;

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
  byte NTPServerSyncImpl(String server = "pool.ntp.org", byte TimeZone = 32)
  {
      // AT+CURTC Control CCLK Show UTC Or RTC Time
      // Use AT CCLK? command to get UTC Or RTC Time
      // Start to query network time
      sendAT(GF("+CSNTPSTART="), '\"', server, GF("\",\"+"), String(TimeZone), '\"');
      if (waitResponse(10000L) != 1) {
          return -1;
      }

      // Stop to query network time
      sendAT(GF("+CSNTPSTOP"));
      if (waitResponse(10000L) != 1) {
          return -1;
      }
      return 1;
  }

  /*
   * Battery functions
   */
 protected:
  bool getBatteryStatusImpl(Battery_t& batt) {
    sendAT(GF("+CBC"));
    if (waitResponse(GF("+CBC:")) != 1) { return false; }
    batt.chargeState = 0;
    batt.percent     = streamGetIntBefore(',');
    batt.milliVolts  = streamGetIntBefore('\n');
    waitResponse();
    return true;
  }
  /*
   * Client related functions
   */
 protected:
  bool modemConnect(const char* host, uint16_t port, uint8_t mux,
                    bool ssl = false, int timeout_s = 75) {
    if (!sockets[mux]) return 0;
    if (ssl) { DBG("SSL only supported using application on Sim7020!"); }
    uint32_t timeout_ms = ((uint32_t)timeout_s) * 1000;

    /* Select Data Transmitting Mode */
    sendAT(GF("+CIPQSEND=1"));
    if (waitResponse() != 1) { return false; }
    /* Set to get data manually */
    sendAT(GF("+CIPRXGET=1"));
    waitResponse();

    // when not using SSL, the TCP application toolkit is more stable
    sendAT(GF("+CIPSTART="), GF("\"TCP"), GF("\",\""), host, GF("\","), port);
    int8_t rsp = 0;
    rsp = waitResponse(timeout_ms, GF("CONNECT OK" ACK_NL),
                         GF("CONNECT FAIL" ACK_NL),
                         GF("ALREADY CONNECT" ACK_NL), GF("ERROR" ACK_NL),
                         GF("CLOSE OK" ACK_NL));
    return (rsp == 1);
  }

  int16_t modemSend(const void* buff, size_t len, uint8_t mux) {
    if (!sockets[mux]) return 0;
    sendAT(GF("+CIPSEND="), (uint16_t)len);
    if (waitResponse(GF(">")) != 1) { return 0; }

    stream.write(reinterpret_cast<const uint8_t*>(buff), len);
    stream.flush();

    if (waitResponse(GF(ACK_NL "DATA ACCEPT:")) != 1) { return 0; }
    return streamGetIntBefore('\n');
  }

  size_t modemRead(size_t size, uint8_t mux) {
    if (!sockets[mux]) return 0;

#ifdef SIMPLE_NB_USE_HEX
    sendAT(GF("+CIPRXGET=3,"), (uint16_t)size);
    if (waitResponse(GF("+CIPRXGET:")) != 1) { return 0; }
#else
    sendAT(GF("+CIPRXGET=2,"), (uint16_t)size);
    if (waitResponse(GF("+CIPRXGET:")) != 1) { return 0; }
#endif
    streamSkipUntil(',');  // Skip Rx mode 2/normal or 3/HEX
    int16_t len_requested = streamGetIntBefore(',');
    //  ^^ Requested number of data bytes (1-1460 bytes)to be read
    int16_t len_confirmed = streamGetIntBefore('\n');
    // ^^ Confirmed number of data bytes to be read, which may be less than
    // requested. 0 indicates that no data can be read.
    // SRGD NOTE:  Contrary to above (which is copied from AT command manual)
    // this is actually be the number of bytes that will be remaining in the
    // buffer after the read.
    for (int i = 0; i < len_requested; i++) {
      uint32_t startMillis = millis();
#ifdef SIMPLE_NB_USE_HEX
      while (stream.available() < 2 && (millis() - startMillis < sockets[mux]->_timeout)) {
        SIMPLE_NB_YIELD();
      }
      char buf[4] = {0,};
      buf[0] = stream.read();
      buf[1] = stream.read();
      char c = strtol(buf, NULL, 16);
#else
      while (!stream.available() && (millis() - startMillis < sockets[mux]->_timeout)) {
        SIMPLE_NB_YIELD();
      }
      char c = stream.read();
#endif
      sockets[mux]->rx.put(c);
    }
    // DBG("### READ:", len_requested, "from", mux);
    // sockets[mux]->sock_available = modemGetAvailable(mux);
    sockets[mux]->sock_available = len_confirmed;
    waitResponse();
    return len_requested;
  }

  size_t modemGetAvailable(uint8_t mux) {
    if (!sockets[mux]) return 0;

    sendAT(GF("+CIPRXGET=4"), mux);
    size_t result = 0;
    if (waitResponse(GF("+CIPRXGET:")) == 1) {
      streamSkipUntil(',');  // Skip mode 4
      result = streamGetIntBefore('\n');
      waitResponse();
    }
    // DBG("### Available:", result, "on", mux);
    if (!result) { sockets[mux]->sock_connected = modemGetConnected(mux); }
    return result;
  }

  bool modemGetConnected(uint8_t mux) {
    if (!sockets[mux]) { return 0;}
    /* Get Socket Status */
    sendAT(GF("+CIPSTATUS"));
    if (waitResponse(GF("STATE: "))) {
        if (waitResponse(GF("CONNECT OK"), GF("TCP CLOSED"), GF("TCP CONNECTING"), GF("IP INITIAL")) != 1) {
            return false;
        }
    }
    return true;
  }

  /*
   * Utilities
   */
 public:
  // TODO(vshymanskyy): Optimize this!
  int8_t waitResponse(uint32_t timeout_ms, String& data, GsmConstStr r1 = GFP(ACK_OK), GsmConstStr r2 = GFP(ACK_ERROR),
#if defined SIMPLE_NB_DEBUG
                      GsmConstStr r3 = GFP(ACK_CME_ERROR), GsmConstStr r4 = GFP(ACK_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    /*String r1s(r1); r1s.trim();
    String r2s(r2); r2s.trim();
    String r3s(r3); r3s.trim();
    String r4s(r4); r4s.trim();
    String r5s(r5); r5s.trim();
    DBG("### ..:", r1s, ",", r2s, ",", r3s, ",", r4s, ",", r5s);*/
    data.reserve(64);
    uint8_t  index       = 0;
    uint32_t startMillis = millis();
    do {
      SIMPLE_NB_YIELD();
      while (stream.available() > 0) {
        SIMPLE_NB_YIELD();
        int8_t a = stream.read();
        if (a <= 0) continue;  // Skip 0x00 bytes, just in case
        data += static_cast<char>(a);
        if (r1 && data.endsWith(r1)) {
          index = 1;
          goto finish;
        } else if (r2 && data.endsWith(r2)) {
          index = 2;
          goto finish;
        } else if (r3 && data.endsWith(r3)) {
#if defined SIMPLE_NB_DEBUG
          if (r3 == GFP(ACK_CME_ERROR)) {
            streamSkipUntil('\n');  // Read out the error
          }
#endif
          index = 3;
          goto finish;
        } else if (r4 && data.endsWith(r4)) {
          index = 4;
          goto finish;
        } else if (r5 && data.endsWith(r5)) {
          index = 5;
          goto finish;
        } else if (data.endsWith(GF(ACK_NL "+CIPRXGET:"))) {
          int8_t mode = streamGetIntBefore(',');
          if (mode == 1) {
            int8_t mux = streamGetIntBefore('\n');
            if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
              sockets[mux]->got_data = true;
            }
            data = "";
            // DBG("### Got Data:", mux);
          } else {
            data += mode;
          }
        } else if (data.endsWith(GF(ACK_NL "+RECEIVE:"))) {
          int8_t  mux = streamGetIntBefore(',');
          int16_t len = streamGetIntBefore('\n');
          if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
            sockets[mux]->got_data = true;
            if (len >= 0 && len <= 1024) { sockets[mux]->sock_available = len; }
          }
          data = "";
          // DBG("### Got Data:", len, "on", mux);
        } else if (data.endsWith(GF("CLOSED" ACK_NL))) {
          // int8_t nl   = data.lastIndexOf(ACK_NL, data.length() - 8);
          // int8_t coma = data.indexOf(',', nl + 2);
          // int8_t mux  = data.substring(nl + 2, coma).toInt();
          int8_t mux = 0;
          if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
            sockets[mux]->sock_connected = false;
          }
          data = "";
          // DBG("### Closed: ", mux);
        } else if (data.endsWith(GF("*PSNWID:"))) {
          streamSkipUntil('\n');  // Refresh network name by network
          data = "";
          DBG("### Network name updated.");
        } else if (data.endsWith(GF("*PSUTTZ:"))) {
          streamSkipUntil('\n');  // Refresh time and time zone by network
          data = "";
          DBG("### Network time and time zone updated.");
        } else if (data.endsWith(GF("+CTZV:"))) {
          streamSkipUntil('\n');  // Refresh network time zone by network
          data = "";
          DBG("### Network time zone updated.");
        } else if (data.endsWith(GF("DST: "))) {
          streamSkipUntil(
              '\n');  // Refresh Network Daylight Saving Time by network
          data = "";
          DBG("### Daylight savings time state updated.");
        } else if (data.endsWith(GF(ACK_NL "SMS Ready" ACK_NL))) {
          data = "";
          DBG("### Unexpected module reset!");
          init();
        }
      }
    } while (millis() - startMillis < timeout_ms);
  finish:
    if (!index) {
      data.trim();
      if (data.length()) { DBG("### Unhandled:", data); }
      data = "";
    }
    // data.replace(ACK_NL, "/");
    // DBG('<', index, '>', data);
    return index;
  }

  int8_t waitResponse(uint32_t timeout_ms, GsmConstStr r1 = GFP(ACK_OK), GsmConstStr r2 = GFP(ACK_ERROR),
#if defined SIMPLE_NB_DEBUG
    GsmConstStr r3 = GFP(ACK_CME_ERROR), GsmConstStr r4 = GFP(ACK_CMS_ERROR),
#else
    GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
    GsmConstStr r5 = NULL) {
    String data;
    return waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
  }

  int8_t waitResponse(GsmConstStr r1 = GFP(ACK_OK), GsmConstStr r2 = GFP(ACK_ERROR),
#if defined SIMPLE_NB_DEBUG
    GsmConstStr r3 = GFP(ACK_CME_ERROR), GsmConstStr r4 = GFP(ACK_CMS_ERROR),
#else
    GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
    GsmConstStr r5 = NULL) {
      return waitResponse(1000, r1, r2, r3, r4, r5);
    }

 protected:
  GsmClientSim7020* sockets[SIMPLE_NB_MUX_COUNT];
};

#endif  // SRC_SIMPLE_NB_CLIENTSim7020_H_
