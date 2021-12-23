/**************************************************************
 *
 * If you can't run this diagnositics sketch successfully, your
 * may need StreamDebugger library for further dianostics:
 *   https://github.com/vshymanskyy/StreamDebugger
 *   or from http://librarymanager/all#StreamDebugger
 *
 * SimpleNB README:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
 *
 **************************************************************/

// Select your modem:
// #define SIMPLE_NB_MODEM_SIM7000SSL
// #define SIMPLE_NB_MODEM_SIM7020
// #define SIMPLE_NB_MODEM_SIM7070
#define SIMPLE_NB_MODEM_SIM7080
// #define SIMPLE_NB_MODEM_SIM7090
// #define SIMPLE_NB_MODEM_UBLOX
// #define SIMPLE_NB_MODEM_SARAR4
// #define SIMPLE_NB_MODEM_BG96
// #define SIMPLE_NB_MODEM_XBEE
// #define SIMPLE_NB_MODEM_SEQUANS_MONARCH

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#ifndef __AVR_ATmega328P__
#define SerialAT Serial2

// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

// Increase RX buffer to capture the entire response
// Chips without internal buffering need enough space in
// the buffer for the entire response else data will be lost
#ifndef SIMPLE_NB_RX_BUFFER
#define SIMPLE_NB_RX_BUFFER 1024
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define ACK_AUTOBAUD_MIN 9600
#define ACK_AUTOBAUD_MAX 115200

// Add a reception delay - may be needed for a fast processor at a slow baud
// rate #define SIMPLE_NB_YIELD() { delay(2); }

// Uncomment this if you want to use SSL
// #define USE_SSL

//#define SIMPLE_NB_USE_GPRS true
#define SIMPLE_NB_USE_DATA true

// set GSM PIN, if any
#define GSM_PIN ""

#define PWRKEY    23      // GPIO pin used for PWRKEY

#include <SimpleNBClient.h>

// #define DUMP_AT_COMMANDS
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger  debugger(SerialAT, Serial);
SimpleNB        modem(debugger);
#else
SimpleNB        modem(SerialAT);
#endif

// Your GPRS credentials, if any
const char apn[]      = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details
const char server[]   = "postman-echo.com";
const char resource[] = "/ip";

#ifdef USE_SSL
SimpleNBClientSecure client(modem);
const int port = 443;
#else
SimpleNBClient client(modem);
const int port = 80;
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

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  digitalWrite(PWRKEY, HIGH);  // Keep PWRKEY off
  pinMode(PWRKEY, OUTPUT);
  // !!!!!!!!!!!

  Serial.println("Powering Up...");
  powerUp();
  delay(6000);
}

void loop() {
  // Set module baud rate
  SimpleNBBegin(SerialAT, 115200);
  // SimpleNBAutoBaud(SerialAT, 9600, 115200);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.print("Initializing modem...");
  // if (!modem.restart()) {
  if (!modem.init()) {
    Serial.println(F(" [fail]"));
    Serial.println(F("************************"));
    Serial.println(F(" Is your modem connected properly?"));
    Serial.println(F(" Is your serial speed (baud rate) correct?"));
    Serial.println(F(" Is your modem powered on?"));
    Serial.println(F(" Do you use the correct Serial port for SerialAT?"));
    Serial.println(F(" Do you use a good, stable power source?"));
    Serial.println(F(" Try using File -> Examples -> SimpeNB -> tools -> AT_Debug to find correct configuration"));
    Serial.println(F("************************"));
    delay(10000);
    return;
  }
  Serial.println(F(" [OK]"));

  String modemInfo = modem.getModemInfo();
  Serial.print(F("Modem Info: "));
  Serial.println(modemInfo);

#if SIMPLE_NB_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
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
    Serial.println(F(" Does the SIM card support NB-IoT/CAT-M1?"));
    Serial.println(F("************************"));
    delay(10000);
    return;
  }
  Serial.println(F(" [OK]"));

#if SIMPLE_NB_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  Serial.print(F("Connecting to "));
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

#if SIMPLE_NB_USE_DATA
  Serial.print("Activate Data Network... ");
  if (!modem.activateDataNetwork()) {
    Serial.println(F(" [fail]"));
    Serial.println(F("************************"));
    Serial.println(F(" Is Data service enabled by network provider?"));
    Serial.println(F(" Check with your mobile carrier."));
    Serial.println(F("************************"));
    delay(1000);
    return;
  }
#endif

  IPAddress local = modem.localIP();
  Serial.print(F("Local IP: "));
  Serial.println(local);

  Serial.print(F("Connecting to "));
  Serial.print(server);
  Serial.print(F(" port "));
  Serial.print(port);
  if (!client.connect(server, port)) {
    Serial.println(F(" [fail]"));
    Serial.println(F("************************"));
    Serial.println(F(" Are the server url and port correct?"));
#ifdef USE_SSL
    Serial.println(F(" Make sure that the root CA cert is setup correctly? (see tools/ca_upload)"));
#endif
    Serial.println(F("************************"));
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
  uint32_t timeout = millis();
  uint32_t bytesReceived = 0;
  char response[30] = {'\0'};
  while (client.connected() && millis() - timeout < 10000L) {
    while (client.available()) {
      response[bytesReceived] = client.read();
      bytesReceived += 1;
      timeout = millis();
    }
  }

  client.stop();
  Serial.println(F("Server connection closed"));

  Serial.print(F("Deactivate Data Network... "));
  if (!modem.deactivateDataNetwork()) {
    Serial.println(F("failed"));
    delay(1000);
    return;
  }
  Serial.println(F("disconnected"));

  Serial.println();
  Serial.println(F("*********************************"));
  Serial.print(F(" Received: "));
  Serial.print(bytesReceived);
  Serial.println(F(" bytes"));
  Serial.print(F(" Response: "));
  Serial.println(response);
  Serial.print(F(" Test:     "));
  Serial.println((bytesReceived == 22) ? "PASSED" : "FAILED");
  Serial.println(F("*********************************"));

  // Do nothing forevermore
  while (true) { delay(1000); }
}
