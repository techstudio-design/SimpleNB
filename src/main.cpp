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
 * This example connects to HiveMQ's showcase broker.
 *
 * Test sending and receiving messages from the HiveMQ webclient
 * available at http://www.hivemq.com/demos/websocket-client/.
 *
 * Publish 0 or 1 to the topic SimpleNB/led to toggle on-board LED,
 * Subscribe to the topic SimpleNB/status to receive LED status
 * send back by the mqtt client.
 *
 **************************************************************/

// Select your modem: (See AllFunctions example for the definition of other modem)
#define SIMPLE_NB_MODEM_SIM7080

// Enbale Serial.print debug output to Serial Monitor for debug prints, if needed
//#define SIMPLE_NB_DEBUG Serial

// Select your Serial port for AT interface (See AllFunctions example for other port settings)
#define SerialAT Serial2

#include <SimpleNBClient.h>
#include <PubSubClient.h>

#define LED_PIN   2
#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

// MQTT details
const char* broker      = "broker.hivemq.com";
const char* topicLed    = "SimpleNB/led";
const char* topicStatus = "SimpleNB/status";

int ledStatus = LOW;
uint32_t lastReconnectAttempt = 0;

SimpleNB       modem(SerialAT);
SimpleNBClient client(modem);
PubSubClient   mqtt(client);

void mqttCallback(char* topic, byte* payload, unsigned int len) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < len; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    // Only proceed if incoming message's topic matches
    if (String(topic) == topicLed) {
      ledStatus = !ledStatus;
      digitalWrite(LED_PIN, ledStatus);
      mqtt.publish(topicStatus, ledStatus ? "1" : "0");
    }
}

bool mqttConnect() {
    Serial.print("Connecting to ");
    Serial.print(broker);

    // Connect to MQTT Broker
    bool status = mqtt.connect("SimpleNBClient");
    // bool status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");

    if (status == false) {
      Serial.println(" fail");
      return false;
    }
    Serial.println(" success");

    const char* initMsg = "MQTT Started";
    mqtt.publish(topicStatus, initMsg);
    Serial.print("Published topic [");
    Serial.print(topicStatus);
    Serial.print("]: ");
    Serial.println(initMsg);

    mqtt.subscribe(topicLed);
    Serial.print("Subscribed to topic: ");
    Serial.println(topicLed);

    return mqtt.connected();
}

// different modem may have different time requirements, check datasheet of your modem
void powerUp() {
    digitalWrite(PWRKEY, LOW);
    delay(1000);
    digitalWrite(PWRKEY, HIGH);
    delay(2500);
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

    Serial.print("Network registration ");
    if (!modem.waitForRegistration()) {
      Serial.println("fail");
      delay(1000);
      return;
    }
    Serial.println("success");

    Serial.println("Activate Data Network...");
    modem.deactivateDataNetwork();    // disconnect any broken connection if there is one
    if (!modem.activateDataNetwork()) {
      delay(1000);
      return;
    }

    // MQTT Broker setup
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);
    mqtt.setKeepAlive(60);
}

void loop() {

  if (!mqtt.connected()) {
    mqttConnect();
  }

  mqtt.loop();
}
