/**
 * @file       SimpleNBSSL.tpp
 * @author     Volodymyr Shymanskyy
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2016 Volodymyr Shymanskyy
 * @date       Nov 2016
 */

#ifndef SRC_SIMPLE_NB_SSL_H_
#define SRC_SIMPLE_NB_SSL_H_

#include "SimpleNBCommon.h"

#define SIMPLE_NB_SUPPORT_SSL


template <class modemType>
class SimpleNBSSL {
 public:
  /*
   * SSL functions
   */
  bool addCertificate(const char* filename) {
    return thisModem().addCertificateImpl(filename);
  }
  bool deleteCertificate() {
    return thisModem().deleteCertificateImpl();
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
   * Inner Secure Client
   */
  /*
 public:
  class GsmClientSecure : public GsmClient {
   public:
    GsmClientSecureSim800() {}

    explicit GsmClientSecureSim800(SimpleNBSim800& modem, uint8_t mux = 0)
        : GsmClientSim800(modem, mux) {}

   public:
    int connect(const char* host, uint16_t port, int timeout_s) overide {
      stop();
      SIMPLE_NB_YIELD();
      rx.clear();
      sock_connected = at->modemConnect(host, port, mux, true, timeout_s);
      return sock_connected;
    }
  };*/

  /*
   * SSL functions
   */
 protected:
  bool addCertificateImpl(const char* filename) SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
  bool deleteCertificateImpl() SIMPLE_NB_ATTR_NOT_IMPLEMENTED;
};

#endif  // SRC_SIMPLE_NB_SSL_H_
