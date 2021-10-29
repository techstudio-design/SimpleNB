/*****************************************************************
 *
 * This example send a HTTP POST request to httpbin.org/post with
 * a JSON object {"msg":"Hello World"} using a TCP connection. the
 * server will echo back what you sent together with response code,
 * http headers, etc.
 *
 * This demo is based on using SIM7080.
 *
 *****************************************************************/

// Select your modem: (See AllFunctions example for the definition of other modem)
#define SIMPLE_NB_MODEM_SIM7080

// Enbale DBG debug output to Serial Monitor for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

#include <SimpleNBClient.h>

#define PWRKEY   23       // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

// Server configuration
const char host[] = "httpbin.org";
const char resource[] = "/post";
const int port = 80;

SimpleNB modem(SerialAT);

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
    digitalWrite(PWRKEY, HIGH);  // Keep PWRKEY off
    pinMode(PWRKEY, OUTPUT);
}

void loop() {
    DBG("Powering Up...");
    powerUp();

    // Set modem baud rate
    SimpleNBBegin(SerialAT, BAUD_RATE);

    DBG("Initializing modem...");
    modem.init();

    DBG("Waiting for network registration...");
    if (!modem.waitForRegistration(60000L, true)) {
      delay(1000);
      return;  // network timeout, can't register to the mobile network, check your sim or carrier setup
    }

    DBG("Activate Data Network...");
    if (!modem.activateDataNetwork()) {
      delay(1000);
      return;   // can't connect to the data network, check your APN and sim setup
    }

    SimpleNBClient client(modem, 0);

    DBG("Connecting to", host);
    if (!client.connect(host, port)) {
      DBG("... failed");
    } else {
      // Make a HTTP POST request via TCP:
      client.println("POST " + String(resource) + " HTTP/1.1");
      client.println("Host: " + String(host));
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.print("Content-Length: ");
      String payload = "{\"msg\":\"Hello World\"}";
      client.println(payload.length());
      client.println();
      client.println(payload);

      // Wait for response to arrive
      uint32_t start = millis();
      while (client.connected() && !client.available() && millis() - start < 30000L) {
        delay(100);
      };

      // Read response data, the response data consists of headers, payload acho backed so can be quite long
      start = millis();
      char received[680] = {'\0'};  // adjust the length accordingly as per your payload size
      int read_chars = 0;
      while (client.connected() && millis() - start < 10000L) {
        while (client.available()) {
          received[read_chars] = client.read();
          received[read_chars + 1] = '\0';
          read_chars++;
          start = millis();
        }
      }
      DBG(received);
      DBG("Data received:", strlen(received), "characters");

      client.stop();
    }

    DBG("Powering Down...");
    powerDown();
    DBG("Done!\n\n");

    delay(120000);  //wait for 2 minutes
}
