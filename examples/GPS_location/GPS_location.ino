/*********************************************************************
 *
 * This example retrieve latitude and longitude from the GPS receiver
 * This functonalities may not available on your NB-IoT/CAT M1 modules,
 * check with module's manual prior proceed.
 *
 * Make sure this is test at outdoor where GPS signal can be reached.
 * The time it takes to get the GPS TTFF (Time-to-first-fix) could be
 * any where between 30 - 60 seconds, depends on antenna sensitivity,
 * the visibility of sky, and many other factors, each call to
 * modem.getGPS() will timeout at 2 minutes.
 *
 * This demo is based on using SIM7080.
 *
 ********************************************************************/

// Select your modem:
// GPS functionality is not available on SIM7020 and BG96
#define SIMPLE_NB_MODEM_SIM7080

// Enbale DBG debug output to Serial Monitor for debug prints, if needed
#define SIMPLE_NB_DEBUG Serial

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

#include <SimpleNBClient.h>

#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem
#define ANT_CTRL  12      // For GPS Antenna Control, make sure it match to the GPIO pin you use

const unsigned long _startTime = millis();

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

    DBG("Registrating to mobile network...");
    if (!modem.waitForRegistration(60000L, true)) {
      delay(1000);
      return;
    }

    DBG("Enabling GSP Reciver...");
    digitalWrite(ANT_CTRL, HIGH);
    modem.enableGPS();
    delay(1000);

    // GPS won't accept any AT command before 9 seconds from boot-up
    delay(3000);

    // Getting GPS data as a String
    // DBG("Requesting current GPS location and GMT time");
    // String gpsData = modem.getGPS();
    // if (gpsData != "") {
    //   DBG(gpsData);
    // } else {
    //   Serial.println("Couldn't get GPS location, retrying in 5s.");
    //   delay(5000L);
    // }

    // Getting GPS data with parsed data in GPS_t object
    GPS_t gps;
    for (int8_t i = 0; i < 3; i++) {
      DBG("Requesting current GPS location and GMT time");
      if (modem.getGPS(gps)) {
        DBG("Latitude:", String(gps.lat, 8), "\tLongitude:", String(gps.lon, 8));
        DBG("Accuracy:", gps.accuracy);
        DBG("Year:", gps.year, "\tMonth:", gps.month, "\tDay:", gps.day);
        DBG("Hour:", gps.hour, "\tMinute:", gps.minute, "\tSecond:", gps.second);
        break;
      } else {
        DBG("Couldn't get GPS location, retrying in 5s.");
        delay(5000L);
      }
    }

    DBG("Disable GPS Receiver");
    modem.disableGPS();
    digitalWrite(ANT_CTRL, LOW);

    DBG("Powering Down...");
    powerDown();
    DBG("Done!\n\n");

    delay(120000);  //wait for 2 minutes
}
