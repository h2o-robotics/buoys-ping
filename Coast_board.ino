/*
  COAST BOARD
  Send the "$G..." message to the master buoy via LoRa every 8 sec, 
  and receive the RNG calculated by the 3 buoys.
*/

// Include libraries
#include <SPI.h>              
#include <LoRa.h>
#include <WiFi.h>
#include <Vector.h>
#include "BufferedOutput.h"
#include "SafeStringReader.h"


// Initialize variables
const long frequency = 866E6;   // LoRa Frequency 
const int csPin = 5;            // LoRa radio chip select
const int resetPin = 4;         // LoRa radio reset
const int irqPin = 27;          // change for your board; must be a hardware interrupt pin

String outgoing;                // outgoing message

byte msgCount = 0;              // count of outgoing messages
byte localAddress = 0xFF;       // address of this device
const int ELEMENT_COUNT_MAX = 10;
int storage_array[ELEMENT_COUNT_MAX];
Vector<int> destination;       // tab of master buoys that will have to ping

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
void sendMessage(SafeString& outgoing) {
  for(int i = 0; i < destination.size(); i++){
    LoRa.beginPacket();                   // start packet
    LoRa.write(destination[i]);           // add destination address
    LoRa.write(localAddress);             // add sender address
    LoRa.write(msgCount);                 // add message ID
    LoRa.write(outgoing.length());        // add payload length
    LoRa.print(outgoing);                 // add payload
    LoRa.endPacket();                     // finish packet and send it
    msgCount++;                           // increment message ID
  }
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
  if (recipient != localAddress) {
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Sender : " + String(sender));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println(incoming);
  Serial.println();
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

  // Initialization of masters that will com. with the coast
  destination.setStorage(storage_array);
  const int master1 = 172;
  const byte master2 = 0xAA2;
  destination.push_back(master1);
  destination.push_back(master2);

  // Recover the device's MAC address
  byte mac[6];
  memcpy(&localAddress, WiFi.macAddress(mac), sizeof(int)); // convert byte array into int
}


void loop() {
  // Send text written on Serial over LoRa to master buoy
  while (Serial.available()){  
    if(sfReader.read()){
      accoustSignal = sfReader;
      sendMessage(accoustSignal);
      Serial.println("\nSIGNAL SENT\n"); 
    }
  }
  
  // Receive the RNG tab from master buoy
  onReceive(LoRa.parsePacket());

  // Release one byte from the buffer each 9600 baud
  input.nextByteOut();
}
