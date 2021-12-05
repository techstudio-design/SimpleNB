/**
 * @file       SimpleNBSIM7000.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy.
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_CLIENTSIM7000_H_
#define SRC_SIMPLE_NB_CLIENTSIM7000_H_

// #define SIMPLE_NB_DEBUG Serial
// #define SIMPLE_NB_USE_HEX

#define SIMPLE_NB_MUX_COUNT 8
#define SIMPLE_NB_BUFFER_READ_AND_CHECK_SIZE

#include "SimpleNBClientSIM70xx.h"
#include "SimpleNBTCP.tpp"
#include "SimpleNBGPS.tpp"

class SimpleNBSim7000
  : public SimpleNBSim70xx<SimpleNBSim7000>,
    public SimpleNBTCP<SimpleNBSim7000, SIMPLE_NB_MUX_COUNT>,
    public SimpleNBGPS<SimpleNBSim7000> {
  friend class SimpleNBSim70xx<SimpleNBSim7000>;
  friend class SimpleNBTCP<SimpleNBSim7000, SIMPLE_NB_MUX_COUNT>;
  friend class SimpleNBGPS<SimpleNBSim7000>;

  /*
   * Inner Client
   */
 public:
  class GsmClientSim7000 : public GsmClient {
    friend class SimpleNBSim7000;

   public:
    GsmClientSim7000() {}

    explicit GsmClientSim7000(SimpleNBSim7000& modem, uint8_t mux = 0) {
      init(&modem, mux);
    }

    bool init(SimpleNBSim7000* modem, uint8_t mux = 0) {
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
      //stop();
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
      at->waitResponse(3000);
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
  // NOTE:  Use modem SIMPLENBSIM7000SSL for a secure client!

  /*
   * Constructor
   */
 public:
  explicit SimpleNBSim7000(Stream& stream)
      : SimpleNBSim70xx<SimpleNBSim7000>(stream) {
    memset(sockets, 0, sizeof(sockets));
  }

  /*
   * Basic functions
   */
 protected:
  bool initImpl(const char* pin = NULL) {
    DBG(GF("### SimpleNB Version:"), SIMPLENB_VERSION);
    DBG(GF("### SimpleNB Compiled Module:  SimpleNBClientSIM7000"));

    if (!testAT()) { return false; }

    sendAT(GF("E0"));  // Echo Off
    if (waitResponse() != 1) { return false; }

#ifdef SIMPLE_NB_DEBUG
    sendAT(GF("+CMEE=2"));  // turn on verbose error codes
#else
    sendAT(GF("+CMEE=0"));  // turn off error codes
#endif
    waitResponse();

    getModemName();

    // Enable Local Time Stamp for getting network time
    sendAT(GF("+CLTS=1"));
    if (waitResponse(10000L) != 1) { return false; }

    // Enable battery checks TO-DO: this command caused error
    //sendAT(GF("+CBATCHK=1"));
    //if (waitResponse() != 1) { return false; }

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

  /*
   * Power functions
   */
 protected:
  // Follows the SIM70xx template

  /*
   * Generic network functions
   */
 public:
   bool activateDataNetwork() {
     // Activate application network connection
     // AT+CNACT=<pdpidx>,<action>
     // <pdpidx> PDP Context Identifier. i.e. mux
     // <action> 0: Deactive
     //          1: Active
     //          2: Auto Active
     bool res    = false;
     int  ntries = 0;
     while (!res && ntries < 5) {
       sendAT(GF("+CNACT=1"));
       res = waitResponse(60000L, GF(ACK_NL "+APP PDP: ACTIVE"), GF(ACK_NL "+APP PDP: DEACTIVE"));
       waitResponse();
       ntries++;
     }
     return res;
   }

   bool deactivateDataNetwork() {
     sendAT(GF("+CIPSHUT"));   // Deactivate PDP Connection
     waitResponse(5000L);
     sendAT(GF("+CNACT=0"));
     waitResponse(60000L, GF(ACK_NL "+APP PDP: DEACTIVE"));
     waitResponse();
     return true;
   }

   String getNetworkModes() {
     // Get the help string, not the setting value
     sendAT(GF("+CNMP=?"));
     if (waitResponse(GF(ACK_NL "+CNMP:")) != 1) { return ""; }
     String res = stream.readStringUntil('\n');
     waitResponse();
     return res;
   }

   int16_t getNetworkMode() {
     sendAT(GF("+CNMP?"));
     if (waitResponse(GF(ACK_NL "+CNMP:")) != 1) { return false; }
     int16_t mode = streamGetIntBefore('\n');
     waitResponse();
     return mode;
   }

   bool setNetworkMode(uint8_t mode) {
     // 2 Automatic
     // 13 GSM only
     // 38 LTE only
     // 51 GSM and LTE only
     sendAT(GF("+CNMP="), mode);
     return waitResponse() == 1;
   }

   String getPreferredModes() {
     // Get the help string, not the setting value
     sendAT(GF("+CMNB=?"));
     if (waitResponse(GF(ACK_NL "+CMNB:")) != 1) { return ""; }
     String res = stream.readStringUntil('\n');
     waitResponse();
     return res;
   }

   int16_t getPreferredMode() {
     sendAT(GF("+CMNB?"));
     if (waitResponse(GF(ACK_NL "+CMNB:")) != 1) { return false; }
     int16_t mode = streamGetIntBefore('\n');
     waitResponse();
     return mode;
   }

   bool setPreferredMode(uint8_t mode) {
     // 1 CAT-M
     // 2 NB-IoT
     // 3 CAT-M and NB-IoT
     sendAT(GF("+CMNB="), mode);
     return waitResponse() == 1;
   }

   bool getNetworkSystemMode(bool& n, int16_t& stat) {
     // n: whether to automatically report the system mode info
     // stat: the current service. 0 if it not connected
     sendAT(GF("+CNSMOD?"));
     if (waitResponse(GF(ACK_NL "+CNSMOD:")) != 1) { return false; }
     n    = streamGetIntBefore(',') != 0;
     stat = streamGetIntBefore('\n');
     waitResponse();
     return true;
   }

   bool setNetworkSystemMode(bool n) {
     // n: whether to automatically report the system mode info
     sendAT(GF("+CNSMOD="), int8_t(n));
     return waitResponse() == 1;
   }

  /*
   * GPRS functions
   */
 protected:
  bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) {
    gprsDisconnect();

    // Bearer settings for applications based on IP
    // Set the connection type to GPRS
    sendAT(GF("+SAPBR=3,1,\"Contype\",\"GPRS\""));
    waitResponse();

    // Set the APN
    sendAT(GF("+SAPBR=3,1,\"APN\",\""), apn, '"');
    waitResponse();

    // Set the user name
    if (user && strlen(user) > 0) {
      sendAT(GF("+SAPBR=3,1,\"USER\",\""), user, '"');
      waitResponse();
    }
    // Set the password
    if (pwd && strlen(pwd) > 0) {
      sendAT(GF("+SAPBR=3,1,\"PWD\",\""), pwd, '"');
      waitResponse();
    }

    // Define the PDP context
    sendAT(GF("+CGDCONT=1,\"IP\",\""), apn, '"');
    waitResponse();

    // Attach to GPRS
    sendAT(GF("+CGATT=1"));
    if (waitResponse(60000L) != 1) { return false; }

    // Activate the PDP context
    sendAT(GF("+CGACT=1,1"));
    waitResponse(60000L);

    // Open the definied GPRS bearer context
    sendAT(GF("+SAPBR=1,1"));
    waitResponse(85000L);
    // Query the GPRS bearer context status
    sendAT(GF("+SAPBR=2,1"));
    if (waitResponse(30000L) != 1) { return false; }

    // Set the TCP application toolkit to multi-IP
    sendAT(GF("+CIPMUX=1"));
    if (waitResponse() != 1) { return false; }

    // Put the TCP application toolkit in "quick send" mode
    // (thus no extra "Send OK")
    sendAT(GF("+CIPQSEND=1"));
    if (waitResponse() != 1) { return false; }

    // Set the TCP application toolkit to get data manually
    sendAT(GF("+CIPRXGET=1"));
    if (waitResponse() != 1) { return false; }

    // Start the TCP application toolkit task and set APN, USER NAME, PASSWORD
    sendAT(GF("+CSTT=\""), apn, GF("\",\""), user, GF("\",\""), pwd, GF("\""));
    if (waitResponse(60000L) != 1) { return false; }

    // Bring up the TCP application toolkit wireless connection with GPRS or CSD
    sendAT(GF("+CIICR"));
    if (waitResponse(60000L) != 1) { return false; }

    // Get local IP address for the TCP application toolkit
    // only assigned after connection
    sendAT(GF("+CIFSR;E0"));
    if (waitResponse(10000L) != 1) { return false; }

    return true;
  }

  bool gprsDisconnectImpl() {
    // Shut the TCP application toolkit connection
    // CIPSHUT will close *all* open TCP application toolkit connections
    sendAT(GF("+CIPSHUT"));
    if (waitResponse(60000L) != 1) { return false; }

    sendAT(GF("+CGATT=0"));  // Deactivate the bearer context
    if (waitResponse(60000L) != 1) { return false; }

    return true;
  }

  /*
   * SIM card functions
   */
 protected:
  // Follows the SIM70xx template

  /*
   * Messaging functions
   */
 protected:
  // Follows all messaging functions per template

  /*
   * GPS/GNSS/GLONASS location functions
   */
protected:
 // enable GPS
 bool enableGPSImpl() {
   sendAT(GF("+CGNSPWR=1"));
   if (waitResponse() != 1) { return false; }
   return true;
 }

 bool disableGPSImpl() {
   sendAT(GF("+CGNSPWR=0"));
   if (waitResponse() != 1) { return false; }
   return true;
 }

 // get the RAW GPS output
 String getGPSImpl() {
   sendAT(GF("+CGNSINF"));
   if (waitResponse(120000L, GF(ACK_NL "+CGNSINF:")) != 1) {
     return "";
   }
   String res = stream.readStringUntil('\n');
   waitResponse();
   res.trim();
   return res;
 }

 // get GPS informations
 bool getGPSImpl(GPS_t gps) {
   sendAT(GF("+CGNSINF"));
   if (waitResponse(120000L, GF(ACK_NL "+CGNSINF:")) != 1) {
     return false;
   }

   streamSkipUntil(',');                // GNSS run status
   if (streamGetIntBefore(',') == 1) {  // fix status

     // UTC date & Time
     gps.year   = streamGetIntLength(4);  // Four digit year
     if (gps.year < 2000) gps.year += 2000;
     gps.month  = streamGetIntLength(2);  // Two digit month
     gps.day    = streamGetIntLength(2);  // Two digit day
     gps.hour   = streamGetIntLength(2);  // Two digit hour
     gps.minute = streamGetIntLength(2);  // Two digit minute
     gps.second = static_cast<int>(streamGetFloatBefore(','));  // second only

     gps.lat   = streamGetFloatBefore(',');  // Latitude
     gps.lon   = streamGetFloatBefore(',');  // Longitude
     gps.alt   = streamGetFloatBefore(',');  // MSL Altitude. Unit is meters
     gps.speed = streamGetFloatBefore(','); // Speed in knots.
     streamSkipUntil(',');  // Course Over Ground. Degrees.
     streamSkipUntil(',');  // Fix Mode
     streamSkipUntil(',');  // Reserved1
     gps.accuracy = streamGetFloatBefore(','); // Horizontal Dilution Of Precision
     streamSkipUntil(',');  // Position Dilution Of Precision
     streamSkipUntil(',');  // Vertical Dilution Of Precision
     streamSkipUntil(',');  // Reserved2
     gps.vsat = streamGetIntBefore(',');  // GNSS Satellites in View
     gps.usat = streamGetIntBefore(',');  // GNSS Satellites Used
     streamSkipUntil(',');             // GLONASS Satellites Used
     streamSkipUntil(',');             // Reserved3
     streamSkipUntil(',');             // C/N0 max
     streamSkipUntil(',');             // HPA
     streamSkipUntil('\n');            // VPA

     waitResponse();
     return true;
   }

   streamSkipUntil('\n');  // toss the row of commas
   waitResponse();
   return false;
 }

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
 protected:
  bool modemConnect(const char* host, uint16_t port, uint8_t mux,
                    bool ssl = false, int timeout_s = 75) {
    if (ssl) { DBG("SSL only supported using application on SIM7000!"); }
    uint32_t timeout_ms = ((uint32_t)timeout_s) * 1000;

    // Set multiple connection mode
    sendAT(GF("+CIPMUX=1"));
    if (waitResponse() != 1) { return false; }

    // Set "quick send" mode, CIPSEND will response with "DATA ACCEPT: <mux>, <len>"
    // instead of ""<mux>, Send OK"
    sendAT(GF("+CIPQSEND=1"));
    if (waitResponse() != 1) { return false; }

    // Set the TCP application toolkit to get data manually
    sendAT(GF("+CIPRXGET=1"));
    if (waitResponse() != 1) { return false; }

    // Start the TCP application toolkit task and set APN, USER NAME, PASSWORD
    sendAT(GF("+CSTT=\"stmiot\""));    //TO-DO: temperary hardcoded APN
    if (waitResponse() != 1) { return false; }

    // Bring up wireless connection with GPRS
    sendAT(GF("+CIICR"));
    if (waitResponse() != 1) { return false; }

    // Get local IP address (this can't be skipt as it moves from one state to another)
    sendAT(GF("+CIFSR;E0"));
    if (waitResponse(10000L) != 1) { return false; }

    sendAT(GF("+CIPSTART="), mux, GF(",\"TCP\",\""), host, GF("\",\""), port, GF("\""));
    return (1 == waitResponse(timeout_ms, GF("CONNECT OK" ACK_NL),
              GF("CONNECT FAIL" ACK_NL),
              GF("ALREADY CONNECT" ACK_NL),
              GF("ERROR" ACK_NL),
              GF("CLOSE OK" ACK_NL))
            );
  }

  int16_t modemSend(const void* buff, size_t len, uint8_t mux) {
    sendAT(GF("+CIPSEND="), mux, ',', (uint16_t)len);
    if (waitResponse(GF(">")) != 1) { return 0; }

    stream.write(reinterpret_cast<const uint8_t*>(buff), len);
    stream.flush();

    if (waitResponse(GF(ACK_NL "DATA ACCEPT:")) != 1) { return 0; }
    streamSkipUntil(',');  // Skip mux
    return streamGetIntBefore('\n');
  }

  size_t modemRead(size_t size, uint8_t mux) {
    if (!sockets[mux]) return 0;

#ifdef SIMPLE_NB_USE_HEX
    sendAT(GF("+CIPRXGET=3,"), mux, ',', (uint16_t)size);
    if (waitResponse(GF("+CIPRXGET:")) != 1) { return 0; }
#else
    sendAT(GF("+CIPRXGET=2,"), mux, ',', (uint16_t)size);
    if (waitResponse(GF("+CIPRXGET:")) != 1) { return 0; }
#endif
    streamSkipUntil(',');  // Skip Rx mode 2/normal or 3/HEX
    streamSkipUntil(',');  // Skip mux
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
      while (stream.available() < 2 &&
             (millis() - startMillis < sockets[mux]->_timeout)) {
        SIMPLE_NB_YIELD();
      }
      char buf[4] = {
          0,
      };
      buf[0] = stream.read();
      buf[1] = stream.read();
      char c = strtol(buf, NULL, 16);
#else
      while (!stream.available() &&
             (millis() - startMillis < sockets[mux]->_timeout)) {
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

    sendAT(GF("+CIPRXGET=4,"), mux);
    size_t result = 0;
    if (waitResponse(GF("+CIPRXGET:")) == 1) {
      streamSkipUntil(',');  // Skip mode 4
      streamSkipUntil(',');  // Skip mux
      result = streamGetIntBefore('\n');
      waitResponse();
    }
    // DBG("### Available:", result, "on", mux);
    if (!result) { sockets[mux]->sock_connected = modemGetConnected(mux); }
    return result;
  }

  bool modemGetConnected(uint8_t mux) {
    sendAT(GF("+CIPSTATUS="), mux);
    waitResponse(GF("+CIPSTATUS"));
    int8_t res = waitResponse(GF(",\"CONNECTED\""), GF(",\"CLOSED\""),
                              GF(",\"CLOSING\""), GF(",\"REMOTE CLOSING\""),
                              GF(",\"INITIAL\""));
    waitResponse();
    return 1 == res;
  }

  /*
   * Utilities
   */
 public:
  // TODO(vshymanskyy): Optimize this!
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
            DBG("### Got Data on socket:", mux);
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
          DBG("### READ: " + String(len) + " from " + String(mux));
        } else if (data.endsWith(GF("CLOSED" ACK_NL))) {
          int8_t nl   = data.lastIndexOf(ACK_NL, data.length() - 8);
          int8_t coma = data.indexOf(',', nl + 2);
          int8_t mux  = data.substring(nl + 2, coma).toInt();
          if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
            sockets[mux]->sock_connected = false;
          }
          data = "";
          DBG("### Closed socket: ", mux);
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
    return waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
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

 protected:
  GsmClientSim7000* sockets[SIMPLE_NB_MUX_COUNT];
};

#endif  // SRC_SIMPLE_NB_CLIENTSIM7000_H_
