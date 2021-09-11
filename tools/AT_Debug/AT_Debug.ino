/**************************************************************
 *
 * This script tries to auto-detect the baud rate
 * and allows direct AT commands access
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

// Set serial for debug console (to the Serial Monitor, speed 115200)
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

#define SIMPLE_NB_DEBUG Serial

#include <SimpleNBClient.h>

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(6000);
}

void loop() {

  if (!rate) {
    rate = SimpleNBAutoBaud(SerialAT);
  }

  if (!rate) {
    Serial.println(F("***********************************************************"));
    Serial.println(F(" Module does not respond!"));
    Serial.println(F("   Check your Serial wiring"));
    Serial.println(F("   Check the module is correctly powered and turned on"));
    Serial.println(F("***********************************************************"));
    delay(30000L);
    return;
  }

  SerialAT.begin(rate);

  // Access AT commands from Serial Monitor
  Serial.println(F("***********************************************************"));
  Serial.println(F(" You can now send AT commands"));
  Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
  Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
  Serial.println(F("***********************************************************"));

  while(true) {
    if (SerialAT.available()) {
      Serial.write(SerialAT.read());
    }
    if (Serial.available()) {
      SerialAT.write(Serial.read());
    }
    delay(0);
  }
}
