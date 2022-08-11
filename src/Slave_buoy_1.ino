/*
  SLAVE BUOY #1
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
WiFiServer server(80);
bool wifiConnected = false;

const char *mqtt_broker = "192.168.4.2";    // MQTT Broker info : has to be changed depending on the board the broker is running on
const char *mqtt_username = "slave1";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
bool MQTTconnection = false;                // stock the MQTT connection state

String messageTemp;                         // message received by MQTT

WiFiClient espClient;                       // creation of a client for the broker
PubSubClient client(espClient);

createBufferedOutput(output, 255, DROP_UNTIL_EMPTY);  // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial
createSafeString(MQTTstate, 100);             // create a SafeString to hold the state of the MQTT connection of the buoy


// Initialization of LoRa
void LoRaInit(){
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {           
    Serial.println("LoRa init failed. Check your connections.");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("LoRa init succeeded.\n");
}

// Create Wifi connection from the ESP32
bool createWifi(){
  try{
    Serial.print("Setting AP (Access Point)…");
    WiFi.softAP(ssid, password);
  
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    server.begin();

    wifiConnected = true;
    Serial.println("Wifi creation succeed !\n");
    return true;
  }
  catch(bool wifiConnected){
    Serial.println("AP Wifi creation failed !\n");
    return false;
  }
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
      client.subscribe("esp32/pinger/request/S1");      // subscribe to receive ping request

      MQTTconnection = true;                    // MQTT connection established
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
  
  // Read the received message
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Print the message on Serial2
  Serial2.println(messageTemp);
  messageTemp = "";                 // clear variable
}

// Send a RNG message
void publishMQTT(SafeString& msg, bool RNG){
  char buf[100];
  
  // If the message to be sent is a RNG data
  if(RNG){
    // Convert sfReader which holds the ping response into a char*
    const char* one = "slave1 ";
    const char* two = msg.c_str();
  
    // Concatenate the 2 char
    
    strcpy(buf, one);
    strcat(buf, two);
  }

  // If the message to be published is NOT a RNG data 
  else
    strcpy(buf, msg.c_str());
  
  // Publish the ping response
  client.publish("esp32/pinger/RNG", buf);
  Serial.print("DATA PUBLISHED : "); Serial.println(buf);
  
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial
  Serial.begin(9600);
  Serial2.begin(9600);  // initialize serial2 for nano modem pins 16 (receiver) and 17 (transmitter)
  while (!Serial);

  Serial.println("SLAVE BUOY #1");

  // Connect to Wi-Fi network with SSID and password
  while(!wifiConnected)
    createWifi();
  
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

  // If the MQTT connection is established with the slave
  if(MQTTconnection){
    // Send a connexion confirmation message to the coast board
    MQTTstate = "SLAVE BUOY #1 : MQTT connection established";
    publishMQTT(MQTTstate, false);

    // Change the connection state so †he message is sent just once
    MQTTconnection = false;
  }
  
  // If there is something written on Serial2,
  if (sfReader.read()){                               
    if (sfReader != messageTemp.c_str())                // check if the message written on Serial2 is not the ping request received
       publishMQTT(sfReader, true);                     // publish data
  }

  // Release one byte from the buffer each 9600 baud
  output.nextByteOut();
}
