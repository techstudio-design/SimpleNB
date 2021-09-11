/**************************************************************
 *
 * This sketch connects to a website and downloads a page.
 * It can be used to perform HTTP/RESTful API calls.
 *
 * SimpleNB README:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
 *
 **************************************************************/

// Hologram Dash uses UBLOX U2 modems
#define SIMPLE_NB_MODEM_UBLOX

// Increase RX buffer if needed
#if !defined(SIMPLE_NB_RX_BUFFER)
#define SIMPLE_NB_RX_BUFFER 512
#endif

#include <SimpleNBClient.h>

// Uncomment this if you want to see all AT commands
// #define DUMP_AT_COMMANDS

// Uncomment this if you want to use SSL
// #define USE_SSL

// We'll be using SerialSystem in Passthrough mode
#define SerialAT SerialSystem

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
  SimpleNB mdm(debugger);
#else
  SimpleNB mdm(SerialAT);
#endif

#ifdef USE_SSL
  SimpleNBClientSecure client(mdm);
  const int  port = 443;
#else
  SimpleNBClient client(mdm);
  const int  port = 80;
#endif

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set up Passthrough
  HologramCloud.enterPassthrough();
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  // mdm.restart();
  mdm.init();
  Serial.println(F("Initializing modem..."));


  String modemInfo = mdm.getModemInfo();
  Serial.print(F("Modem: "));
  Serial.println(modemInfo);

  // Unlock your SIM card with a PIN
  //mdm.simUnlock("1234");
}

void loop() {
  Serial.print(F("Waiting for network..."));
  if (!mdm.waitForNetwork()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!mdm.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  Serial.print(F("Connecting to "));
  Serial.print(server);
  if (!client.connect(server, port)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" success");

  // Make a HTTP GET request:
  client.print(String("GET ") + resource + " HTTP/1.0\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n\r\n");

  uint32_t timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      timeout = millis();
    }
  }
  Serial.println();

  // Shutdown

  client.stop();
  Serial.println(F("Server disconnected"));

  mdm.gprsDisconnect();
  Serial.println(F("GPRS disconnected"));

  // Do nothing forevermore
  while (true) {
    delay(1000);
  }
}
