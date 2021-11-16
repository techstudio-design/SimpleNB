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
// #define SIMPLE_NB_MODEM_SIM7000
// #define SIMPLE_NB_MODEM_SIM7000SSL
// #define SIMPLE_NB_MODEM_SIM7020
// #define SIMPLE_NB_MODEM_SIM7070
#define SIMPLE_NB_MODEM_SIM7080
// #define SIMPLE_NB_MODEM_SIM7090
// #define SIMPLE_NB_MODEM_BG96
// #define SIMPLE_NB_MODEM_SARAR4
// #define SIMPLE_NB_MODEM_UBLOX
// #define SIMPLE_NB_MODEM_SEQUANS_MONARCH
// #define SIMPLE_NB_MODEM_XBEE

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#ifndef __AVR_ATmega328P__
#define SerialAT Serial2

// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define SIMPLE_NB_YIELD() { delay(2); }

/*
 * Tests enabled
 */
#define SIMPLE_NB_TEST_GPRS false
#define SIMPLE_NB_TEST_DATA true  // only for SIM70x0 series
#define SIMPLE_NB_TEST_TCP true
#define SIMPLE_NB_TEST_SSL false
#define SIMPLE_NB_TEST_BATTERY true
#define SIMPLE_NB_TEST_GSM_LOCATION true
#define SIMPLE_NB_TEST_NTP true
#define SIMPLE_NB_TEST_TIME true
#define SIMPLE_NB_TEST_GPS false
// disconnect and power down modem after tests
#define SIMPLE_NB_POWERDOWN true
#define SIMPLE_NB_TEST_TEMPERATURE false
#define SIMPLE_NB_TEST_CALL false
#define SIMPLE_NB_TEST_SMS false
#define SIMPLE_NB_TEST_USSD false

// set GSM PIN, if any
#define GSM_PIN ""

#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

// Set phone numbers, if you want to test SMS and Calls
// #define SMS_TARGET  "+380xxxxxxxxx"
// #define CALL_TARGET "+380xxxxxxxxx"

// Your GPRS credentials, if any
const char apn[] = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details to test TCP/SSL
const char server[]   = "postman-echo.com";
const char resource[] = "/ip";
const int port = 80;
const int securePort = 443;

bool res = false;

#include <SimpleNBClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
SimpleNB        modem(debugger);
#else
SimpleNB        modem(SerialAT);
#endif

// different modem may have different time requirements, check datasheet of your modem
void powerUp() {
    digitalWrite(PWRKEY, LOW);
    delay(1000);
    digitalWrite(PWRKEY, HIGH);
    delay(2500);
}

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);
  Serial.println();

  DBG("Powering Up...");
  digitalWrite(PWRKEY, HIGH);
  pinMode(PWRKEY, OUTPUT);
  powerUp();

  // Set GSM module baud rate
  SimpleNBBegin(SerialAT, 115200);
  // If unsure the baud rate work, alternatively use
  // SimpleNBAutoBaud(SerialAT, 9600, 115200);
}

void loop() {
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  // if (!modem.restart()) {
  if (!modem.init()) {
    DBG("Failed to initialize modem, delaying 10s and retrying");
    // restart autobaud in case GSM just rebooted
    // SimpleNBBegin(SerialAT, 115200);
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

#if SIMPLE_NB_TEST_GPRS && defined SIMPLE_NB_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  DBG("Waiting for network registration...");
  if (!modem.waitForRegistration(60000L, true)) {
    delay(1000);
    return;
  }

  if (modem.isNetworkRegistered()) { DBG("Network registered"); }


#if SIMPLE_NB_TEST_DATA
  DBG("Activate Data Network... ");
  if (!modem.activateDataNetwork()) {
    delay(1000);
    return;
  }
#endif

#if SIMPLE_NB_TEST_GPRS
  DBG("Connecting to", apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    delay(10000);
    return;
  }

  res = modem.isGprsConnected();
  DBG("GPRS status:", res ? "connected" : "not connected");
#endif

  String ccid = modem.getSimCCID();
  DBG("CCID:", ccid);

  String imei = modem.getIMEI();
  DBG("IMEI:", imei);

  String imsi = modem.getIMSI();
  DBG("IMSI:", imsi);

  String cop = modem.getOperator();
  DBG("Operator:", cop);

  int csq = modem.getSignalQuality();
  DBG("Signal quality:", csq);

  // Use the following to get IP as a String
  // String local = modem.getLocalIP();
  IPAddress local = modem.localIP();
  DBG("Local IP:", local);

#if SIMPLE_NB_TEST_USSD && defined SIMPLE_NB_SUPPORT_SMS
   String mobile_imei = modem.sendUSSD("*#06#"); //this is universal USSD for getting IMEI
   DBG("IMEI (USSD):", mobile_imei);
#endif

#if SIMPLE_NB_TEST_TCP && defined SIMPLE_NB_SUPPORT_TCP
   SimpleNBClient client(modem, 0);
   DBG("Connecting to", server);
   if (!client.connect(server, port)) {
     DBG("... failed");
   } else {
     // Make a HTTP GET request:
     client.print(String("GET ") + resource + " HTTP/1.1\r\n");
     client.print(String("Host: ") + server + "\r\n");
     client.print("Connection: close\r\n\r\n");

     // Wait for data to arrive
     uint32_t start = millis();
     while (client.connected() && !client.available() &&
            millis() - start < 30000L) {
       delay(100);
     };

     // Read data
     start = millis();
     char logo[640] = {'\0'};
     int read_chars = 0;
     while (client.connected() && millis() - start < 10000L) {
       while (client.available()) {
         logo[read_chars] = client.read();
         logo[read_chars + 1] = '\0';
         read_chars++;
         start = millis();
       }
     }
     Serial.println(logo);
     DBG("#####  RECEIVED:", strlen(logo), "CHARACTERS");
     client.stop();
   }
#endif

#if SIMPLE_NB_TEST_SSL && defined SIMPLE_NB_SUPPORT_SSL
   SimpleNBClientSecure secureClient(modem, 1);
   DBG("Connecting securely to", server);
   if (!secureClient.connect(server, securePort)) {
     DBG("... failed");
   } else {
     // Make a HTTP GET request:
     secureClient.print(String("GET ") + resource + " HTTP/1.1\r\n");
     secureClient.print(String("Host: ") + server + "\r\n");
     secureClient.print("Connection: close\r\n\r\n");

     // Wait for data to arrive
     uint32_t startS = millis();
     while (secureClient.connected() && !secureClient.available() &&
            millis() - startS < 30000L) {
       delay(100);
     };

     // Read data
     startS = millis();
     char response[640] = {'\0'};
     int read_res = 0;
     while (secureClient.connected() && millis() - startS < 10000L) {
       while (secureClient.available()) {
         response[read_res] = secureClient.read();
         response[read_res + 1] = '\0';
         read_res++;
         startS = millis();
       }
     }
     Serial.println(response);
     DBG("#####  RECEIVED:", strlen(response), "CHARACTERS");
     secureClient.stop();
   }
 #endif

 #if SIMPLE_NB_TEST_CALL && defined SIMPLE_NB_SUPPORT_CALLING && defined CALL_TARGET
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
   CellLBS_t lbs;
   for (int8_t i = 15; i; i--) {
     DBG("Requesting current GSM location (with GMT time)");
     if (modem.getGsmLocation(lbs, 0)) {
       DBG("Latitude:", String(lbs.lat, 8), "\tLongitude:", String(lbs.lon, 8));
       DBG("Accuracy:", lbs.accuracy);
       DBG("Year:", lbs.year, "\tMonth:", lbs.month, "\tDay:", lbs.day);
       DBG("Hour:", lbs.hour, "\tMinute:", lbs.minute, "\tSecond:", lbs.second);
       break;
     } else {
       DBG("Couldn't get GSM location, retrying in 15s.");
       delay(15000L);
     }
   }
   DBG("Retrieving GSM location again as a string (without time)");
   String location = modem.getGsmLocation(0);
   DBG("GSM Based Location String:", location);
 #endif

 #if SIMPLE_NB_TEST_GPS && defined SIMPLE_NB_SUPPORT_GPS
   DBG("Enabling GPS/GNSS/GLONASS and waiting 15s for warm-up");
   GPS_t gps;
   modem.enableGPS();
   for (int8_t i = 15; i; i--) {
     DBG("Requesting current GPS/GNSS/GLONASS location");
     if (modem.getGPS(&gps)) {
       DBG("Latitude:", String(gps.lat, 8), "\tLongitude:", String(gps.lon, 8));
       DBG("Speed:", gps.speed, "\tAltitude:", gps.alt);
       DBG("Visible Satellites:", gps.vsat, "\tUsed Satellites:", gps.usat);
       DBG("Accuracy:", gps.accuracy);
       DBG("Year:", gps.year, "\tMonth:", gps.month, "\tDay:", gps.day);
       DBG("Hour:", gps.hour, "\tMinute:", gps.minute, "\tSecond:", gps.second);
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
   modem.NTPServerSync("pool.ntp.org", 32);
 #endif

 #if SIMPLE_NB_TEST_TIME && defined SIMPLE_NB_SUPPORT_TIME
   DateTime_t dt;
   for (int8_t i = 5; i; i--) {
     DBG("Requesting current network time");
     if (modem.getNetworkTime(dt)) {
       DBG("Year:", dt.year, "\tMonth:", dt.month, "\tDay:", dt.day);
       DBG("Hour:", dt.hour, "\tMinute:", dt.minute, "\tSecond:", dt.second);
       DBG("Timezone:", dt.timezone/4.0F);
       break;
     } else {
       DBG("Couldn't get network time, retrying in 15s.");
       delay(15000L);
     }
   }
   DBG("Retrieving time again as a string");
   String time = modem.getNetworkTime();
   DBG("Current Network Time:", time);
 #endif

 #if SIMPLE_NB_TEST_BATTERY && defined SIMPLE_NB_SUPPORT_BATTERY
   Battery_t batt;
   modem.getBatteryStatus(batt);
   DBG("Battery charge state:", batt.chargeState);
   DBG("Battery charge 'percent':", batt.percent);
   DBG("Battery voltage:", batt.milliVolts / 1000.0F);
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

 #if SIMPLE_NB_TEST_DATA
   modem.deactivateDataNetwork();
   delay(2000);
 #endif

   // Try to power-off (modem may decide to restart automatically)
   // To turn off modem completely, please use Reset/Enable pins
   modem.powerOff();
   DBG("Power Off.");
 #endif

   DBG("End of tests.");

  // Do nothing forevermore
  while (true) { modem.maintain(); }
}
