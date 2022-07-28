/*
  MASTER BUOY
  When receiving the "$G..." message, sends a PING request to buoys 2 and 3,
  receive the calculated RNG by Wifi, pings the diver too and sends the 3 RNG
  via LoRa to the board on the coast.
*/

// Include libraries
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"


// Initialize variables
const char* ssid = "ESP32-Access-Point";  // Wifi
const char* password = "123456789";
WiFiServer server(80);

const char *mqtt_broker = "192.168.4.2";  // MQTT Broker info
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;                     // creation of a client for the broker
PubSubClient client(espClient);

const long frequency = 866E6;             // LoRa Frequency
const int csPin = 5;                      // LoRa radio chip select
const int resetPin = 4;                   // LoRa radio reset
const int irqPin = 27;                    // change for your board; must be a hardware interrupt pin

byte localAddress = 0xAA;                 // address of this device
const byte destinationCoast = 0xFF;       // destination of board on the coast
byte msgCount = 0;                        // count of outgoing messages
byte sender;                              // sender address

String range[3] = {};                     // tab to stock the RNG calculated by each buoy

createBufferedOutput(output, 255, DROP_UNTIL_EMPTY);  // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial2
createSafeString(accoustic, 50);                      // SafeStringReader holding the accoustic signal to cast
createSafeString(msgToCoast, 60);                     // SafeStringReader holding the final message to sent to the coast board


// Create Wifi connection from the ESP32
void createWifi(){
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

// Initialize LoRa
void LoRaInit(){
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {           
    Serial.println("LoRa init failed. Check your connections.");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("LoRa init succeeded.\n");
}

// Connect to MQTT broker for Wifi communication
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("esp32/pinger/RNG");     // subscribe
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

// Send a message to the coast board
void sendMessage(SafeString& outgoing, byte destination) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

// Receive a message from the coast board
void onReceive(int packetSize) {
  if (packetSize == 0) return;    // if there's no packet, return        

  // read packet header bytes:
  int recipient = LoRa.read();
  sender = LoRa.read();
  byte incomingMsgId = LoRa.read();
  byte incomingLength = LoRa.read();

  // read the message
  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  // check length for error
  if (incomingLength != incoming.length()) {
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // If the recipient isn't this device or broadcast,
  if (recipient != localAddress || sender != destinationCoast) {
    return;                             // skip rest of function
  }

  // If message received is from the coast, transmit it to buoys
  if (sender == destinationCoast ){
    accoustic = incoming.c_str();      // print the incoming message in the SafeString
    
    publishMQTT(accoustic, 1);         // send the message to slave buoys via MQTT
    delay(200);                               
    publishMQTT(accoustic, 2);   
    delay(200);
    
    Serial2.println(accoustic);        // print incoming message on Serial2 so main buoy pings

    Serial.println("PING REQUEST SENT \n");
  }
}

// Send a message to slave buoys via MQTT
void publishMQTT(SafeString& msg, int slave){
  // Convert sfReader which holds the ping request into a char*
  const char* msg_char = msg.c_str();

  // Publish the ping response to the desired slave buoy
  switch(slave){
    case 1:
      client.publish("esp32/pinger/request/S1", msg_char);
      break;

    case 2:
      client.publish("esp32/pinger/request/S2", msg_char);
      break;

    default:
      break;
  }
}

// Receive a message from slave buoys via MQTT
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Read the received message
  String messageTemp;
  for (int i = 0; i < length; i++) 
    messageTemp += (char)message[i];

  // Check from wich slave buoy the message has been received
  char *slave1, *slave2;
  slave1 = strstr(messageTemp.c_str(), "slave1");      // check if the message received by master contains "slave1" 
  slave2 = strstr(messageTemp.c_str(), "slave2");      // check if the message received by master contains "slave2"

  // Treatment of the received message depending on the sender
  if (slave1){
    // Remove the "slave1" string from the message
    char* char_arr;
    char_arr = &messageTemp[0];
    str_rem(char_arr, "slave1 ");

    // Stock the message in the RNG tab
    range[1] = "B2 : " + messageTemp;              
    Serial.println("Message from slave buoy #1");
    Serial.println(range[1]);
  }
  else if (slave2){
    // Remove the "slave2" string from the message
    char* char_arr;
    char_arr = &messageTemp[0];
    str_rem(char_arr, "slave2 ");

    // Stock the message in the RNG tab
    range[2] = "B3 : " + messageTemp;               
    Serial.println("Message from slave buoy #2");
    Serial.println(range[2]);
  }
  
  Serial.println();
}

// Remove a word from a message
int str_rem(char *s, char const *srem){
  int n = 0; char *p;
 
  if (s && srem){
    size_t const len = strlen(srem);
 
    while((p = strstr(s, srem)) != NULL){
      memmove(p, p + len, strlen(p + len) + 1);
      n++;
    }
  }
 
  return n;
}

// Stock the ping response received by master buoy in the RNG tab
void responseB1(){
  // If something is written on Serial2
  if (sfReader.read()){
    char *rng;
    rng = strstr(sfReader.c_str(), "ACK");    // check if the message contains "ACK"  

    // If it don't, stock it in the tab
    if (!rng){
      range[0] = "B1 : ";
      range[0] += sfReader.c_str();
      Serial.println(range[0]);
    }
  }
}

// Check if the RNG tab has all its case filled
bool isTabFilled() {
  for (int i = 0; i < 3; i++) {
    if (range[i].isEmpty())
      return false;
  }
  return true;
}

// Empty the RNG tab 
void emptyTab() {
  for (int i = 0; i < 3; i++)
    range[i] = "";

  Serial.println("Tab empty\n");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial
  Serial.begin(9600);
  Serial2.begin(9600);  // initialize serial2 for nano modem pins 16 (receiver) and 17 (transmitter)
  while (!Serial);

  Serial.println("MAIN BUOY");
  
  // Connect to Wi-Fi network with SSID and password
  createWifi();

  // Initialize LoRa
  LoRaInit();

  // Connect to MQTT broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);     // listen for incoming messages

  // Prepare Serial2 to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  output.connect(Serial2);            // where "output" will read from
  sfReader.connect(output);
}

void loop() {
  // Check the connection to the MQTT broker
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Receive data from the coast board
  onReceive(LoRa.parsePacket());

  // Stock the ping response received by master buoy in the RNG tab
  responseB1();  

  // Send the RNG tab to the board on the coast over LoRa
  if(isTabFilled()){                            // if all 3 RNG received
     for(int i=0; i<3; i++){
       msgToCoast = range[i].c_str();
       sendMessage(msgToCoast, destinationCoast);
     }
     Serial.println("Messages sent.");

     // Empty the RNG tab
     emptyTab();
  }

  // Reset the value of sender
  sender = 0;

  // Release one byte from the buffer each 9600 baud
  output.nextByteOut();
}
