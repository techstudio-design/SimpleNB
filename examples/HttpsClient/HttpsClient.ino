/**************************************************************
 *
 * This sketch connects to a website and downloads a page.
 * It can be used to perform HTTP/RESTful API calls.
 *
 * For this example, you need to install ArduinoHttpClient library:
 *   https://github.com/arduino-libraries/ArduinoHttpClient
 *   or from http://librarymanager/all#ArduinoHttpClient
 *
 * SimpleNB Readme:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
 *
 * SSL/TLS is not yet supported on the Quectel modems
 * The A6/A7/A20 and M590 are not capable of SSL/TLS
 *
 * For more HTTP API examples, see ArduinoHttpClient library
 *
 * NOTE: This example may NOT work with the XBee because the
 * HttpClient library does not empty to serial buffer fast enough
 * and the buffer overflow causes the HttpClient library to stall.
 * Boards with faster processors may work, 8MHz boards will not.
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

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#ifndef __AVR_ATmega328P__
#define SerialAT Serial1

// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#if !defined(SIMPLE_NB_RX_BUFFER)
#define SIMPLE_NB_RX_BUFFER 650
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial
// #define LOGGING  // <- Logging is for the HTTP library

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define SIMPLE_NB_YIELD() { delay(2); }

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define SIMPLE_NB_USE_GPRS true
#define SIMPLE_NB_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// flag to force SSL client authentication, if needed
// #define SIMPLE_NB_SSL_CLIENT_AUTHENTICATION

// Your GPRS credentials, if any
const char apn[]      = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// Server details
const char server[]   = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int  port       = 443;

#include <SimpleNBClient.h>
#include <ArduinoHttpClient.h>

// Just in case someone defined the wrong thing..
#if SIMPLE_NB_USE_GPRS && not defined SIMPLE_NB_SUPPORT_GPRS
#undef SIMPLE_NB_USE_GPRS
#undef SIMPLE_NB_USE_WIFI
#define SIMPLE_NB_USE_GPRS false
#define SIMPLE_NB_USE_WIFI true
#endif
#if SIMPLE_NB_USE_WIFI && not defined SIMPLE_NB_SUPPORT_WIFI
#undef SIMPLE_NB_USE_GPRS
#undef SIMPLE_NB_USE_WIFI
#define SIMPLE_NB_USE_GPRS true
#define SIMPLE_NB_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
SimpleNB        modem(debugger);
#else
SimpleNB        modem(SerialAT);
#endif

SimpleNBClientSecure client(modem);
HttpClient          http(client, server, port);

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  Serial.println("Wait...");

  // Set GSM module baud rate
  SimpleNBBegin(SerialAT, 115200);
  //SimpleNBAutoBaud(SerialAT, 9600, 115200);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);

#if SIMPLE_NB_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif
}

void loop() {
#if SIMPLE_NB_USE_WIFI
  // Wifi connection parameters must be set before waiting for the network
  Serial.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");
#endif

#if SIMPLE_NB_USE_GPRS && defined SIMPLE_NB_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  Serial.print("Waiting for network...");
  if (!modem.waitForRegistration()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isNetworkRegistered()) { Serial.println("Network connected"); }

#if SIMPLE_NB_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected()) { Serial.println("GPRS connected"); }
#endif

  Serial.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive();  // Currently, this is needed for HTTPS
  int err = http.get(resource);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  Serial.print(F("Response status code: "));
  Serial.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  Serial.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    Serial.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    Serial.print(F("Content length is: "));
    Serial.println(length);
  }
  if (http.isResponseChunked()) {
    Serial.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  Serial.println(F("Response:"));
  Serial.println(body);

  Serial.print(F("Body length is: "));
  Serial.println(body.length());

  // Shutdown

  http.stop();
  Serial.println(F("Server disconnected"));

#if SIMPLE_NB_USE_WIFI
  modem.networkDisconnect();
  Serial.println(F("WiFi disconnected"));
#endif
#if SIMPLE_NB_USE_GPRS
  modem.gprsDisconnect();
  Serial.println(F("GPRS disconnected"));
#endif

  // Do nothing forevermore
  while (true) { delay(1000); }
}
