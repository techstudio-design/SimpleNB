/**************************************************************
 *
 * This sketch connects to a website and downloads a page.
 * It can be used to perform HTTP/RESTful API calls.
 *
 * For this example, you need to install ArduinoHttpClient library:
 *   https://github.com/arduino-libraries/ArduinoHttpClient
 *   or from http://librarymanager/all#ArduinoHttpClient
 *
 * SimpleNB README:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
 *
 * For more HTTP API examples, see ArduinoHttpClient library
 *
 **************************************************************/

// Industruino uses SIM800H
#define SIMPLE_NB_MODEM_SIM800

// Increase RX buffer if needed
#if !defined(SIMPLE_NB_RX_BUFFER)
#define SIMPLE_NB_RX_BUFFER 512
#endif

#include <SimpleNBClient.h>
#include <ArduinoHttpClient.h>

// Uncomment this if you want to see all AT commands
// #define DUMP_AT_COMMANDS

// Uncomment this if you want to use SSL
// #define USE_SSL

// Set serial for debug console (to the Serial Monitor, speed 115200)
#define Serial SerialUSB

// Select Serial1 or Serial depending on your module configuration
#define SerialAT Serial1

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "YourAPN";
const char user[] = "";
const char pass[] = "";

// Server details
const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  SimpleNB modem(debugger);
#else
  SimpleNB modem(SerialAT);
#endif

#ifdef USE_SSL
  SimpleNBClientSecure client(modem);
  HttpClient http(client, server, 443);
#else
  SimpleNBClient client(modem);
  HttpClient http(client, server, 80);
#endif

void setup() {
  // Turn on modem with 1 second pulse on D6
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  delay(1000);
  digitalWrite(6, LOW);

  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(115200);
  SimpleNBBegin(SerialAT, 115200);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println(F("Initializing modem..."));
  modem.restart();

  String modemInfo = modem.getModemInfo();
  Serial.print(F("Modem: "));
  Serial.println(modemInfo);

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");
}

void loop() {
  Serial.print(F("Waiting for network..."));
  if (!modem.waitForRegistration()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  Serial.print(F("Performing HTTP GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    Serial.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  Serial.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    //Serial.println(headerName + " : " + headerValue);
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

  modem.gprsDisconnect();
  Serial.println(F("GPRS disconnected"));

  // Do nothing forevermore
  while (true) {
    delay(1000);
  }
}
