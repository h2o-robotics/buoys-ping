/*
  SLAVE BUOY #2
  Receive the ping request from the master buoy, ping the desired
  diver and send the response to the master over MQTT.
*/

// Include libraries
#include <SPI.h>              
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"

// Initialize variables
const long frequency = 866E6;               // LoRa Frequency 
const int csPin = 5;                        // LoRa radio chip select
const int resetPin = 4;                     // LoRa radio reset
const int irqPin = 27;                      // change for your board; must be a hardware interrupt pin

const char* ssid = "ESP32-Access-Point";    // SSID & password for Wifi
const char* password = "123456789";

const char *mqtt_broker = "192.168.4.2";    // MQTT Broker info
const char *mqtt_username = "slave2";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;                       // creation of a client for the broker
PubSubClient client(espClient);

createBufferedOutput(output, 255, DROP_UNTIL_EMPTY);  // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial


// Initialization of LoRa
void LoRaInit(){
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {           
    Serial.println("LoRa init failed. Check your connections.");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("LoRa init succeeded.\n");
}

// Connect the board to a Wifi Access Point
void setup_wifi(){
  delay(10);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
      delay(1000);
  }
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

// Check the connection to MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("esp32/pinger/request/S2");   // subscribe to receive ping request
    }
    
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Receive a message
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  // Read the received message
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Print the message on Serial2
  Serial2.println(messageTemp);
}

// Send a message
void publishMQTT(SafeString& sfReader){
  // Convert sfReader which holds the ping response into a char*
  const char* one = "slave2 ";
  const char* two = sfReader.c_str();

  // Concatenate the 2 char
  char buf[100];
  strcpy(buf, one);
  strcat(buf, two);

  // Publish the ping response
  client.publish("esp32/pinger/RNG", buf);
  Serial.println("DATA PUBLISHED");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial
  Serial.begin(9600);
  Serial2.begin(9600);  // initialize serial2 for nano modem pins 16 (receiver) and 17 (transmitter)
  while (!Serial);

  Serial.println("SLAVE BUOY #2");

  // Connect to Wifi
  setup_wifi();

  // Initialize LoRa
  LoRaInit();

  // Connect to MQTT broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);       // listen for incoming messages

  // Prepare Serial2 to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  output.connect(Serial2);            // where "output" will read from
  sfReader.connect(output);
}

void loop() {
  // Check the connection to the broker
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // If there is something written on Serial2,
  if (sfReader.read()){                                 // if not, send it over LoRa
    char *ack;
    ack = strstr(sfReader.c_str(), "ACK");              // check if the message contains "RNG", if not we don't send it
    if (!ack){
        // Publish data
        publishMQTT(sfReader);
    }
  }

  // Release one byte from the buffer each 9600 baud
  output.nextByteOut();
}
