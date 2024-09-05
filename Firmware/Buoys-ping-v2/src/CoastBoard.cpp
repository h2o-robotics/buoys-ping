/*
  COAST BOARD
  The coast can make 1 master communicate with multiple divers.

  Sends the "$G..." message to the master buoy via LoRa every 10 sec,
  and receive the RNG calculated by the 3 buoys. 
*/

#ifdef COAST_BOARD

// Include libraries
#include "CoastBoard.h"

// Initialize variables
String outgoing[10];                                  // outgoing messages tab (coast can communicate with 10 divers max)
String outgoing_i;                                    // save the current outgoing message

int measuringInterval = 15000;                        // interval between each ping request (ms)
unsigned long lastTime = 0;
int i = 0;

String RNGdata[10];                                   // save the RNG data received from the master (non sorted)

createBufferedOutput(input, 255, DROP_UNTIL_EMPTY);   // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial2
createSafeString(accoustSignal, 50);                  // SafeString to save the "$G..." message as long as it is the same

createSafeString(LORAsendBuffer, 50);

// Receive a message from master buoy
void onReceive(int packetSize) 
{
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
    Serial.println("[CU] error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != LORA_CoastAddress || sender != LORA_MasterAddress) {
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  //Serial.println("Message ID: " + String(incomingMsgId));
  
  Serial.println(incoming);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial
  Serial.begin(9600);
  if (!Serial)
  {
    Serial.println("[CU] Serial failed. Rebooting...");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("[CU] COAST BOARD");

  // Prepare Serial to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  input.connect(Serial);              // where "input" will read from
  sfReader.connect(input);

  // Initialize LoRa
  LORA_init();

  // Initialize LoRa automatic send buffer
  LORAsendBuffer = "$G,007,RNG";
} 


void loop() 
{
  unsigned long currentTime = millis();
  // Save text written on Serial
  while (Serial.available()) {
    if (sfReader.read()){
      Serial.print("[CU] Forwarding '");
      Serial.print(sfReader);
      Serial.println("' to Master Bouy...");
      
      LORA_sendMessage(sfReader, LORA_CoastAddress, LORA_MasterAddress);
    }
  delay(10);
  }

  if (AUTOMATIC_MEASUREMENT && currentTime - lastTime >= measuringInterval) {
    Serial.println("[CU] Sending automatically '$G,007,RNG' to Master Bouy...");
    LORA_sendMessage(LORAsendBuffer, LORA_CoastAddress, LORA_MasterAddress);
    lastTime = currentTime;
  }

  // Receive the RNG tab from master buoy
  onReceive(LoRa.parsePacket());

  // Release one byte from the buffer each 9600 baud
  input.nextByteOut();
}

#endif
