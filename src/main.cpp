/**************************************************************
 *
 *  DO NOT USE THIS - this is just a compilation test!
 *  This is NOT an example for use of this library!
 *
 **************************************************************/
// #define SIMPLE_NB_MODEM_SIM7000
#define SIMPLE_NB_MODEM_SIM7020
// #define SIMPLE_NB_MODEM_SIM7070
// #define SIMPLE_NB_MODEM_SIM7080
// #define SIMPLE_NB_MODEM_SIM7090
// #define SIMPLE_NB_MODEM_BG96
// #define SIMPLE_NB_MODEM_SARAR4
// #define SIMPLE_NB_MODEM_UBLOX
// #define SIMPLE_NB_MODEM_SEQUANS_MONARCH
// #define SIMPLE_NB_MODEM_XBEE

#include <SimpleNBClient.h>

SimpleNB modem(Serial);

void setup() {
  Serial.begin(115200);
  delay(6000);
}

void loop() {
  // Test the basic functions
  modem.begin();
  modem.begin("1234");
  modem.init();
  modem.init("1234");
  modem.setBaud(115200);
  modem.testAT();

  modem.getModemInfo();
  modem.getModemName();
  modem.factoryDefault();

  // Test Power functions
  modem.restart();
  // modem.sleepEnable();  // Not available for all modems
  // modem.radioOff();  // Not available for all modems
  modem.poweroff();

  // Test generic network functions
  modem.getRegistrationStatus();
  modem.isNetworkConnected();
  modem.waitForNetwork();
  modem.waitForNetwork(15000L);
  modem.waitForNetwork(15000L, true);
  modem.getSignalQuality();
  modem.getLocalIP();
  modem.localIP();

// Test the GPRS and SIM card functions
#if defined(SIMPLE_NB_SUPPORT_GPRS)
  modem.simUnlock("1234");
  modem.getSimCCID();
  modem.getIMEI();
  modem.getIMSI();
  modem.getSimStatus();

  modem.gprsConnect("myAPN");
  modem.gprsConnect("myAPN", "myUser");
  modem.gprsConnect("myAPN", "myAPNUser", "myAPNPass");
  modem.gprsDisconnect();
  modem.getOperator();
#endif

  // Test TCP functions
  modem.maintain();
  SimpleNBClient client;
  SimpleNBClient client2(modem);
  SimpleNBClient client3(modem, 1);
  client.init(&modem);
  client.init(&modem, 1);

  char server[]   = "somewhere";
  char resource[] = "something";

  client.connect(server, 80);

  // Make a HTTP GET request:
  client.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  uint32_t timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    while (client.available()) {
      client.read();
      timeout = millis();
    }
  }

  client.stop();

#if defined(SIMPLE_NB_SUPPORT_SSL)
  // modem.addCertificate();  // not yet impemented
  // modem.deleteCertificate();  // not yet impemented
  SimpleNBClientSecure client_secure(modem);
  SimpleNBClientSecure client_secure2(modem);
  SimpleNBClientSecure client_secure3(modem, 1);
  client_secure.init(&modem);
  client_secure.init(&modem, 1);

  client_secure.connect(server, 443);

  // Make a HTTP GET request:
  client_secure.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client_secure.print(String("Host: ") + server + "\r\n");
  client_secure.print("Connection: close\r\n\r\n");

  timeout = millis();
  while (client_secure.connected() && millis() - timeout < 10000L) {
    while (client_secure.available()) {
      client_secure.read();
      timeout = millis();
    }
  }

  client_secure.stop();
#endif

// Test the calling functions
#if defined(SIMPLE_NB_SUPPORT_CALLING) && not defined(__AVR_ATmega32U4__)
  modem.callNumber(String("+380000000000"));
  modem.callHangup();

#if not defined(SIMPLE_NB_MODEM_SEQUANS_MONARCH)
  modem.callAnswer();
  modem.dtmfSend('A', 1000);
#endif

#endif

// Test the SMS functions
#if defined(SIMPLE_NB_SUPPORT_SMS) && not defined(__AVR_ATmega32U4__)
  modem.sendSMS(String("+380000000000"), String("Hello from "));

#if not defined(SIMPLE_NB_MODEM_XBEE) && not defined(SIMPLE_NB_MODEM_SARAR4)
  modem.sendUSSD("*111#");
#endif

#if not defined(SIMPLE_NB_MODEM_XBEE) && not defined(SIMPLE_NB_MODEM_M590) && \
    not defined(SIMPLE_NB_MODEM_SARAR4)
  modem.sendSMS_UTF16("+380000000000", "Hello", 5);
#endif

#endif

// Test the GSM location functions
#if defined(SIMPLE_NB_SUPPORT_GSM_LOCATION) && not defined(__AVR_ATmega32U4__)
  modem.getGsmLocation();
  CellLBS_t lbs;
  modem.getGsmLocation(lbs);
#endif

// Test the GPS functions
#if defined(SIMPLE_NB_SUPPORT_GPS) && not defined(__AVR_ATmega32U4__)
  modem.enableGPS();
  modem.getGPSraw();
  float latitude  = -9999;
  float longitude = -9999;
  float speed     = 0;
  float alt       = 0;
  int   vsat      = 0;
  int   usat      = 0;
  float acc       = 0;
  int   year      = 0;
  int   month     = 0;
  int   day       = 0;
  int   hour      = 0;
  int   minute    = 0;
  int   second    = 0;
  modem.getGPS(&latitude, &longitude);
  modem.getGPS(&latitude, &longitude, &speed, &alt, &vsat, &usat, &acc, &year,
               &month, &day, &hour, &minute, &second);
  modem.disableGPS();
#endif

// Test the Network time function
#if defined(SIMPLE_NB_SUPPORT_NTP) && not defined(__AVR_ATmega32U4__)
  modem.NTPServerSync("pool.ntp.org", 3);
#endif

// Test the Network time function
#if defined(SIMPLE_NB_SUPPORT_TIME) && not defined(__AVR_ATmega32U4__)
  modem.getNetworkTime();
  DateTime_t dt;
  modem.getNetworkTime(dt);
#endif

// Test Battery functions
#if defined(SIMPLE_NB_SUPPORT_BATTERY)
  Battery_t batt;
  modem.getBatteryStatus(batt);
#endif

// Test the temperature function
#if defined(SIMPLE_NB_SUPPORT_TEMPERATURE)
  modem.getTemperature();
#endif
}
