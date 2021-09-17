/**************************************************************
 *
 * To run this tool you need StreamDebugger library:
 *   https://github.com/vshymanskyy/StreamDebugger
 *   or from http://librarymanager/all#StreamDebugger
 *
 * SimpleNB README:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
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
#define Serial Serial

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
#ifndef SIMPLE_NB_RX_BUFFER
#define SIMPLE_NB_RX_BUFFER 1024
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Add a reception delay - may be needed for a fast processor at a slow baud
// rate #define SIMPLE_NB_YIELD() { delay(2); }

// Uncomment this if you want to use SSL
// #define USE_SSL

#define SIMPLE_NB_USE_GPRS true
#define SIMPLE_NB_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// Server details
const char server[]   = "vsh.pp.ua";
const char resource[] = "/SimpeNB/logo.txt";

#include <SimpleNBClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
SimpleNB        modem(debugger);
#else
SimpleNB        modem(SerialAT);
#endif

#ifdef USE_SSL&& defined SIMPLE_NB_SUPPORT_SSL
SimpleNBClientSecure      client(modem);
const int                port = 443;
#else
SimpleNBClient  client(modem);
const int      port = 80;
#endif

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!

  Serial.println("Wait...");

  // Set GSM module baud rate
  SerialAT.begin(115200);
  SimpleNBBegin(SerialAT, 115200);
  // SimpleNBAutoBaud(SerialAT, 9600, 115200);
  delay(6000);
}

void loop() {
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.print("Initializing modem...");
  if (!modem.restart()) {
    // if (!modem.init()) {
    Serial.println(F(" [fail]"));
    Serial.println(F("************************"));
    Serial.println(F(" Is your modem connected properly?"));
    Serial.println(F(" Is your serial speed (baud rate) correct?"));
    Serial.println(F(" Is your modem powered on?"));
    Serial.println(F(" Do you use a good, stable power source?"));
    Serial.println(F(" Try using File -> Examples -> SimpeNB -> tools -> AT_Debug to find correct configuration"));
    Serial.println(F("************************"));
    delay(10000);
    return;
  }
  Serial.println(F(" [OK]"));

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);

#if SIMPLE_NB_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif

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
  if (!modem.waitForRegistration(600000L)) {
    Serial.println(F(" [fail]"));
    Serial.println(F("************************"));
    Serial.println(F(" Is your sim card locked?"));
    Serial.println(F(" Do you have a good signal?"));
    Serial.println(F(" Is antenna attached?"));
    Serial.println(F(" Does the SIM card work with your phone?"));
    Serial.println(F("************************"));
    delay(10000);
    return;
  }
  Serial.println(F(" [OK]"));

#if SIMPLE_NB_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(F(" [fail]"));
    Serial.println(F("************************"));
    Serial.println(F(" Is GPRS enabled by network provider?"));
    Serial.println(F(" Try checking your card balance."));
    Serial.println(F("************************"));
    delay(10000);
    return;
  }
  Serial.println(F(" [OK]"));
#endif

  IPAddress local = modem.localIP();
  Serial.print("Local IP: ");
  Serial.println(local);

  Serial.print(F("Connecting to "));
  Serial.print(server);
  if (!client.connect(server, port)) {
    Serial.println(F(" [fail]"));
    delay(10000);
    return;
  }
  Serial.println(F(" [OK]"));

  // Make a HTTP GET request:
  client.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  // Wait for data to arrive
  while (client.connected() && !client.available()) {
    delay(100);
    Serial.print('.');
  };
  Serial.println();

  // Skip all headers
  client.find("\r\n\r\n");

  // Read data
  uint32_t timeout       = millis();
  uint32_t bytesReceived = 0;
  while (client.connected() && millis() - timeout < 10000L) {
    while (client.available()) {
      char c = client.read();
      // Serial.print(c);
      bytesReceived += 1;
      timeout = millis();
    }
  }

  client.stop();
  Serial.println(F("Server disconnected"));

#if SIMPLE_NB_USE_WIFI
  modem.networkDisconnect();
  Serial.println(F("WiFi disconnected"));
#endif
#if SIMPLE_NB_USE_GPRS
  modem.gprsDisconnect();
  Serial.println(F("GPRS disconnected"));
#endif

  Serial.println();
  Serial.println(F("************************"));
  Serial.print(F(" Received: "));
  Serial.print(bytesReceived);
  Serial.println(F(" bytes"));
  Serial.print(F(" Test:     "));
  Serial.println((bytesReceived == 121) ? "PASSED" : "FAILED");
  Serial.println(F("************************"));

  // Do nothing forevermore
  while (true) { delay(1000); }
}
