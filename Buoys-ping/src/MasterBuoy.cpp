/*
  MASTER BUOY
  When receiving the "$G..." message, sends a PING request to buoys 2 and 3,
  receive the calculated RNG by Wifi, pings the diver too and sends the 3 RNG
  via LoRa to the board on the coast.
*/

#include "MasterBuoy.h"

// Connect the board to a Wifi Access Point
void setup_wifi(){
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.disconnect();
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
      Serial.println("Wifi connection failed, retry in 1 sec");
      delay(1000);
  }
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

// Connect to MQTT broker for Wifi communication
void reconnectMQTT() {
  // Save the IP address of the device in a char* variable
  IPAddress ip = WiFi.localIP();
  mqtt_broker = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("esp32/pinger/RNG");     // subscribe

      MQTTconnection = true;                    // MQTT connection established
      msgToCoast = "MASTER BUOY : MQTT connection established";
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

// Send a message to the coast board
void sendMessage_LoRa(SafeString& outgoing, byte destination) {
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
void onReceive_LoRa(int packetSize) {
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
    
    publishMQTT(accoustic, 1);         // send the ping request to slave buoys via MQTT
    delay(200);                               
    publishMQTT(accoustic, 2);   
    delay(200);

    Serial2.println(accoustic);        // print incoming message on Serial2 so main buoy pings

    nbPing++;                          // increase the number of pings by 1
    Serial.println("\nPING REQUEST SENT \n");

    accoustic.clear();                 // clear SafeString
  }
}

// Receive a message from slave buoys via MQTT
void callbackMQTT(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Read the received message
  String messageTemp;
  for (int i = 0; i < length; i++) 
    messageTemp += (char)message[i];

  // Check which type of message has been received from the slave buoys (MQTT connection or RNG data ?)
  char *slave1, *slave2, *MQTT;
  slave1 = strstr(messageTemp.c_str(), "slave1");      // check if the message received by master contains "slave1" 
  slave2 = strstr(messageTemp.c_str(), "slave2");      // check if the message received by master contains "slave2"
  MQTT = strstr(messageTemp.c_str(), "MQTT");          // check if the message received by master contains "MQTT" 

  // Treatment of the received message depending on the type of message
  if (slave1){
    msgReceived2 ++;               // 1 RNG data was received from slave 1
    
    // Concatenate the 2 messages in one
    const char* one = "\nB2 : ";
    const char* two = &messageTemp[7]; // 7 because we don't want to read the 7 characters of "slave1 " in messageTemp
    
    char buf[100];
    strcpy(buf, one);
    strcat(buf, two);

    // Stock the message in the RNG tab
    range += buf;              
    Serial.println("Message from slave buoy #1");
    Serial.println(range);
  }
  else if (slave2){
    msgReceived3 ++;                // 1 RNG data was received from slave 2
    
    // Concatenate the 2 messages in one
    const char* one = "\nB3 : ";
    const char* two = &messageTemp[7];  // 7 because we don't want to read the 7 characters of "slave1 " in messageTemp
  
    char buf[100];
    strcpy(buf, one);
    strcat(buf, two);

    // Stock the message in the RNG tab
    range +=  buf;               
    Serial.println("Message from slave buoy #2");
    Serial.println(range);
  }
  else if(MQTT){
    MQTTconnection = true;              // MQTT connection established
    msgToCoast = messageTemp.c_str();
  }
  
  Serial.println();
}

// Stock the ping response received by master buoy in the RNG tab
void responseB1(){
  // If something is written on Serial2
  if (sfReader.read()){
    msgReceived1 ++;                    // 1 RNG data was received from master
    
    range += "\nB1 : ";
    range += sfReader.c_str();
    Serial.println(range);
  }
}

// Check if master received all the RNG data it is supposed to or not
bool isTabFilled() {
  // if we are waiting for the responses of ping #N and we receive it from every buoys
  // (each buoy has to receive 2 messages for the ping to be correctly done : one for the ACK and the other for the RNG)
  if(nbPing == lastPing+1 && (msgReceived1 == 2 && msgReceived2 == 2 && msgReceived3 == 2))
    return true;  // true, we send data

  // if we pass to a new ping #N+1 but we didn't receive the response from all the buoys
  else if (nbPing != lastPing+1 && (msgReceived1 != 0 || msgReceived2 != 0 || msgReceived3 != 0))  
    return true;   // false, but we still send data before emptying the tab

  // if we are waiting for the responses of ping #N and we don't receive it from every buoy
  return false;  // false, but we don't do nothing (either we wait until we receive every responses or the ping #N+1)
}

// Empty the RNG tab 
void emptyTab() {
  range = "";

  Serial.println("TAB EMPTY\n");
}


/************************************************ MULTITHREADING FUNCTIONS ******************************************/

void brokerLoop(void * parameter){
  // Broker initialization
  const unsigned short mqttPort=1883;
  broker.init(mqttPort);
  Serial.println("Broker initialized");
  
  // Update message received
  while(1) {
    broker.update();
    delay(10);        // to avoid watchdog error and reboot of the board
  }
    
}

void masterBuoyLoop(void * parameter){ 
  // Initialize LoRa
  LoRaInit();

  // Connect to MQTT broker
  client.setServer(mqtt_broker.c_str(), mqtt_port);
  client.setCallback(callbackMQTT);       // listen for incoming messages

  // Prepare Serial2 to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  output.connect(Serial2);            // where "output" will read from
  sfReader.connect(output);

  while(1){
    // Check the connection to the MQTT broker
    if (!client.connected()) {
      reconnectMQTT();
    }
    client.loop();

    // If the MQTT connection is established with 1 of the 3 buoys
    if(MQTTconnection){
      // Send a connexion confirmation message to the coast board
      sendMessage_LoRa(msgToCoast, destinationCoast);

      // Change the connection state so †he message is sent just once
      MQTTconnection = false;
    }
  
    // Receive data from the coast board
    onReceive_LoRa(LoRa.parsePacket());
  
    // Stock the ping response received by master buoy in the RNG tab
    responseB1();  
  
    // Send the RNG tab to the board on the coast over LoRa
    if(isTabFilled()){                            // if at least 1 of the 3 RNG is received
     msgToCoast = range.c_str();
     sendMessage_LoRa(msgToCoast, destinationCoast);
     Serial.println("MESSAGE SENT.\n");

     // Reinitialization of the variable for the receive check
     msgReceived1 = 0, msgReceived2 = 0, msgReceived3 = 0;
     
     // Empty the RNG tab
     emptyTab();
    }

    // Save the number of the previous ping
    lastPing = nbPing - 1;
  
    // Reset the value of sender
    sender = 0;
  
    // Clear SafeStrings
    output.nextByteOut();
    msgToCoast.clear();
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TaskHandle_t Task1, Task2; // Task handles serve the purpose of referencing a particular task in other parts of the code

void setup() {
  // Initialize serial
  Serial.begin(9600);
  Serial2.begin(9600);  // initialize serial2 for nano modem pins 16 (receiver) and 17 (transmitter)
  while (!Serial);

  Serial.println("MAIN BUOY");

  // Connect to Wifi AP
  setup_wifi();

  // Multi threading 
  xTaskCreatePinnedToCore(brokerLoop,    "Broker Task",    10000,    NULL,    1,    &Task2,    0);
  xTaskCreatePinnedToCore(masterBuoyLoop,    "Master Buoy Task",    10000,      NULL,    1,    &Task1,    1);
}

void loop() {}
