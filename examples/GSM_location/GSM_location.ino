/*****************************************************************
 *
 * This example retrieve latitude and longitude based on GSM LBS
 * (Location Based Service) commands. This functonalities may not
 * available on your NB-IoT/CAT M1 modules, check with module's
 * manual prior proceed. The data network must be activated prior
 * calling the GSM location function.
 *
 * This demo is based on using SIM7080.
 *
 *****************************************************************/

// Select your modem: (See AllFunctions example for the definition of other modem)
// GSM Location is only available on selective modems. See README.md for supported modems
#define SIMPLE_NB_MODEM_SIM7080

// Enbale DBG debug output to Serial Monitor for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

#include <SimpleNBClient.h>

#define PWRKEY   23       // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

SimpleNB modem(SerialAT);

// different modem may have different time requirements, check datasheet of your modem
void powerUp() {
    digitalWrite(PWRKEY, LOW);
    delay(1000);
    digitalWrite(PWRKEY, HIGH);
    delay(2500);
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

    // GSM LBS service is based on the cell-location(s) of your device current location
    // to estimate the location, with a typical accuracy of +/- 500m, depends on Network
    // coverage and number of mobile towers "visible". This functonalities may not
    // available on your NB-IoT/CAT M1 modules, check with module's manual prior proceed.
    // Data Network must be activated prior calling the service.
    CellLBS_t lbs;
    for (int8_t i = 3; i; i--) {
     DBG("Requesting current GSM location with GMT time");
     if (modem.getGsmLocation(lbs, 0)) {
       DBG("Latitude:", String(lbs.lat, 8), "\tLongitude:", String(lbs.lon, 8));
       DBG("Accuracy:", lbs.accuracy);
       DBG("Year:", lbs.year, "\tMonth:", lbs.month, "\tDay:", lbs.day);
       DBG("Hour:", lbs.hour, "\tMinute:", lbs.minute, "\tSecond:", lbs.second);
       break;
     } else {
       DBG("Couldn't get GSM location, retrying in 15s.");
       delay(15000L);
     }
    }

    // there is an alternative API that return the location data (without date)
    // as a String
    // DBG("Retrieving GSM location again as a string without time");
    // String location = modem.getGsmLocation(0);
    // DBG("GSM Based Location String:", location);

    DBG("Powering Off...");
    powerOff();
    DBG("Done!\n\n");

    delay(120000);  //wait for 2 minutes
}
