/**
 * @file       SimpleNBClientBG96.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy.
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_CLIENTBG96_H_
#define SRC_SIMPLE_NB_CLIENTBG96_H_
// #pragma message("SimpleNB:  SimpleNBClientBG96")

// #define SIMPLE_NB_DEBUG Serial

#define SIMPLE_NB_MUX_COUNT 12
#define SIMPLE_NB_BUFFER_READ_AND_CHECK_SIZE

#include "SimpleNBBattery.tpp"
#include "SimpleNBCalling.tpp"
#include "SimpleNBGPS.tpp"
#include "SimpleNBModem.tpp"
#include "SimpleNBSMS.tpp"
#include "SimpleNBTCP.tpp"
#include "SimpleNBTemperature.tpp"
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

class SimpleNBBG96 : public SimpleNBModem<SimpleNBBG96>,
                    public SimpleNBTCP<SimpleNBBG96, SIMPLE_NB_MUX_COUNT>,
                    public SimpleNBCalling<SimpleNBBG96>,
                    public SimpleNBSMS<SimpleNBBG96>,
                    public SimpleNBTime<SimpleNBBG96>,
                    public SimpleNBNTP<SimpleNBBG96>,
                    public SimpleNBGPS<SimpleNBBG96>,
                    public SimpleNBBattery<SimpleNBBG96>,
                    public SimpleNBTemperature<SimpleNBBG96> {
  friend class SimpleNBModem<SimpleNBBG96>;
  friend class SimpleNBTCP<SimpleNBBG96, SIMPLE_NB_MUX_COUNT>;
  friend class SimpleNBCalling<SimpleNBBG96>;
  friend class SimpleNBSMS<SimpleNBBG96>;
  friend class SimpleNBTime<SimpleNBBG96>;
  friend class SimpleNBNTP<SimpleNBBG96>;
  friend class SimpleNBGPS<SimpleNBBG96>;
  friend class SimpleNBBattery<SimpleNBBG96>;
  friend class SimpleNBTemperature<SimpleNBBG96>;

  /*
   * Inner Client
   */
 public:
  class GsmClientBG96 : public GsmClient {
    friend class SimpleNBBG96;

   public:
    GsmClientBG96() {}

    explicit GsmClientBG96(SimpleNBBG96& modem, uint8_t mux = 0) {
      init(&modem, mux);
    }

    bool init(SimpleNBBG96* modem, uint8_t mux = 0) {
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
      uint32_t startMillis = millis();
      dumpModemBuffer(maxWaitMs);
      at->sendAT(GF("+QICLOSE="), mux);
      sock_connected = false;
      at->waitResponse((maxWaitMs - (millis() - startMillis)));
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
/*
  class GsmClientSecureBG96 : public GsmClientBG96
  {
  public:
    GsmClientSecure() {}

    GsmClientSecure(SimpleNBBG96& modem, uint8_t mux = 0)
     : public GsmClient(modem, mux)
    {}


  public:
    int connect(const char* host, uint16_t port, int timeout_s) override {
      stop();
      SIMPLE_NB_YIELD();
      rx.clear();
      sock_connected = at->modemConnect(host, port, mux, true, timeout_s);
      return sock_connected;
    }
    SIMPLE_NB_CLIENT_CONNECT_OVERRIDES
  };
*/

  /*
   * Constructor
   */
 public:
  explicit SimpleNBBG96(Stream& stream) : stream(stream) {
    memset(sockets, 0, sizeof(sockets));
  }

  /*
   * Basic functions
   */
 protected:
  bool initImpl(const char* pin = NULL) {
    DBG(GF("### SimpleNB Version:"), SIMPLENB_VERSION);
    DBG(GF("### SimpleNB Compiled Module:  SimpleNBClientBG96"));

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

    // Disable time and time zone URC's
    sendAT(GF("+CTZR=0"));
    if (waitResponse(10000L) != 1) { return false; }

    // Enable automatic time zone update
    sendAT(GF("+CTZU=1"));
    if (waitResponse(10000L) != 1) { return false; }

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
  bool restartImpl(const char* pin = NULL) {
    if (!testAT()) { return false; }
    if (!setPhoneFunctionality(1, true)) { return false; }
    waitResponse(10000L, GF("APP RDY"));
    return init(pin);
  }

  bool powerOffImpl() {
    sendAT(GF("+QPOWD=1"));
    //waitResponse(300);  // returns OK first
    return waitResponse(300, GF("POWERED DOWN")) == 1;
  }

  // When entering into sleep mode is enabled, DTR is pulled up, and WAKEUP_IN
  // is pulled up, the module can directly enter into sleep mode.If entering
  // into sleep mode is enabled, DTR is pulled down, and WAKEUP_IN is pulled
  // down, there is a need to pull the DTR pin and the WAKEUP_IN pin up first,
  // and then the module can enter into sleep mode.
  bool sleepEnableImpl(bool enable = true) {
    sendAT(GF("+QSCLK="), enable);
    return waitResponse() == 1;
  }

  bool setPhoneFunctionalityImpl(uint8_t fun, bool reset = false) {
    sendAT(GF("+CFUN="), fun, reset ? ",1" : "");
    return waitResponse(10000L, GF("OK")) == 1;
  }

  /*
   * Generic network functions
   */
 public:
  bool activateDataNetwork() {
    sendAT(GF("+QIACT=1"));
    if (waitResponse(150000L) != 1) { return false; }

    // check if activation success? it returns +CGACT: <cid>,<state>
    // <state> = 1, activated
    sendAT(GF("+QIACT?"));
    if (waitResponse(60000L, GF(ACK_NL "+QIACT:")) != 1) { return false; }
    streamSkipUntil(',');
    int16_t state = streamGetIntBefore(',');
    waitResponse();
    return state == 1;
  }

  bool deactivateDataNetwork() {
    sendAT(GF("+QIDEACT=1"));  // Deactivate the bearer context
    if (waitResponse(40000L) != 1) { return false; }

    return true;
  }

  RegStatus getRegistrationStatus() {
    // Check first for EPS registration
    RegStatus epsStatus = (RegStatus)getRegistrationStatusXREG("CEREG");

    // If we're connected on EPS, great!
    if (epsStatus == REG_OK_HOME || epsStatus == REG_OK_ROAMING) {
      return epsStatus;
    } else {
      // Otherwise, check generic network status
      return (RegStatus)getRegistrationStatusXREG("CREG");
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
  bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) {

    sendAT(GF("+CGATT=1"));  // attach to GPRS
    if (waitResponse(360000L) != 1) { return false; }

    // Configure the TCPIP Context
    sendAT(GF("+CGDCONT=1,1,\""), apn, GF("\",\""), user, GF("\",\""), pwd, GF("\""));
    if (waitResponse() != 1) { return false; }

    // Activate GPRS/CSD Context
    sendAT(GF("+CGACT=1,1"));
    if (waitResponse(150000L) != 1) { return false; }

    return true;

    // check if activation success? it returns +CGACT: <cid>,<state>
    // <state> = 1, activated
    sendAT(GF("+CGACT?"));
    if (waitResponse(60000L, GF(ACK_NL "+CGACT:")) != 1) { return false; }
    streamSkipUntil(',');
    int16_t state = streamGetIntBefore('\n');
    waitResponse();
    return state == 1;
  }

  bool gprsDisconnectImpl() {
    sendAT(GF("+CGACT=0,1"));
    if (waitResponse(40000L) != 1) { return false; }

    sendAT(GF("+CGATT=0"));  // detach from GPRS
    if (waitResponse(360000L) != 1) { return false; }

    return true;
  }

  /*
   * SIM card functions
   */
 protected:
  String getSimCCIDImpl() {
    sendAT(GF("+QCCID"));
    if (waitResponse(GF(ACK_NL "+QCCID:")) != 1) { return ""; }
    String res = stream.readStringUntil('\n');
    waitResponse();
    res.trim();
    return res;
  }

  /*
   * Phone Call functions
   */
 protected:
  // Can follow all of the phone call functions from the template

  /*
   * Messaging functions
   */
 protected:
  // Follows all messaging functions per template

  /*
   * GSM Location functions
   */
 protected:
  // NOTE:  As of application firmware version 01.016.01.016 triangulated
  // locations can be obtained via the QuecLocator service and accompanying AT
  // commands.  As this is a separate paid service which I do not have access
  // to, I am not implementing it here.

  /*
   * GPS/GNSS/GLONASS location functions
   */
 protected:
  // enable GPS
  bool enableGPSImpl() {
    sendAT(GF("+QGPS=1"));
    if (waitResponse() != 1) { return false; }
    return true;
  }

  bool disableGPSImpl() {
    sendAT(GF("+QGPSEND"));
    if (waitResponse() != 1) { return false; }
    return true;
  }

  // get the GPS output as a string
  String getGPSImpl() {
    sendAT(GF("+QGPSLOC=2"));
    if (waitResponse(10000L, GF(ACK_NL "+QGPSLOC:")) != 1) { return ""; }
    String res = stream.readStringUntil('\n');
    waitResponse();
    res.trim();
    return res;
  }

  // get GPS informations
  bool getGPSImpl(GPS_t& gps) {
    sendAT(GF("+QGPSLOC=2"));
    if (waitResponse(10000L, GF(ACK_NL "+QGPSLOC:")) != 1) {
      // NOTE:  Will return an error if the position isn't fixed
      return false;
    }

    // UTC date & Time
    gps.hour        = streamGetIntLength(2);      // Two digit hour
    gps.minute      = streamGetIntLength(2);      // Two digit minute
    gps.second      = static_cast<int>(streamGetFloatBefore(','));  // seconds

    gps.lat      = streamGetFloatBefore(',');  // Latitude
    gps.lon      = streamGetFloatBefore(',');  // Longitude
    gps.accuracy = streamGetFloatBefore(',');  // Horizontal precision
    gps.alt      = streamGetFloatBefore(',');  // Altitude from sea level
    streamSkipUntil(',');                   // GNSS positioning mode
    streamSkipUntil(',');  // Course Over Ground based on true north
    streamSkipUntil(',');  // Speed Over Ground in Km/h
    gps.speed = streamGetFloatBefore(',');  // Speed Over Ground in knots

    gps.day   = streamGetIntLength(2);    // Two digit day
    gps.month = streamGetIntLength(2);    // Two digit month
    gps.year  = streamGetIntBefore(',');  // Two digit year
    if (gps.year < 2000) gps.year += 2000;

    gps.usat = streamGetIntBefore(',');  // Number of satellites,
    streamSkipUntil('\n');  // The error code of the operation. If it is not
                            // 0, it is the type of error.

    waitResponse();  // Final OK
    return true;
  }

  /*
   * Time functions
   */
 protected:
  String getNetworkTimeImpl() {
    sendAT(GF("+QLTS=2"));
    if (waitResponse(2000L, GF("+QLTS: \"")) != 1) { return ""; }

    String res = stream.readStringUntil('"');
    waitResponse();  // Ends with OK
    return res;
  }

  // The BG96 returns UTC time instead of local time as other modules do in
  // response to CCLK, so we're using QLTS where we can specifically request
  // local time.
  bool getNetworkTimeImpl(DateTime_t& dt) {
    sendAT(GF("+QLTS=2"));
    if (waitResponse(2000L, GF("+QLTS: \"")) != 1) { return false; }

    // Date & Time
    dt.year       = streamGetIntBefore('/');
    if (dt.year < 2000) dt.year += 2000;
    dt.month      = streamGetIntBefore('/');
    dt.day        = streamGetIntBefore(',');
    dt.hour       = streamGetIntBefore(':');
    dt.minute     = streamGetIntBefore(':');
    dt.second     = streamGetIntLength(2);
    char tzSign = stream.read();
    dt.timezone   = streamGetIntBefore(',');
    if (tzSign == '-') { dt.timezone = dt.timezone * -1; }
    streamSkipUntil('\n');  // DST flag
    // Final OK
    waitResponse();
    return true;
  }

  /*
   * NTP server functions
   */

  byte NTPServerSyncImpl(String server = "pool.ntp.org", byte = -5) {
    // Request network synchronization
    // AT+QNTP=<contextID>,<server>[,<port>][,<autosettime>]
    sendAT(GF("+QNTP=1,\""), server, '"');
    if (waitResponse(10000L, GF("+QNTP:"))) {
      String result = stream.readStringUntil(',');
      streamSkipUntil('\n');
      result.trim();
      if (SimpleNBIsValidNumber(result)) {
        return result.toInt();
      }
    }
    return -1;
  }

  String ShowNTPErrorImpl(byte error) SIMPLE_NB_ATTR_NOT_IMPLEMENTED;

  /*
   * Battery functions
   */
 protected:
  // Can follow CBC as in the template

  /*
   * Temperature functions
   */
 protected:
  // get temperature in degree celsius
  uint16_t getTemperatureImpl() {
    sendAT(GF("+QTEMP"));
    if (waitResponse(GF(ACK_NL "+QTEMP:")) != 1) { return 0; }
    // return temperature in C
    uint16_t res =
        streamGetIntBefore(',');  // read PMIC (primary ic) temperature
    streamSkipUntil(',');         // skip XO temperature ??
    streamSkipUntil('\n');        // skip PA temperature ??
    // Wait for final OK
    waitResponse();
    return res;
  }

  /*
   * Client related functions
   */
 protected:
  bool modemConnect(const char* host, uint16_t port, uint8_t mux,
                    bool ssl = false, int timeout_s = 150) {
    if (ssl) { DBG("SSL not yet supported on this module!"); }

    uint32_t timeout_ms = ((uint32_t)timeout_s) * 1000;

    // <PDPcontextID>(1-16), <connectID>(0-11),
    // "TCP/UDP/TCP LISTENER/UDPSERVICE", "<IP_address>/<domain_name>",
    // <remote_port>,<local_port>,<access_mode>(0-2; 0=buffer)
    sendAT(GF("+QIOPEN=1,"), mux, GF(",\""), GF("TCP"), GF("\",\""), host,
           GF("\","), port, GF(",0,0"));
    waitResponse();

    if (waitResponse(timeout_ms, GF(ACK_NL "+QIOPEN:")) != 1) { return false; }

    if (streamGetIntBefore(',') != mux) { return false; }
    // Read status
    return (0 == streamGetIntBefore('\n'));
  }

  int16_t modemSend(const void* buff, size_t len, uint8_t mux) {
    sendAT(GF("+QISEND="), mux, ',', (uint16_t)len);
    if (waitResponse(GF(">")) != 1) { return 0; }
    stream.write(reinterpret_cast<const uint8_t*>(buff), len);
    stream.flush();
    if (waitResponse(GF(ACK_NL "SEND OK")) != 1) { return 0; }
    // TODO(?): Wait for ACK? AT+QISEND=id,0
    return len;
  }

  size_t modemRead(size_t size, uint8_t mux) {
    if (!sockets[mux]) return 0;
    sendAT(GF("+QIRD="), mux, ',', (uint16_t)size);
    if (waitResponse(GF("+QIRD:")) != 1) { return 0; }
    int16_t len = streamGetIntBefore('\n');

    for (int i = 0; i < len; i++) { moveCharFromStreamToFifo(mux); }
    waitResponse();
    DBG("### READ:", len, "from", mux);
    sockets[mux]->sock_available = modemGetAvailable(mux);
    return len;
  }

  size_t modemGetAvailable(uint8_t mux) {
    if (!sockets[mux]) return 0;
    sendAT(GF("+QIRD="), mux, GF(",0"));
    size_t result = 0;
    if (waitResponse(GF("+QIRD:")) == 1) {
      streamSkipUntil(',');  // Skip total received
      streamSkipUntil(',');  // Skip have read
      result = streamGetIntBefore('\n');
      if (result) { DBG("### DATA AVAILABLE:", result, "on", mux); }
      waitResponse();
    }
    if (!result) { sockets[mux]->sock_connected = modemGetConnected(mux); }
    return result;
  }

  bool modemGetConnected(uint8_t mux) {
    sendAT(GF("+QISTATE=1,"), mux);
    // +QISTATE: 0,"TCP","151.139.237.11",80,5087,4,1,0,0,"uart1"

    if (waitResponse(GF("+QISTATE:")) != 1) { return false; }

    streamSkipUntil(',');                  // Skip mux
    streamSkipUntil(',');                  // Skip socket type
    streamSkipUntil(',');                  // Skip remote ip
    streamSkipUntil(',');                  // Skip remote port
    streamSkipUntil(',');                  // Skip local port
    int8_t res = streamGetIntBefore(',');  // socket state

    waitResponse();

    // 0 Initial, 1 Opening, 2 Connected, 3 Listening, 4 Closing
    return 2 == res;
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
        } else if (data.endsWith(GF(ACK_NL "+QIURC:"))) {
          streamSkipUntil('\"');
          String urc = stream.readStringUntil('\"');
          streamSkipUntil(',');
          if (urc == "recv") {
            int8_t mux = streamGetIntBefore('\n');
            DBG("### URC RECV:", mux);
            if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
              sockets[mux]->got_data = true;
            }
          } else if (urc == "closed") {
            int8_t mux = streamGetIntBefore('\n');
            DBG("### URC CLOSE:", mux);
            if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
              sockets[mux]->sock_connected = false;
            }
          } else {
            streamSkipUntil('\n');
          }
          data = "";
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

 public:
  Stream& stream;

 protected:
  GsmClientBG96* sockets[SIMPLE_NB_MUX_COUNT];
  const char*    gsmNL = ACK_NL;
};

#endif  // SRC_SIMPLE_NB_CLIENTBG96_H_
