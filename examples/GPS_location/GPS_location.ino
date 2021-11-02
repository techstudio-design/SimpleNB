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
#define SIMPLE_NB_MODEM_SIM7080

// Enbale DBG debug output to Serial Monitor for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

#include <SimpleNBClient.h>

#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem
#define ANT_CTRL  12      // For GPS Antenna Control, make sure it match to the GPIO pin you use

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
    digitalWrite(PWRKEY, HIGH);   // Keep PWRKEY off
    pinMode(PWRKEY, OUTPUT);

   // set ANT_CTRL pin low before configuration
    digitalWrite(ANT_CTRL, LOW);
    pinMode(ANT_CTRL, OUTPUT);
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

    // DBG("Activate Data Network...");
    // if (!modem.activateDataNetwork()) {
    //   delay(1000);
    //   return;   // can't connect to the data network, check your APN and sim setup
    // }

    // Make sure this is test at outdoor where GPS signal can be reached. The time
    // it takes to get the GPS TTFF (Time-to-first-fix) could be any where between
    // 30 - 60 seconds, depends on antenna sensitivity, the visibility of sky, and
    // many other factors, each call to modem.getGPS() will timeout at 2 minutes.

    // Enable GPS receiver and turn on GPS Active-antenna power control
    digitalWrite(ANT_CTRL, HIGH);
    modem.enableGPS();
    delay(1000);

    // Getting GPS data with parsed data in GPS_t object
    GPS_t gps;
    for (int8_t i = 3; i; i--) {
      DBG("Requesting current GPS location and GMT time");
      if (modem.getGPS(gps)) {
        DBG("Latitude:", String(gps.lat, 8), "\tLongitude:", String(gps.lon, 8));
        DBG("Accuracy:", gps.accuracy);
        DBG("Year:", gps.year, "\tMonth:", gps.month, "\tDay:", gps.day);
        DBG("Hour:", gps.hour, "\tMinute:", gps.minute, "\tSecond:", gps.second);
        break;
      } else {
        DBG("Couldn't get GPS location, retrying in 15s.");
        delay(15000L);
      }
    }

    // Getting GPS data as a String
    // String gpsData = modem.getGPS();
    // Serial.println(gpsData);

    // Disable GPS receiver and Turn off GPS antenna control
    modem.disableGPS();
    digitalWrite(ANT_CTRL, LOW);

    DBG("Powering Down...");
    powerDown();
    DBG("Done!\n\n");

    delay(120000);  //wait for 2 minutes
}
