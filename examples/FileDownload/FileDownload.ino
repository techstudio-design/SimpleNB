/**************************************************************
 *
 * For this example, you need to install CRC32 library:
 *   https://github.com/bakercp/CRC32
 *   or from http://librarymanager/all#CRC32+checksum
 *
 * ATTENTION! Downloading big files requires of knowledge of
 * the SimpleNB internals and some modem specifics,
 * so this is for more experienced developers.
 *
 **************************************************************/
// Select your modem: (See AllFunctions example for the definition of other modem)
#define SIMPLE_NB_MODEM_SIM7080

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

#include <SimpleNBClient.h>
#include <CRC32.h>

#define PWRKEY   23       // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

// Increase RX buffer to capture the entire response. Some chips without internal
// buffering need enough space in the buffer for the entire response, else data
// will be lost (and the http library will fail.
#if !defined(SIMPLE_NB_RX_BUFFER)
#define SIMPLE_NB_RX_BUFFER 1024
#endif

// Server details
const char server[]    = "vsh.pp.ua";
const int port         = 80;
const char resource[]  = "/TinyGSM/test_1k.bin";
uint32_t knownCRC32    = 0x6f50d767;
uint32_t knownFileSize = 1024;  // In case server does not send it


SimpleNB modem(SerialAT);

// different modem may have different time requirements, check datasheet of your modem
void powerUp() {
    digitalWrite(PWRKEY, LOW);
    delay(1000);
    digitalWrite(PWRKEY, HIGH);
    delay(2500);
}

void printPercent(uint32_t readLength, uint32_t contentLength) {
    // If we know the total length
    if (contentLength != (uint32_t)-1) {
      Serial.print("\r ");
      Serial.print((100.0 * readLength) / contentLength);
      Serial.print('%');
    } else {
      Serial.println(readLength);
    }
}

void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println();

    pinMode(PWRKEY, OUTPUT);
    Serial.println("Powering Up...");
    powerUp();

    SimpleNBBegin(SerialAT, BAUD_RATE);
    modem.init();

    Serial.println("Waiting for network registration...");
    if (!modem.waitForRegistration(60000L, true)) {
      // can't register to the mobile network, check your sim or carrier setup
      delay(1000);
      return;
    }

    Serial.println("Activate Data Network...");
    if (!modem.activateDataNetwork()) {
      // can't connect to the data network, check your APN and sim setup
      delay(1000);
      return;
    }

    SimpleNBClient client(modem);

    Serial.print(F("Connecting to "));
    Serial.print(server);
    if (!client.connect(server, port)) {
      Serial.println(" fail");
      delay(10000);
      return;
    }
    Serial.println(" connected");

    // Make a HTTP GET request:
    client.println(String("GET ") + resource + " HTTP/1.1");
    client.println(String("Host: ") + server);
    client.println("Connection: close\r\n");

    // Let's see what the entire elapsed time is, from after we send the request.
    uint32_t timeElapsed = millis();

    Serial.println(F("Waiting for response header"));

    // While we are still looking for the end of the header (i.e. empty line
    // FOLLOWED by a newline), continue to read data into the buffer, parsing each
    // line (data FOLLOWED by a newline). Time out if it takes too long.

    const uint32_t timeout   = 5000;
    uint32_t       clientReadStartTime = millis();
    String         headerBuffer;
    bool           finishedHeader = false;
    uint32_t       contentLength  = 0;

    // loop through the incoming data until the end of http headers
    while (!finishedHeader) {
      int nlPos;

      if (client.available()) {
        clientReadStartTime = millis();
        while (client.available()) {
          char c = client.read();
          headerBuffer += c;

          // Uncomment the lines below to see the data coming into the buffer
          // Serial.print((char) c);

          if (headerBuffer.indexOf(F("\r\n")) >= 0) break;
        }
      } else {
        if (millis() - clientReadStartTime > timeout) {
          // Time-out waiting for data from client
          Serial.println(F(">>> Client Timeout !"));
          break;
        }
      }

      // See if we have a new line.
      nlPos = headerBuffer.indexOf(F("\r\n"));

      // if "\r\n" is at the beginning of the line, it is the beginning of payload
      if (nlPos > 0) {
        // Look for the line contains Content-Length
        if (headerBuffer.startsWith(F("Content-Length:"))) {
          contentLength = headerBuffer.substring(headerBuffer.indexOf(':') + 1).toInt();
        }

        headerBuffer.remove(0, nlPos + 2);  // remove the line
      } else if (nlPos == 0) {
        finishedHeader = true;
      }
    }

    // The two cases which are not managed properly are as follows:
    // 1. The client doesn't provide data quickly enough to keep up with this loop.
    // 2. If the client data is segmented in the middle of the 'Content-Length:' header,
    //    then that header may be missed/damaged.

    uint32_t readLength = 0;
    CRC32    crc;

    if (finishedHeader && contentLength == knownFileSize) {
      Serial.println(F("Reading response data"));
      clientReadStartTime = millis();

      printPercent(readLength, contentLength);
      while (readLength < contentLength && client.connected() &&
             millis() - clientReadStartTime < timeout) {
        while (client.available()) {
          uint8_t c = client.read();
          // Serial.print(reinterpret_cast<char>c);  // Uncomment this to show
          // data
          crc.update(c);
          readLength++;
          if (readLength % (contentLength / 13) == 0) {
            printPercent(readLength, contentLength);
          }
          clientReadStartTime = millis();
        }
      }
      printPercent(readLength, contentLength);
    }

    timeElapsed = millis() - timeElapsed;
    Serial.println();

    // Shutdown
    client.stop();
    Serial.println(F("Server disconnected"));
    modem.deactivateDataNetwork();
    Serial.println(F("Data Network deactivated"));

    float duration = float(timeElapsed) / 1000;

    Serial.println();
    Serial.print("Content-Length: ");
    Serial.println(contentLength);
    Serial.print("Actually read:  ");
    Serial.println(readLength);
    Serial.print("Calc. CRC32:    0x");
    Serial.println(crc.finalize(), HEX);
    Serial.print("Known CRC32:    0x");
    Serial.println(knownCRC32, HEX);
    Serial.print("Duration:       ");
    Serial.print(duration);
    Serial.println("s");

}

void loop() {

}
