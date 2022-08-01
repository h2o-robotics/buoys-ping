/*
  COAST BOARD
  The coast can make 1 master communicate with multiple divers.
  
  Send the "$G..." message to the master buoy via LoRa every 8 sec, 
  and receive the RNG calculated by the 3 buoys.
*/

// Include libraries
#include <SPI.h>              
#include <LoRa.h>
#include "BufferedOutput.h"
#include "SafeStringReader.h"


// Initialize variables
const long frequency = 866E6;   // LoRa Frequency 
const int csPin = 5;            // LoRa radio chip select
const int resetPin = 4;         // LoRa radio reset
const int irqPin = 27;          // change for your board; must be a hardware interrupt pin

String outgoing[10];            // outgoing message tab (coast can communicate with 10 divers)

byte msgCount = 0;              // count of outgoing messages
byte localAddress = 0xFF;       // address of this device
byte destination = 0xAA;        // master buoy

int interval = 8000;            // interval between each ping request (ms)
int i = 0;

createBufferedOutput(input, 255, DROP_UNTIL_EMPTY);   // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial2
createSafeString(accoustSignal, 50);                  // SafeString to save the "$G..." message as long as it is the same


// Initialization of LoRa
void LoRaInit(){
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {           
    Serial.println("LoRa init failed. Check your connections.");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("LoRa init succeeded.\n");
}


// Send a message to master buoy
void sendMessage(String& outgoing) {
    LoRa.beginPacket();                   // start packet
    LoRa.write(destination);              // add destination address
    LoRa.write(localAddress);             // add sender address
    LoRa.write(msgCount);                 // add message ID
    LoRa.write(outgoing.length());        // add payload length
    LoRa.print(outgoing);                 // add payload
    LoRa.endPacket();                     // finish packet and send it
    msgCount++;                           // increment message ID
    Serial.println(outgoing);
}


// Receive a message from maester buoy
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

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

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress || sender != destination) {
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message: " + incoming);
  Serial.println();
}


// Counts an interval of time in milliseconds
boolean runEvery(unsigned long interval) {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }

  return false;
}

// Put each diver ping command in a tab and recover the size of the tab
int splitPings(){
  int i = 0; // counts the number of diver to ping
  
  // Convert String into char array
  char str[accoustSignal.length() + 1]; 
  strcpy(str, accoustSignal.c_str()); 
 
  // Returns first token and save it in the outgoing message tab
  char *token = strtok(str, ";");
  outgoing[i] = token;
 
  // Keep printing tokens while one of the
  // delimiters present in str[].
  while (token != NULL){
      token = strtok(NULL, ";");
      i++;
      outgoing[i] = token;        // save each token in the outgoing message tab
  }

  return i;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial
  Serial.begin(9600);
  while (!Serial);

  Serial.println("COAST BOARD");

  // Initialize LoRa
  LoRaInit();

  // Prepare Serial to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  input.connect(Serial);              // where "input" will read from
  sfReader.connect(input);
}


void loop() {
  // Save text written on Serial
  while (Serial.available()){  
    if(sfReader.read())
      accoustSignal = sfReader;
  }

  // Put each diver ping command in a tab and recover the size of the tab
  int sizeOutgoing = splitPings();

  // Send a ping request to each diver one after the other, every time frame
  if (runEvery(interval)){                // process every 5 seconds
    if(accoustSignal.isEmpty() == 0) {    // if the message to send is not empty
      if(i == sizeOutgoing)   
        i = 0;                            // start again from beginning of tab when end is reached
     
      sendMessage(outgoing[i]);           // send ping to the desired diver
      Serial.println("\nSIGNAL SENT\n");
      
      i++;
    }
  }
  
  // Receive the RNG tab from master buoy
  onReceive(LoRa.parsePacket());

  // Release one byte from the buffer each 9600 baud
  input.nextByteOut();   
}
