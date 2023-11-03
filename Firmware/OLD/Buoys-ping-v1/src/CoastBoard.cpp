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

int interval = 10000;                                 // interval between each ping request (ms)
int i = 0;

String RNGdata[10];                                   // save the RNG data received from the master (non sorted)

createBufferedOutput(input, 255, DROP_UNTIL_EMPTY);   // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial2
createSafeString(accoustSignal, 50);                  // SafeString to save the "$G..." message as long as it is the same

// Empty the RNGdata tab 
void emptyTab(String tab[]) {
  for(int i=0; i<6; i++) // 6 because the RNG data tab sent to ThingsBoard always has 6 cases
    tab[i] = "";
}

// Split a message in several, put them in a tab and recover the size of the tab
int split(String tab[], char *str, const char * delimiter) {
  int i = 0; // counts the number of diver to ping 

  // Returns first token and save it in the tab
  char *token = strtok(str, delimiter);
  tab[i] = token;

  // Keep spliting tokens while one of the
  // delimiters present in str[].
  while (token != NULL) {
    token = strtok(NULL, delimiter);
    i++;
    tab[i] = token;        // save each token in the tab
  }

  return i;
}

// Sort the RNG data message received from the master so it always shows data in the same order
String sortRNGmessage(String messsage){
  char *Bo1, *Bo2, *Bo3, *ack, *rng, *timeout; int i; String new_RNGdata[6]; String finalMsg = "";
  
  // Split the RNG message into submessages every time ther is a new line
  char str[messsage.length() + 1];   // convert String into char array
  strcpy(str, messsage.c_str());
  
  int tabSize = split(RNGdata, str, "\n");     

  // Empty the new tab that will stock the submessages in the correct order
  emptyTab(new_RNGdata);
  
  // Sort each split in a specific way
  for(i=0; i<tabSize; i++){
    Bo1 = strstr(RNGdata[i].c_str(), "B1");    // check if the message contains a specific word
    Bo2 = strstr(RNGdata[i].c_str(), "B2");
    Bo3 = strstr(RNGdata[i].c_str(), "B3");
    ack = strstr(RNGdata[i].c_str(), "ACK");
    rng = strstr(RNGdata[i].c_str(), "RNG");
    timeout = strstr(RNGdata[i].c_str(), "timeout");

    // Put the submessage in a specific position in the tab according to its content
    if(Bo1 && ack)
      new_RNGdata[0] = RNGdata[i];                // put the submessage in a new tab at the correct position
    else if(Bo2 && ack)
      new_RNGdata[1] = RNGdata[i];
    else if(Bo3 && ack)
      new_RNGdata[2] = RNGdata[i];
    else if(Bo1 &&(rng || timeout))
      new_RNGdata[3] = RNGdata[i];
    else if(Bo2 &&(rng || timeout))
      new_RNGdata[4] = RNGdata[i];
    else if(Bo3 &&(rng || timeout))
      new_RNGdata[5] = RNGdata[i];
  }

  // Read every case of the new RNG data tab and put the content in a String, 
  // to be able to send it to ThingsBoard
  for(i=0; i<6; i++){
    finalMsg += "\n";
    finalMsg += new_RNGdata[i];
  }

  Serial.println("TAB SORTED");
  Serial.println(finalMsg);
  
  return finalMsg;
}

// Receive a message from master buoy
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
  if (recipient != LORA_CoastAddress || sender != LORA_MasterAddress) {
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println(incoming);
  Serial.println();

  // Uploads new telemetry to ThingsBoard using MQTT
  char* MQTT = strstr(incoming.c_str(), "MQTT");      // check if the message contains "MQTT" 
  
  if (!MQTT){                                         // if not, data is sent bc we don't want to send the MQTT confirmation message
    // Send RNG data to ThingsBoard
    String RNGdataSorted = sortRNGmessage(incoming);  // sort the RNG data received in the correct order
    
    char d[RNGdataSorted.length() + 1];               // convert string into char array
    strcpy(d, RNGdataSorted.c_str());
    //tb.sendTelemetryString("Buoys RNG", d);         

    // Send the "id" of the diver that has be pinged to ThingsBoard
    char nameDiver[outgoing[i].length() + 1];       // convert string into char array
    strcpy(nameDiver, outgoing_i.c_str());
    //tb.sendTelemetryString("Buoys RNG", nameDiver);
          
    //tb.loop();                                      // process messages 

    Serial.print("Sending data to dashboard...");
  }
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // Initialize serial
  Serial.begin(9600);
  while (!Serial);

  Serial.println("COAST BOARD");

  // Prepare Serial to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  input.connect(Serial);              // where "input" will read from
  sfReader.connect(input);

  // WiFi connection
  WIFI_connect_userPrompt();

  // Initialize LoRa
  LORA_init();
}


void loop() {
  // Reconnect to WiFi, if needed
  WIFI_force_reconnect();

  // Save text written on Serial
  while (Serial.available()) {
    if (sfReader.read())
      accoustSignal = sfReader;
  }

  // Put each diver ping command in a tab and recover the size of the tab
  char str[accoustSignal.length() + 1];   // convert String into char array
  strcpy(str, accoustSignal.c_str());
  
  int sizeOutgoing = split(outgoing, str, ";");

  // Send a ping request to each diver one after the other, every time frame
  if (runEvery(interval)) {               // process every 5 seconds
    if (accoustSignal.isEmpty() == 0) {   // if the message to send is not empty
      if (i == sizeOutgoing)
        i = 0;                            // start again from beginning of tab when end is reached

      LORA_sendMessage(outgoing[i], LORA_CoastAddress, LORA_MasterAddress);           // send ping to the desired diver
      
      Serial.println();
      Serial.print("\nSIGNAL SENT : "); Serial.println(outgoing[i]);
      Serial.println();
      
      outgoing_i = outgoing[i]; 
      i++;
    }
  }

  // Receive the RNG tab from master buoy
  onReceive(LoRa.parsePacket());

  // Release one byte from the buffer each 9600 baud
  input.nextByteOut();
}

#endif
