/*****************************************************************
 *
 * This example send a HTTP GET request to postman-echo.com, it
 * return the public IP address of the NB-IoT module.
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
//#define USE_SSL           // uncomment this line if using SSL secure connection
#if USE_SSL && defined SIMPLE_NB_SUPPORT_SSL
const int port = 443;
#else
const int port = 80;
#endif

// Server configuration
const char host[] = "postman-echo.com";
const char resource[] = "/ip";

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

#if USE_SSL && defined SIMPLE_NB_SUPPORT_SSL
    SimpleNBClientSecure client(modem, 0);
#else
    SimpleNBClient client(modem,0);
#endif

    DBG("Connecting to", host, "port", port);
    if (!client.connect(host, port)) {
      DBG("... failed");
    } else {
      // Make a HTTP POST request via TCP:
      client.println("GET " + String(resource) + " HTTP/1.1");
      client.println("Host: " + String(host));
      client.println("Connection: close");
      client.println();


      // Wait for response to arrive
      uint32_t start = millis();
      while (client.connected() && !client.available() && millis() - start < 30000L) {
        delay(100);
      };

      // Read and print the response data to Serial Monitor
      char buff[400] = {'\0'};
      int i = 0;
      while (client.available()) {
        buff[i++] = (char)client.read();
      }
      // Serial.println(buff);  //uncomment this out to see the entire response data

      // return response is in a JSON format {"ip":"xxx.xxx.xxx.xxx"}
      // parse the data to get the IP
      strtok(buff, "{");
      strtok(NULL, ":");
      char * p = strtok(NULL, "\"");
      DBG("Your IP address is: " + String(strtok(p, "\"")) );

      client.stop();
    }

    DBG("Deactivate Data Network... ");
    if (!modem.deactivateDataNetwork()) {
      delay(1000);
      return;
    }

    DBG("Powering Down...");
    // turn off module with AT command
    // modem.powerOff();
    // alternatively, you could implmenent your power down function by triggering PWRKEY
    powerDown();
    DBG("Done!\n\n");

    delay(120000);  //wait for 2 minutes
}
