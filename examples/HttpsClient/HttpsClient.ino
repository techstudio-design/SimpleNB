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
 *
 * For more HTTP API examples, see ArduinoHttpClient library
 *
 * NOTE: This example may NOT work with the XBee because the
 * HttpClient library does not empty to serial buffer fast enough
 * and the buffer overflow causes the HttpClient library to stall.
 * Boards with faster processors may work, 8MHz boards will not.
 **************************************************************/

// Select your modem:
// SSL/TLS is not yet implemented on the Quectel BG96 and SIM7020
// #define SIMPLE_NB_MODEM_SIM7000
// #define SIMPLE_NB_MODEM_SIM7000SSL
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
#define SerialAT Serial1

// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

// Increase RX buffer to capture the entire response
// else data will be lost (and the http library will fail).
#if !defined(SIMPLE_NB_RX_BUFFER)
#define SIMPLE_NB_RX_BUFFER 650
#endif

// Enbale DBG debug output to Serial Monitor for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial

#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

// Server details
const char server[]   = "httpbin.org";
const char resource[] = "/post";
const int  port       = 443;

#include <SimpleNBClient.h>
#include <ArduinoHttpClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
SimpleNB        modem(debugger);
#else
SimpleNB        modem(SerialAT);
#endif

SimpleNBClientSecure client(modem);
HttpClient http(client, server, port);

// different modem may have different time requirements, check datasheet of your modem
void powerUp() {
    digitalWrite(PWRKEY, LOW);
    delay(1000);
    digitalWrite(PWRKEY, HIGH);
    delay(2500);
}

void powerDown() {
    digitalWrite(PWRKEY, LOW);
    delay(1200);
    digitalWrite(PWRKEY, HIGH);
}

void setup() {
  Serial.begin(115200);
  digitalWrite(PWRKEY, HIGH);   // Keep PWRKEY off
  pinMode(PWRKEY, OUTPUT);
}

void loop() {

  SimpleNBBegin(SerialAT, 115200);

  Serial.println("Initializing modem...");
  modem.init();

  Serial.print("Waiting for network registration... ");
  if (!modem.waitForRegistration()) {
    Serial.println("fail");
    delay(10000);
    return;
  }
  Serial.println("registered");

  DBG("Activate Data Network...");
  if (!modem.activateDataNetwork()) {
    delay(1000);
    return;
  }

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

  delay(120000);
}
