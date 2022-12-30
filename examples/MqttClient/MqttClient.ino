/**************************************************************
 *
 * For this example, you need to install PubSubClient library:
 *   https://github.com/knolleary/pubsubclient, or from
 *   http://librarymanager/all#PubSubClient
 *
 * This example is mainly to show the pre-requisition that is
 * needed for establishing the NB-IoT connect in the setup().
 * For the rest of the part, it is from PubSubClient. For more
 * MQTT examples, see PubSubClient library.
 *
 **************************************************************
 * This example connects to test.mosquitto.org broker either via
 * port 1883 (non-secure) or port 8883 (secure) by comment in/out
 * the USING_SECURE_SSL macro defined below.
 *
 * Publish anything to the topic SimpleNB/led will toggle the
 * on-board LED,
 * Subscribe to the topic SimpleNB/status to receive LED status
 * send back by the mqtt client after the toggling.
 *
 **************************************************************/

// Select your modem: (See AllFunctions example for the definition of other modem)
#define SIMPLE_NB_MODEM_SIM7080

// Enbale Serial.print debug output to Serial Monitor for debug prints, if needed
//#define SIMPLE_NB_DEBUG Serial

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

// USING_SECURE_SSL for secure connection via port 8883, comment out if using non-secure port 1883
#define USING_SECURE_MQTT

#include <SimpleNBClient.h>
#include <PubSubClient.h>

#define LED_PIN   2
#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

#define PUBLISH_INTERVAL 30000  // publish data every 30s
#define KEEP_ALIVE       60     // MQTT KeepAlive value (default=15s), set it to twice of PUBLISH_INTERVAL

// MQTT details
const char* broker      = "test.mosquitto.org";
const char* topicLed    = "SimpleNB/led";
const char* topicStatus = "SimpleNB/status";

SimpleNB modem(SerialAT);

#ifndef USING_SECURE_MQTT
const int port = 1883;
SimpleNBClient client(modem);
#else
const int port = 8883;
SimpleNBClientSecure client(modem);
// The CA is downloaded from https://test.mosquitto.org web page
static const char mqtt_ca[] PROGMEM = "-----BEGIN CERTIFICATE-----\n" \
"MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n" \
"BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\n" \
"A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\n" \
"BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\n" \
"by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\n" \
"BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\n" \
"MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\n" \
"dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\n" \
"KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\n" \
"UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\n" \
"Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\n" \
"s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\n" \
"3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\n" \
"E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\n" \
"MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\n" \
"6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n" \
"BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\n" \
"6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\n" \
"+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\n" \
"sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\n" \
"LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\n" \
"m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n" \
"-----END CERTIFICATE-----\n";
#endif

PubSubClient mqtt(client);

int count = 0;
unsigned long mqttStart = 0;


void mqttCallback(char* topic, byte* payload, unsigned int len) {

    char payloadStr[len+1] = {0};
    strncpy(payloadStr, (char*) payload, len);
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(payloadStr);

    // Only proceed if incoming message's topic matches
    if (String(topic) == topicLed) {
      if (payloadStr[0] == '0')
        digitalWrite(LED_PIN, LOW);
      else
        digitalWrite(LED_PIN, HIGH);
      // optional send back the status
      // mqtt.publish(topicStatus, digitalRead(LED_PIN) ? "LED is ON" : "LED is OFF");
    }

}

void mqttConnect() {
    while (!client.connected()) {
      Serial.print("Connecting to ");
      Serial.print(broker);
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      if (mqtt.connect(clientId.c_str())) {
      // if (mqtt.connect(clientId.c_str(), "mqtt_user", "mqtt_pass")) {
        Serial.println(" connected");

        const char* initMsg = "MQTT Started";
        mqtt.publish(topicStatus, initMsg);
        Serial.print("Publish topic [");
        Serial.print(topicStatus);
        Serial.print("]: ");
        Serial.println(initMsg);

        mqtt.subscribe(topicLed);
        Serial.print("Subscribe to topic [");
        Serial.print(topicLed);
        Serial.println("]");
      } else {
        Serial.print(" fail, rc=");
        Serial.println(mqtt.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
      }
    }
}

// different modem may have different time requirements, check datasheet of your modem
void powerUp() {
    digitalWrite(PWRKEY, LOW);
    delay(1000);
    digitalWrite(PWRKEY, HIGH);
    delay(2500);
}

void modemConnect() {
    Serial.print("Network registration... ");
    if (!modem.waitForRegistration()) {
      Serial.println("fail");
      delay(1000);
      return;
    }
    Serial.println("success");
}

void activateDataConnection() {
    Serial.print("Activate Data Network... ");
    modem.deactivateDataNetwork();
    if (!modem.activateDataNetwork()) {
      Serial.println("fail");
      delay(1000);
      return;
    }
    Serial.println("success");
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    pinMode(LED_PIN, OUTPUT);

    Serial.println("Powering Up...");
    pinMode(PWRKEY, OUTPUT);
    powerUp();

    Serial.println("Initializing modem...");
    SimpleNBBegin(SerialAT, BAUD_RATE);
    modem.init();

    modemConnect();

    randomSeed(micros());

    // MQTT Broker setup
    mqtt.setServer(broker, port);
    mqtt.setCallback(mqttCallback);
    mqtt.setKeepAlive(KEEP_ALIVE);

    mqttStart = millis();
}

void loop() {
    if ( !(client.connected() || !modem.isNetworkRegistered()) ) {
      modemConnect();
    }

    if (!mqtt.connected() || !modem.isDataActive()) {
        activateDataConnection();
        mqttConnect();
    }

    mqtt.loop();

    // Sending a message "Hello World #xxxx" every 30 seconds
    if (millis() - mqttStart > PUBLISH_INTERVAL) {
        mqttStart = millis();
        String outgoingMsg = "Hello World #" + String(count++);
        mqtt.publish(topicStatus, outgoingMsg.c_str());
        Serial.print("MQTT Sent: ");
        Serial.println(outgoingMsg);
    }

    yield();
}
