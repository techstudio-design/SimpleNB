/**************************************************************
 *
 * This script just listens in on the communication
 * between an Arduino and the modem.
 *
 * SimpleNB README:
 *   https://github.com/techstudio-design/SimpleNB/blob/master/README.md
 *
 **************************************************************/

// Set the baud rate between the modem and the board
#define BAUD_RATE 9600

// Set serial for input from modem
#define MODEM_TX Serial1

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
// #define BOARD_TX Serial1

// or AltSoftware
#include <AltSoftSerial.h>
AltSoftSerial BOARD_TX;


void setup() {
  // Set console baud rate
  Serial.begin(115200);

  MODEM_TX.begin(BAUD_RATE);
  BOARD_TX.begin(BAUD_RATE);
  delay(6000);
}

void loop()
{
  while (MODEM_TX.available()) {
    Serial.write(MODEM_TX.read());
  }
  while (BOARD_TX.available()) {
    Serial.write(BOARD_TX.read());
  }
}
