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
// #define SIMPLE_NB_MODEM_SIM7020
// #define SIMPLE_NB_MODEM_SIM7070
// #define SIMPLE_NB_MODEM_SIM7080
// #define SIMPLE_NB_MODEM_SIM7090
// #define SIMPLE_NB_MODEM_UBLOX
// #define SIMPLE_NB_MODEM_SARAR4
// #define SIMPLE_NB_MODEM_BG96
// #define SIMPLE_NB_MODEM_XBEE
// #define SIMPLE_NB_MODEM_SEQUANS_MONARCH

#include <SimpleNBClient.h>

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#ifndef __AVR_ATmega328P__
#define SerialAT Serial1

// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3);  // RX, TX
#endif

#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
SimpleNB modem(debugger);

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(9600);
  delay(6000);

  if (!modem.init()) {
    Serial.println(F("***********************************************************"));
    Serial.println(F(" Cannot initialize modem!"));
    Serial.println(F("   Use File -> Examples -> SimpeNB -> tools -> AT_Debug"));
    Serial.println(F("   to find correct configuration"));
    Serial.println(F("***********************************************************"));
    return;
  }

  bool ret = modem.factoryDefault();

  Serial.println(F("***********************************************************"));
  Serial.print  (F(" Return settings to Factory Defaults: "));
  Serial.println((ret) ? "OK" : "FAIL");
  Serial.println(F("***********************************************************"));
}

void loop() {

}
