/**
 * @file       SimpleNBSIM7000SSL.tpp
 * @author     Volodymyr Shymanskyy
 * @author     Henry Cheung
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy.
 * @copyright  Copyright (c) 2021 Henry Cheung.
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_CLIENTSIM7000SSL_H_
#define SRC_SIMPLE_NB_CLIENTSIM7000SSL_H_

// #define SIMPLE_NB_DEBUG Serial
// #define SIMPLE_NB_USE_HEX

#define SIMPLE_NB_MUX_COUNT 2
#define SIMPLE_NB_BUFFER_READ_AND_CHECK_SIZE

#include "SimpleNBClientSIM70xx.h"
#include "SimpleNBTCP.tpp"
#include "SimpleNBSSL.tpp"
#include "SimpleNBGPS.tpp"
#include "SimpleNBGSMLocation.tpp"

class SimpleNBSim7000SSL
    : public SimpleNBSim70xx<SimpleNBSim7000SSL>,
      public SimpleNBTCP<SimpleNBSim7000SSL, SIMPLE_NB_MUX_COUNT>,
      public SimpleNBSSL<SimpleNBSim7000SSL>,
      public SimpleNBGPS<SimpleNBSim7000SSL>,
      public SimpleNBGSMLocation<SimpleNBSim7000SSL> {
  friend class SimpleNBSim70xx<SimpleNBSim7000SSL>;
  friend class SimpleNBTCP<SimpleNBSim7000SSL, SIMPLE_NB_MUX_COUNT>;
  friend class SimpleNBSSL<SimpleNBSim7000SSL>;
  friend class SimpleNBGPS<SimpleNBSim7000SSL>;
  friend class SimpleNBGSMLocation<SimpleNBSim7000SSL>;

  /*
   * Inner Client
   */
 public:
  class GsmClientSim7000SSL : public GsmClient {
    friend class SimpleNBSim7000SSL;

   public:
    GsmClientSim7000SSL() {}

    explicit GsmClientSim7000SSL(SimpleNBSim7000SSL& modem, uint8_t mux = 0) {
      init(&modem, mux);
    }

    bool init(SimpleNBSim7000SSL* modem, uint8_t mux = 0) {
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
      at->sendAT(GF("+CACLOSE="), mux);
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

  class GsmClientSecureSIM7000SSL : public GsmClientSim7000SSL {
   public:
    GsmClientSecureSIM7000SSL() {}

    explicit GsmClientSecureSIM7000SSL(SimpleNBSim7000SSL& modem, uint8_t mux = 0)
        : GsmClientSim7000SSL(modem, mux) {}

   public:
    bool setCertificate(const String& certificateName) {
      return at->setCertificate(certificateName, mux);
    }

    virtual int connect(const char* host, uint16_t port,
                        int timeout_s) override {
      stop();
      SIMPLE_NB_YIELD();
      rx.clear();
      sock_connected = at->modemConnect(host, port, mux, true, timeout_s);
      return sock_connected;
    }
    SIMPLE_NB_CLIENT_CONNECT_OVERRIDES
  };

  /*
   * Constructor
   */
 public:
  explicit SimpleNBSim7000SSL(Stream& stream)
      : SimpleNBSim70xx<SimpleNBSim7000SSL>(stream),
        certificates() {
    memset(sockets, 0, sizeof(sockets));
  }

  /*
   * Basic functions
   */
 protected:
  bool initImpl(const char* pin = NULL) {
    DBG(GF("### SimpleNB Version:"), SIMPLENB_VERSION);
    DBG(GF("### SimpleNB Compiled Module:  SimpleNBClientSIM7000SSL"));

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

    // Enable battery checks
    // This command return +CME ERROR: Call index error
    // Battery Status is available even without this command
    // sendAT(GF("+CBATCHK=1"));
    // if (waitResponse() != 1) { return false; }

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

  void maintainImpl() {
    // Keep listening for modem URC's and proactively iterate through
    // sockets asking if any data is avaiable
    bool check_socks = false;
    for (int mux = 0; mux < SIMPLE_NB_MUX_COUNT; mux++) {
      GsmClientSim7000SSL* sock = sockets[mux];
      if (sock && sock->got_data) {
        sock->got_data = false;
        check_socks    = true;
      }
    }
    // modemGetAvailable checks all socks, so we only want to do it once
    // modemGetAvailable calls modemGetConnected(), which also checks allf
    if (check_socks) { modemGetAvailable(0); }
    while (stream.available()) { waitResponse(15, NULL, NULL); }
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
     sendAT(GF("+CNACT=1,\""), _apn, GF("\""));
     res = waitResponse(60000L, GF(ACK_NL "+APP PDP: 0,ACTIVE"), GF(ACK_NL "+APP PDP: 0,DEACTIVE"));
     waitResponse();
     ntries++;
   }
   return res;
  }

  bool deactivateDataNetwork() {
   sendAT(GF("+CNACT=0"));
   if (waitResponse(60000L) != 1) { return false; }
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
   * Secure socket layer functions
   */
 protected:
  bool setCertificate(const String& certificateName, const uint8_t mux = 0) {
    if (mux >= SIMPLE_NB_MUX_COUNT) return false;
    certificates[mux] = certificateName;
    return true;
  }

  /*
   * GPRS functions
   */
 protected:
  bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) {
    gprsDisconnect();

    // Define the PDP context
    sendAT(GF("+CGDCONT=1,\"IP\",\""), apn, '"');
    waitResponse();

    // Attach to GPRS
    sendAT(GF("+CGATT=1"));
    if (waitResponse(60000L) != 1) { return false; }

    // NOTE:  **DO NOT** activate the PDP context
    // For who only knows what reason, doing so screws up the rest of the
    // process

    // Bearer settings for applications based on IP
    // Set the user name and password
    // AT+CNCFG=<ip_type>[,<APN>[,<usename>,<password>[,<authentication>]]]
    //<ip_type> 0: Dual PDN Stack
    //          1: Internet Protocol Version 4
    //          2: Internet Protocol Version 6
    //<authentication> 0: NONE
    //                 1: PAP
    //                 2: CHAP
    //                 3: PAP or CHAP
    if (pwd && strlen(pwd) > 0 && user && strlen(user) > 0) {
      sendAT(GF("+CNCFG=1,\""), apn, "\",\"", "\",\"", user, pwd, '"');
      waitResponse();
    } else if (user && strlen(user) > 0) {
      // Set the user name only
      sendAT(GF("+CNCFG=1,\""), apn, "\",\"", user, '"');
      waitResponse();
    } else {
      // Set the APN only
      sendAT(GF("+CNCFG=1,\""), apn, '"');
      waitResponse();
    }

    // Activate application network connection
    // This is for most other supported applications outside of the
    // TCP application toolkit (ie, SSL)
    // AT+CNACT=<mode>,<action>
    // <mode> 0: Deactive
    //        1: Active
    //        2: Auto Active
    bool res    = false;
    int  ntries = 0;
    while (!res && ntries < 5) {
      sendAT(GF("+CNACT=1,\""), apn, GF("\""));
      res = waitResponse(60000L, GF(ACK_NL "+APP PDP: ACTIVE"),
                         GF(ACK_NL "+APP PDP: DEACTIVE")) == 1;
      waitResponse();
      ntries++;
    }

    return res;
  }

  bool gprsDisconnectImpl() {
    // Shut down the general application TCP/IP connection
    // CNACT will close *all* open application connections
    sendAT(GF("+CNACT=0"));
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
 String getGPSImpl(unsigned long gps_timeout) {
   sendAT(GF("+CGNSINF"));
   if (waitResponse(gps_timeout, GF(ACK_NL "+CGNSINF:")) != 1) {
     return "";
   }
   String res = stream.readStringUntil('\n');
   waitResponse();
   res.trim();
   return res;
 }

 // get GPS informations
 bool getGPSImpl(GPS_t gps, unsigned long gps_timeout) {
   sendAT(GF("+CGNSINF"));
   if (waitResponse(gps_timeout, GF(ACK_NL "+CGNSINF:")) != 1) {
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
    uint32_t timeout_ms = ((uint32_t)timeout_s) * 1000;

    // set the connection (mux) identifier to use
    sendAT(GF("+CACID="), mux);
    if (waitResponse(timeout_ms) != 1) return false;


    if (ssl) {
      // set the ssl version
      // AT+CSSLCFG="SSLVERSION",<ctxindex>,<sslversion>
      // <ctxindex> PDP context identifier
      // <sslversion> 0: QAPI_NET_SSL_PROTOCOL_UNKNOWN
      //              1: QAPI_NET_SSL_PROTOCOL_TLS_1_0
      //              2: QAPI_NET_SSL_PROTOCOL_TLS_1_1
      //              3: QAPI_NET_SSL_PROTOCOL_TLS_1_2
      //              4: QAPI_NET_SSL_PROTOCOL_DTLS_1_0
      //              5: QAPI_NET_SSL_PROTOCOL_DTLS_1_2
      // NOTE:  despite docs using caps, "sslversion" must be in lower case
      sendAT(GF("+CSSLCFG=\"sslversion\",0,3"));  // TLS 1.2
      if (waitResponse(5000L) != 1) return false;
    }

    // enable or disable ssl
    // AT+CASSLCFG=<cid>,"SSL",<sslFlag>
    // <cid> Application connection ID (set with AT+CACID above)
    // <sslFlag> 0: Not support SSL
    //           1: Support SSL
    sendAT(GF("+CASSLCFG="), mux, ',', GF("ssl,"), ssl);
    waitResponse();

    if (ssl) {
      // set the PDP context to apply SSL to
      // AT+CSSLCFG="CTXINDEX",<ctxindex>
      // <ctxindex> PDP context identifier
      // NOTE:  despite docs using caps, "ctxindex" must be in lower case
      sendAT(GF("+CSSLCFG=\"ctxindex\",0"));
      if (waitResponse(5000L, GF("+CSSLCFG:")) != 1) return false;
      streamSkipUntil('\n');  // read out the certificate information
      waitResponse();

      if (certificates[mux] != "") {
        // apply the correct certificate to the connection
        // AT+CASSLCFG=<cid>,"CACERT",<caname>
        // <cid> Application connection ID (set with AT+CACID above)
        // <certname> certificate name
        sendAT(GF("+CASSLCFG="), mux, ",CACERT,\"", certificates[mux].c_str(),
               "\"");
        if (waitResponse(5000L) != 1) return false;
      }

      // set the protocol
      // 0:  TCP; 1: UDP
      sendAT(GF("+CASSLCFG="), mux, ',', GF("protocol,0"));
      waitResponse();

      // set the SSL SNI (server name indication)
      // NOTE:  despite docs using caps, "sni" must be in lower case
      sendAT(GF("+CSSLCFG=\"sni\","), mux, ',', GF("\""), host, GF("\""));
      waitResponse();
    }

    // actually open the connection
    // AT+CAOPEN=<cid>[,<conn_type>],<server>,<port>
    // <cid> TCP/UDP identifier
    // <conn_type> "TCP" or "UDP"
    // NOTE:  the "TCP" can't be included
    sendAT(GF("+CAOPEN="), mux, GF(",\""), host, GF("\","), port);
    if (waitResponse(timeout_ms, GF(ACK_NL "+CAOPEN:")) != 1) { return 0; }
    // returns OK/r/n/r/n+CAOPEN: <cid>,<result>
    // <result> 0: Success
    //          1: Socket error
    //          2: No memory
    //          3: Connection limit
    //          4: Parameter invalid
    //          6: Invalid IP address
    //          7: Not support the function
    //          12: Can’t bind the port
    //          13: Can’t listen the port
    //          20: Can’t resolve the host
    //          21: Network not active
    //          23: Remote refuse
    //          24: Certificate’s time expired
    //          25: Certificate’s common name does not match
    //          26: Certificate’s common name does not match and time expired
    //          27: Connect failed
    streamSkipUntil(',');  // Skip mux

    // make sure the connection really opened
    int8_t res = streamGetIntBefore('\n');
    waitResponse();

    return 0 == res;
  }

  int16_t modemSend(const void* buff, size_t len, uint8_t mux) {
    // send data on prompt
    sendAT(GF("+CASEND="), mux, ',', (uint16_t)len);
    if (waitResponse(GF(">")) != 1) { return 0; }

    stream.write(reinterpret_cast<const uint8_t*>(buff), len);
    stream.flush();

    // after posting data, module responds with:
    //+CASEND: <cid>,<result>,<sendlen>
    if (waitResponse(GF(ACK_NL "+CASEND:")) != 1) { return 0; }
    streamSkipUntil(',');                            // Skip mux
    if (streamGetIntBefore(',') != 0) { return 0; }  // If result != success
    return streamGetIntBefore('\n');
  }

  size_t modemRead(size_t size, uint8_t mux) {
    if (!sockets[mux]) { return 0; }

    sendAT(GF("+CARECV="), mux, ',', (uint16_t)size);

    if (waitResponse(GF("+CARECV:")) != 1) { return 0; }

    // uint8_t ret_mux = stream.parseInt();
    // streamSkipUntil(',');
    // const int16_t len_confirmed = streamGetIntBefore('\n');
    // DBG("### READING:", len_confirmed, "from", ret_mux);

    // if (ret_mux != mux) {
    //   DBG("### Data from wrong mux! Got", ret_mux, "expected", mux);
    //   waitResponse();
    //   sockets[mux]->sock_available = modemGetAvailable(mux);
    //   return 0;
    // }

    // NOTE:  manual says the mux number is returned before the number of
    // characters available, but in tests only the number is returned

    int16_t len_confirmed = stream.parseInt();
    streamSkipUntil(',');  // skip the comma
    if (len_confirmed <= 0) {
      waitResponse();
      sockets[mux]->sock_available = modemGetAvailable(mux);
      return 0;
    }

    for (int i = 0; i < len_confirmed; i++) {
      uint32_t startMillis = millis();
      while (!stream.available() &&
             (millis() - startMillis < sockets[mux]->_timeout)) {
        SIMPLE_NB_YIELD();
      }
      char c = stream.read();
      sockets[mux]->rx.put(c);
    }
    waitResponse();
    // DBG("### READ:", len_confirmed, "from", mux);
    // make sure the sock available number is accurate again
    // the module is **EXTREMELY** testy about being asked to read more from
    // the buffer than exits; it will freeze until a hard reset or power cycle!
    sockets[mux]->sock_available = modemGetAvailable(mux);
    return len_confirmed;
  }

  size_t modemGetAvailable(uint8_t mux) {
    // If the socket doesn't exist, just return
    if (!sockets[mux]) { return 0; }
    // We need to check if there are any connections open *before* checking for
    // available characters.  The SIM7000 *will crash* if you ask about data
    // when there are no open connections.
    if (!modemGetConnected(mux)) { return 0; }
    // NOTE: This gets how many characters are available on all connections that
    // have data.  It does not return all the connections, just those with data.
    sendAT(GF("+CARECV?"));
    for (int muxNo = 0; muxNo < SIMPLE_NB_MUX_COUNT; muxNo++) {
      // after the last connection, there's an ok, so we catch it right away
      int res = waitResponse(3000, GF("+CARECV:"), GFP(ACK_OK), GFP(ACK_ERROR));
      // if we get the +CARECV: response, read the mux number and the number of
      // characters available
      if (res == 1) {
        int                  ret_mux = streamGetIntBefore(',');
        size_t               result  = streamGetIntBefore('\n');
        GsmClientSim7000SSL* sock    = sockets[ret_mux];
        if (sock) { sock->sock_available = result; }
        // if the first returned mux isn't 0 (or is higher than expected)
        // we need to fill in the missing muxes
        if (ret_mux > muxNo) {
          for (int extra_mux = muxNo; extra_mux < ret_mux; extra_mux++) {
            GsmClientSim7000SSL* isock = sockets[extra_mux];
            if (isock) { isock->sock_available = 0; }
          }
          muxNo = ret_mux;
        }
      } else if (res == 2) {
        // if we get an OK, we've reached the last socket with available data
        // so we set any we haven't gotten to yet to 0
        for (int extra_mux = muxNo; extra_mux < SIMPLE_NB_MUX_COUNT;
             extra_mux++) {
          GsmClientSim7000SSL* isock = sockets[extra_mux];
          if (isock) { isock->sock_available = 0; }
        }
        break;
      } else {
        // if we got an error, give up
        break;
      }
      // Should be a final OK at the end.
      // If every connection was returned, catch the OK here.
      // If only a portion were returned, catch it above.
      if (muxNo == SIMPLE_NB_MUX_COUNT - 1) { waitResponse(); }
    }
    modemGetConnected(mux);  // check the state of all connections
    if (!sockets[mux]) { return 0; }
    return sockets[mux]->sock_available;
  }

  bool modemGetConnected(uint8_t mux) {
    // NOTE:  This gets the state of all connections that have been opened
    // since the last connection
    sendAT(GF("+CASTATE?"));

    for (int muxNo = 0; muxNo < SIMPLE_NB_MUX_COUNT; muxNo++) {
      // after the last connection, there's an ok, so we catch it right away
      int res = waitResponse(3000, GF("+CASTATE:"), GFP(ACK_OK),
                             GFP(ACK_ERROR));
      // if we get the +CASTATE: response, read the mux number and the status
      if (res == 1) {
        int    ret_mux = streamGetIntBefore(',');
        size_t status  = streamGetIntBefore('\n');
        // 0: Closed by remote server or internal error
        // 1: Connected to remote server
        // 2: Listening (server mode)
        GsmClientSim7000SSL* sock = sockets[ret_mux];
        if (sock) { sock->sock_connected = (status == 1); }
        // if the first returned mux isn't 0 (or is higher than expected)
        // we need to fill in the missing muxes
        if (ret_mux > muxNo) {
          for (int extra_mux = muxNo; extra_mux < ret_mux; extra_mux++) {
            GsmClientSim7000SSL* isock = sockets[extra_mux];
            if (isock) { isock->sock_connected = false; }
          }
          muxNo = ret_mux;
        }
      } else if (res == 2) {
        // if we get an OK, we've reached the last socket with available data
        // so we set any we haven't gotten to yet to 0
        for (int extra_mux = muxNo; extra_mux < SIMPLE_NB_MUX_COUNT;
             extra_mux++) {
          GsmClientSim7000SSL* isock = sockets[extra_mux];
          if (isock) { isock->sock_connected = false; }
        }
        break;
      } else {
        // if we got an error, give up
        break;
      }
      // Should be a final OK at the end.
      // If every connection was returned, catch the OK here.
      // If only a portion were returned, catch it above.
      if (muxNo == SIMPLE_NB_MUX_COUNT - 1) { waitResponse(); }
    }
    return sockets[mux]->sock_connected;
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
        } else if (data.endsWith(GF("+CARECV:"))) {
          int8_t  mux = streamGetIntBefore(',');
          int16_t len = streamGetIntBefore('\n');
          if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
            sockets[mux]->got_data = true;
            if (len >= 0 && len <= 1024) { sockets[mux]->sock_available = len; }
          }
          data = "";
          DBG("### READ: " + String(len) + " from " + String(mux));
        } else if (data.endsWith(GF("+CADATAIND:"))) {
          int8_t mux = streamGetIntBefore('\n');
          if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
            sockets[mux]->got_data = true;
          }
          data = "";
          DBG("### Got Data on socket:", mux);
        } else if (data.endsWith(GF("+CASTATE:"))) {
          int8_t mux   = streamGetIntBefore(',');
          int8_t state = streamGetIntBefore('\n');
          if (mux >= 0 && mux < SIMPLE_NB_MUX_COUNT && sockets[mux]) {
            if (state != 1) {
              sockets[mux]->sock_connected = false;
              DBG("### Closed: ", mux);
            }
          }
          data = "";
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

 protected:
  GsmClientSim7000SSL* sockets[SIMPLE_NB_MUX_COUNT];
  String               certificates[SIMPLE_NB_MUX_COUNT];
  String               _apn = "stmiot";  //TO-DO: remove hardcoded apn
};

#endif  // SRC_SIMPLE_NB_CLIENTSIM7000SSL_H_
