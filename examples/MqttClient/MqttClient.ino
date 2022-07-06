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

#include <SimpleNBClient.h>
#include <PubSubClient.h>

#define LED_PIN   2
#define PWRKEY    23      // GPIO pin used for PWRKEY
#define BAUD_RATE 115200  // Baud rate to be used for communicating with the modem

// MQTT details
const char* broker      = "broker.hivemq.com";
const char* topicLed    = "SimpleNB/led";
const char* topicStatus = "SimpleNB/status";

int count = 0;
unsigned long mqttStart = 0;

SimpleNB       modem(SerialAT);
SimpleNBClient client(modem);
PubSubClient   mqtt(client);

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
      if (mqtt.connect("SimpleNBClient")) {
      // if (mqtt.connect("SimpleNBClient", "mqtt_user", "mqtt_pass")) {
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

    Serial.print("Activate Data Network... ");
    modem.deactivateDataNetwork();    // disconnect any broken connection if there is one
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

    // MQTT Broker setup
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);
    mqtt.setKeepAlive(60);

    mqttStart = millis();
}

void loop() {
    if ( !(client.connected() || !modem.isNetworkRegistered()) ) {
      modemConnect();
    }

    if (!mqtt.connected()) {
      mqttConnect();
    }

    mqtt.loop();

    // Sending a message "Hello World #xxxx" every 30 seconds
    if (millis() - mqttStart > 30000) {
        mqttStart = millis();
        String outgoingMsg = "Hello World #" + String(count++);
        mqtt.publish(topicStatus, outgoingMsg.c_str());
        Serial.print("MQTT Sent: ");
        Serial.println(outgoingMsg);
    }

    yield();
}
