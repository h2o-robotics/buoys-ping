/*
  SLAVE BUOY #1
  Receive the ping request from the master buoy, ping the desired
  diver and send the response to the master over LoRa.
*/

// Include libraries
#include <SPI.h>              
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"

// Initialize variables
const long frequency = 866E6;  // LoRa Frequency 
const int csPin = 5;           // LoRa radio chip select
const int resetPin = 4;        // LoRa radio reset
const int irqPin = 27;         // change for your board; must be a hardware interrupt pin

String incoming;               // incoming message (accoustic signal)

byte msgCount = 0;             // count of outgoing messages
byte localAddress = 0xBB;      // address of this device
byte destination = 0xAA;       // destination of master buoy
long lastSendTime = 0;         // last send time

// SSID & password for Wifi
const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";

// MQTT Broker
const char *mqtt_broker = "192.168.4.2";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader which reads up to 30 characters delimited by \r\n
createBufferedOutput(output, 255, DROP_UNTIL_EMPTY);  // create an extra output buffer


// Initialization of LoRa
void LoRaInit(){
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {           
    Serial.println("LoRa init failed. Check your connections.");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("LoRa init succeeded.\n");
}

//// Send a message
//void sendMessage(SafeString& outgoing) {
//  LoRa.beginPacket();                   // start packet
//  LoRa.write(destination);              // add destination address
//  LoRa.write(localAddress);             // add sender address
//  LoRa.write(msgCount);                 // add message ID
//  LoRa.write(outgoing.length());        // add payload length
//  LoRa.print(outgoing);                 // add payload
//  LoRa.endPacket();                     // finish packet and send it
//  msgCount++;                           // increment message ID
//}


// Receive a message
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  incoming = "";
  
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    Serial.println("Supposed length: " + String(incomingLength));
    Serial.println("Real length: " + String(incoming.length()));
    Serial.println();
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress || sender != destination) {
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();

  // Print incoming message on Serial2
  Serial2.println(incoming);
}

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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
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

void publishMQTT(SafeStringReader& sfReader){
  // Convert sfReader which holds the ping response into a char*
  const char* one = "slave1 ";
  const char* two = sfReader.c_str();

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

  Serial.println("SLAVE BUOY #1");

  // Connect to Wifi
  setup_wifi();

  // Connect to MQTT broker
  client.setServer(mqtt_broker, mqtt_port);

  // Initialize LoRa
  LoRaInit();

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
  
  // Release one byte from the buffer each 9600 baud
  output.nextByteOut();
  
  // If there is something written on Serial2,
 if (sfReader.read()){                                 // if not, send it over LoRa
    char *ack;
    ack = strstr(sfReader.c_str(), "ACK");              // check if the message contains "RNG", if not we don't send it
    if (!ack){
        // Publish data
        publishMQTT(sfReader);
    }
  }

  // Parse for a packet, and call onReceive with the result
  onReceive(LoRa.parsePacket());
}
