/**************************************************************
 *
 * SimpleNB README:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
 *
 * NOTE:
 * Some of the functions may be unavailable for your modem.
 * Just comment them out.
 *
 **************************************************************/

// Select your modem:
#define SIMPLE_NB_MODEM_SIM7000
// #define SIMPLE_NB_MODEM_SIM7000SSL
// #define SIMPLE_NB_MODEM_SIM7080
// #define SIMPLE_NB_MODEM_UBLOX
// #define SIMPLE_NB_MODEM_SARAR4
// #define SIMPLE_NB_MODEM_BG96
// #define SIMPLE_NB_MODEM_XBEE
// #define SIMPLE_NB_MODEM_SEQUANS_MONARCH

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#ifndef __AVR_ATmega328P__
#define SerialAT Serial1

// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define SIMPLE_NB_DEBUG SerialMon

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 57600

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define SIMPLE_NB_YIELD() { delay(2); }

/*
 * Tests enabled
 */
#define SIMPLE_NB_TEST_GPRS true
#define SIMPLE_NB_TEST_WIFI false
#define SIMPLE_NB_TEST_TCP true
#define SIMPLE_NB_TEST_SSL true
#define SIMPLE_NB_TEST_CALL false
#define SIMPLE_NB_TEST_SMS false
#define SIMPLE_NB_TEST_USSD false
#define SIMPLE_NB_TEST_BATTERY true
#define SIMPLE_NB_TEST_TEMPERATURE true
#define SIMPLE_NB_TEST_GSM_LOCATION false
#define SIMPLE_NB_TEST_NTP false
#define SIMPLE_NB_TEST_TIME false
#define SIMPLE_NB_TEST_GPS false
// disconnect and power down modem after tests
#define SIMPLE_NB_POWERDOWN false

// set GSM PIN, if any
#define GSM_PIN ""

// Set phone numbers, if you want to test SMS and Calls
// #define SMS_TARGET  "+380xxxxxxxxx"
// #define CALL_TARGET "+380xxxxxxxxx"

// Your GPRS credentials, if any
const char apn[] = "YourAPN";
// const char apn[] = "ibasis.iot";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// Server details to test TCP/SSL
const char server[]   = "vsh.pp.ua";
const char resource[] = "/SimpleNB/logo.txt";

#include <SimpleNBClient.h>

#if SIMPLE_NB_TEST_GPRS && not defined SIMPLE_NB_SUPPORT_GPRS
#undef SIMPLE_NB_TEST_GPRS
#undef SIMPLE_NB_TEST_WIFI
#define SIMPLE_NB_TEST_GPRS false
#define SIMPLE_NB_TEST_WIFI true
#endif
#if SIMPLE_NB_TEST_WIFI && not defined SIMPLE_NB_SUPPORT_WIFI
#undef SIMPLE_NB_USE_GPRS
#undef SIMPLE_NB_USE_WIFI
#define SIMPLE_NB_USE_GPRS true
#define SIMPLE_NB_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
SimpleNB        modem(debugger);
#else
SimpleNB        modem(SerialAT);
#endif

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  DBG("Wait...");
  delay(6000);

  // Set GSM module baud rate
  SimpleNBAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  // SerialAT.begin(9600);
}

void loop() {
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  DBG("Initializing modem...");
  if (!modem.restart()) {
    // if (!modem.init()) {
    DBG("Failed to restart modem, delaying 10s and retrying");
    // restart autobaud in case GSM just rebooted
    // SimpleNBAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    return;
  }

  String name = modem.getModemName();
  DBG("Modem Name:", name);

  String modemInfo = modem.getModemInfo();
  DBG("Modem Info:", modemInfo);

#if SIMPLE_NB_TEST_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif

#if SIMPLE_NB_TEST_WIFI
  DBG("Setting SSID/password...");
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    DBG(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

#if SIMPLE_NB_TEST_GPRS && defined SIMPLE_NB_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  DBG("Waiting for network...");
  if (!modem.waitForNetwork(600000L, true)) {
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected()) { DBG("Network connected"); }

#if SIMPLE_NB_TEST_GPRS
  DBG("Connecting to", apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    delay(10000);
    return;
  }

  bool res = modem.isGprsConnected();
  DBG("GPRS status:", res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);

  String imei = modem.getIMEI();
  DBG("IMEI:", imei);

  String imsi = modem.getIMSI();
  DBG("IMSI:", imsi);

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  IPAddress local = modem.localIP();
  DBG("Local IP:", local);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);
#endif

#if SIMPLE_NB_TEST_USSD && defined SIMPLE_NB_SUPPORT_SMS
  String ussd_balance = modem.sendUSSD("*111#");
  DBG("Balance (USSD):", ussd_balance);

  String ussd_phone_num = modem.sendUSSD("*161#");
  DBG("Phone number (USSD):", ussd_phone_num);
#endif

#if SIMPLE_NB_TEST_TCP && defined SIMPLE_NB_SUPPORT_TCP
  SimpleNBClient client(modem, 0);
  const int     port = 80;
  DBG("Connecting to", server);
  if (!client.connect(server, port)) {
    DBG("... failed");
  } else {
    // Make a HTTP GET request:
    client.print(String("GET ") + resource + " HTTP/1.0\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t start = millis();
    while (client.connected() && !client.available() &&
           millis() - start < 30000L) {
      delay(100);
    };

    // Read data
    start          = millis();
    char logo[640] = {
        '\0',
    };
    int read_chars = 0;
    while (client.connected() && millis() - start < 10000L) {
      while (client.available()) {
        logo[read_chars]     = client.read();
        logo[read_chars + 1] = '\0';
        read_chars++;
        start = millis();
      }
    }
    SerialMon.println(logo);
    DBG("#####  RECEIVED:", strlen(logo), "CHARACTERS");
    client.stop();
  }
#endif

#if SIMPLE_NB_TEST_SSL && defined SIMPLE_NB_SUPPORT_SSL
  SimpleNBClientSecure secureClient(modem, 1);
  const int           securePort = 443;
  DBG("Connecting securely to", server);
  if (!secureClient.connect(server, securePort)) {
    DBG("... failed");
  } else {
    // Make a HTTP GET request:
    secureClient.print(String("GET ") + resource + " HTTP/1.0\r\n");
    secureClient.print(String("Host: ") + server + "\r\n");
    secureClient.print("Connection: close\r\n\r\n");

    // Wait for data to arrive
    uint32_t startS = millis();
    while (secureClient.connected() && !secureClient.available() &&
           millis() - startS < 30000L) {
      delay(100);
    };

    // Read data
    startS          = millis();
    char logoS[640] = {
        '\0',
    };
    int read_charsS = 0;
    while (secureClient.connected() && millis() - startS < 10000L) {
      while (secureClient.available()) {
        logoS[read_charsS]     = secureClient.read();
        logoS[read_charsS + 1] = '\0';
        read_charsS++;
        startS = millis();
      }
    }
    SerialMon.println(logoS);
    DBG("#####  RECEIVED:", strlen(logoS), "CHARACTERS");
    secureClient.stop();
  }
#endif

#if SIMPLE_NB_TEST_CALL && defined SIMPLE_NB_SUPPORT_CALLING && \
    defined                       CALL_TARGET
  DBG("Calling:", CALL_TARGET);

  // This is NOT supported on M590
  res = modem.callNumber(CALL_TARGET);
  DBG("Call:", res ? "OK" : "fail");

  if (res) {
    delay(1000L);

    // Play DTMF A, duration 1000ms
    modem.dtmfSend('A', 1000);

    // Play DTMF 0..4, default duration (100ms)
    for (char tone = '0'; tone <= '4'; tone++) { modem.dtmfSend(tone); }

    delay(5000);

    res = modem.callHangup();
    DBG("Hang up:", res ? "OK" : "fail");
  }
#endif

#if SIMPLE_NB_TEST_SMS && defined SIMPLE_NB_SUPPORT_SMS && defined SMS_TARGET
  res = modem.sendSMS(SMS_TARGET, String("Hello from ") + imei);
  DBG("SMS:", res ? "OK" : "fail");

  // This is only supported on SIMxxx series
  res = modem.sendSMS_UTF8_begin(SMS_TARGET);
  if (res) {
    auto stream = modem.sendSMS_UTF8_stream();
    stream.print(F("Привіііт! Print number: "));
    stream.print(595);
    res = modem.sendSMS_UTF8_end();
  }
  DBG("UTF8 SMS:", res ? "OK" : "fail");

#endif

#if SIMPLE_NB_TEST_GSM_LOCATION && defined SIMPLE_NB_SUPPORT_GSM_LOCATION
  float lat      = 0;
  float lon      = 0;
  float accuracy = 0;
  int   year     = 0;
  int   month    = 0;
  int   day      = 0;
  int   hour     = 0;
  int   min      = 0;
  int   sec      = 0;
  for (int8_t i = 15; i; i--) {
    DBG("Requesting current GSM location");
    if (modem.getGsmLocation(&lat, &lon, &accuracy, &year, &month, &day, &hour,
                             &min, &sec)) {
      DBG("Latitude:", String(lat, 8), "\tLongitude:", String(lon, 8));
      DBG("Accuracy:", accuracy);
      DBG("Year:", year, "\tMonth:", month, "\tDay:", day);
      DBG("Hour:", hour, "\tMinute:", min, "\tSecond:", sec);
      break;
    } else {
      DBG("Couldn't get GSM location, retrying in 15s.");
      delay(15000L);
    }
  }
  DBG("Retrieving GSM location again as a string");
  String location = modem.getGsmLocation();
  DBG("GSM Based Location String:", location);
#endif

#if SIMPLE_NB_TEST_GPS && defined SIMPLE_NB_SUPPORT_GPS
  DBG("Enabling GPS/GNSS/GLONASS and waiting 15s for warm-up");
  modem.enableGPS();
  delay(15000L);
  float lat2      = 0;
  float lon2      = 0;
  float speed2    = 0;
  float alt2      = 0;
  int   vsat2     = 0;
  int   usat2     = 0;
  float accuracy2 = 0;
  int   year2     = 0;
  int   month2    = 0;
  int   day2      = 0;
  int   hour2     = 0;
  int   min2      = 0;
  int   sec2      = 0;
  for (int8_t i = 15; i; i--) {
    DBG("Requesting current GPS/GNSS/GLONASS location");
    if (modem.getGPS(&lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2,
                     &year2, &month2, &day2, &hour2, &min2, &sec2)) {
      DBG("Latitude:", String(lat2, 8), "\tLongitude:", String(lon2, 8));
      DBG("Speed:", speed2, "\tAltitude:", alt2);
      DBG("Visible Satellites:", vsat2, "\tUsed Satellites:", usat2);
      DBG("Accuracy:", accuracy2);
      DBG("Year:", year2, "\tMonth:", month2, "\tDay:", day2);
      DBG("Hour:", hour2, "\tMinute:", min2, "\tSecond:", sec2);
      break;
    } else {
      DBG("Couldn't get GPS/GNSS/GLONASS location, retrying in 15s.");
      delay(15000L);
    }
  }
  DBG("Retrieving GPS/GNSS/GLONASS location again as a string");
  String gps_raw = modem.getGPSraw();
  DBG("GPS/GNSS Based Location String:", gps_raw);
  DBG("Disabling GPS");
  modem.disableGPS();
#endif

#if SIMPLE_NB_TEST_NTP && defined SIMPLE_NB_SUPPORT_NTP
  DBG("Asking modem to sync with NTP");
  modem.NTPServerSync("132.163.96.5", 20);
#endif

#if SIMPLE_NB_TEST_TIME && defined SIMPLE_NB_SUPPORT_TIME
  int   year3    = 0;
  int   month3   = 0;
  int   day3     = 0;
  int   hour3    = 0;
  int   min3     = 0;
  int   sec3     = 0;
  float timezone = 0;
  for (int8_t i = 5; i; i--) {
    DBG("Requesting current network time");
    if (modem.getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3,
                             &timezone)) {
      DBG("Year:", year3, "\tMonth:", month3, "\tDay:", day3);
      DBG("Hour:", hour3, "\tMinute:", min3, "\tSecond:", sec3);
      DBG("Timezone:", timezone);
      break;
    } else {
      DBG("Couldn't get network time, retrying in 15s.");
      delay(15000L);
    }
  }
  DBG("Retrieving time again as a string");
  String time = modem.getGSMDateTime(DATE_FULL);
  DBG("Current Network Time:", time);
#endif

#if SIMPLE_NB_TEST_BATTERY && defined SIMPLE_NB_SUPPORT_BATTERY
  uint8_t  chargeState = -99;
  int8_t   percent     = -99;
  uint16_t milliVolts  = -9999;
  modem.getBattStats(chargeState, percent, milliVolts);
  DBG("Battery charge state:", chargeState);
  DBG("Battery charge 'percent':", percent);
  DBG("Battery voltage:", milliVolts / 1000.0F);
#endif

#if SIMPLE_NB_TEST_TEMPERATURE && defined SIMPLE_NB_SUPPORT_TEMPERATURE
  float temp = modem.getTemperature();
  DBG("Chip temperature:", temp);
#endif

#if SIMPLE_NB_POWERDOWN

#if SIMPLE_NB_TEST_GPRS
  modem.gprsDisconnect();
  delay(5000L);
  if (!modem.isGprsConnected()) {
    DBG("GPRS disconnected");
  } else {
    DBG("GPRS disconnect: Failed.");
  }
#endif

#if SIMPLE_NB_TEST_WIFI
  modem.networkDisconnect();
  DBG("WiFi disconnected");
#endif

  // Try to power-off (modem may decide to restart automatically)
  // To turn off modem completely, please use Reset/Enable pins
  modem.poweroff();
  DBG("Poweroff.");
#endif

  DBG("End of tests.");

  // Do nothing forevermore
  while (true) { modem.maintain(); }
}
