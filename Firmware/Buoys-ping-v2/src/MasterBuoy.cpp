/*
  MASTER BUOY
  When receiving the "$G..." message, sends a PING request to buoys 2 and 3,
  receive the calculated RNG by Wifi, pings the diver too and sends the 3 RNG
  via LoRa to the board on the coast.
*/

#ifdef MASTER_BUOY

#include "MasterBuoy.h"

TaskHandle_t Task1, Task2;

TinyGPS gps;
SoftwareSerial ss(14, 26);

float flat = 0.0, flon = 0.0;
unsigned long age = 0, prec = 0;

bool measuring_acustic = false;

byte sender;                              // sender address
String range = "";                        // stock the RNG calculated by each buoy

int nbPing = 0, lastPing = 0;                                // current and last ping counters
int msgReceived1 = 0, msgReceived2 = 0, msgReceived3 = 0;    // variables to check if a RNG data from slaves 1/2 or master has been received

createBufferedOutput(output, 255, DROP_UNTIL_EMPTY);   // create an extra output buffer for the Serial2
createBufferedOutput(output2, 255, DROP_UNTIL_EMPTY);  // create an extra output buffer for the ss (GPS)

createSafeStringReader(sfReader, 50, "\r\n");          // create SafeStringReader to hold messages written on Serial2
createSafeStringReader(sfGPS, 100, "\r");              // create SafeStringReader to hold messages written on ss (GPS)

createSafeString(accoustic, 50);                       // SafeStringReader holding the accoustic signal to cast
createSafeString(msgToCoast, 2000);                    // SafeStringReader holding the final message to sent to the coast board
//createSafeString(msgGPS, 100);                       // SafeStringReader holding the GPS msg

SemaphoreHandle_t myMutex;    //Semaphore

void reInit_esp_now() {
  // DISCONNECT

  WiFi.mode(WIFI_OFF);

  esp_now_unregister_recv_cb();
  esp_now_unregister_send_cb();

  uint8_t slave_1_mac[] = SLAVE_1_MAC;
  uint8_t slave_2_mac[] = SLAVE_2_MAC;

  esp_now_del_peer(slave_1_mac);
  esp_now_del_peer(slave_2_mac);

  esp_now_deinit();

  // CONNECT

  WiFi.mode(WIFI_STA);
  
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
      Serial.println("[ESP_NOW] Error initializing ESP-NOW");
      while(1);
  }
  
  // Register the send and recieve callback
  esp_now_register_send_cb(esp_now_custom_send_callback);
  esp_now_register_recv_cb(callback);

  esp_now_setup_master();
}

// Send a message to slave buoys via ESP_NOW
void publishData(SafeString& msg, int slave)
{
  // Convert sfReader which holds the ping request into a char*
  request_ping_t buf;
  strcpy(buf.request, msg.c_str());

  uint8_t recv_address[6];

  // Publish the ping response to the desired slave buoy
  switch(slave){
    case 1:
    {
      //delay(500);
      uint8_t slave_address[] = SLAVE_1_MAC;
      memcpy(recv_address, slave_address, 6);
    }
      break;
    case 2:
    {
      //delay(1500);
      uint8_t slave_address[] = SLAVE_2_MAC;
      memcpy(recv_address, slave_address, 6);
    }
      break;

    default:
      break;
  }
  esp_now_request_ping(recv_address, buf);
}

// Receive a message from the coast board
void onReceive(int packetSize) 
{

  bool rngCommand = false, mSeqDone = false;
  short mSeq = 0;
  long measurePrevTime, measureTime = 0;

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
  Serial.print("MSG received from Coast Unit (LoRa): ");
  Serial.println(incoming);

  // check length for error
  if (incomingLength != incoming.length()) {
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // If the recipient isn't this device or broadcast,
  if (recipient != LORA_MasterAddress || sender != LORA_CoastAddress) {
    Serial.println("Recipient isn't this device or broadcast");
    return;                             // skip rest of function
  }

  // If message received is from the coast, transmit it to buoys
  if (sender == LORA_CoastAddress)
  {
    Serial.println("Starting RNG measurement sequence...");
    measuring_acustic = true;
    accoustic = incoming.c_str();      // print the incoming message in the SafeString

    measurePrevTime = millis();
    while ((measureTime <= MEASURETIMEOUT) && !mSeqDone)
    {
      mSeqDone = false;
      measureTime = millis() - measurePrevTime;

      switch(mSeq)
      {
        case 0:
          if (!rngCommand){
            Serial.println("\nSENDING PING REQUEST... \n");
            Serial2.println(accoustic);        // print incoming message on Serial2 so main buoy pings

            nbPing++;                          // increase the number of pings by 1

            rngCommand = true;
          }
          else{
            if (responseB1() == true){
              Serial.println("\nB1 RANGE RECEIVED \n");
              rngCommand = false;
              mSeq ++;

              // reInit_esp_now();
            }
          }
          break;

        case 1:
          if (!rngCommand){
            Serial.println("\nSENDING PING REQUEST TO S1... \n");
            publishData(accoustic, 1);
            rngCommand = true;
          }
          else{
            if (msgReceived2){
              Serial.println("\nB2 RANGE RECEIVED \n");
              rngCommand = false;

              //delay(1000);
              mSeq ++;
            }
          }
          break;

        case 2:
          if (!rngCommand){
            Serial.println("\nSENDING PING REQUEST TO S2... \n");
            publishData(accoustic, 2);
            rngCommand = true;
          }
          else{
            if (msgReceived3){
              Serial.println("\nB3 RANGE RECEIVED \n");
              rngCommand = false;

              //delay(1000);
              mSeq ++;
            }
          }
          break;

        default:
            Serial.println("\nRANGE MEASUREMENT SEQUENCE DONE \n");
            // Concatenate sequence time in range msg
            range += ("\n[RNG] T," + String(measureTime)).c_str();
            mSeqDone = true;
          break;
      }
      
      //Serial.print("\nmeasureTime: ");
      //Serial.println(measureTime);
      
    }

    if (measureTime > MEASURETIMEOUT)
    {
      Serial.println("\nRNG COMM TIMEOUT! \n");
      
      msgToCoast = ("[FAIL] RNG Measurement Timeout: " + String(msgReceived1 + msgReceived2 + msgReceived3)).c_str();
      LORA_sendMessage(msgToCoast, LORA_MasterAddress, LORA_CoastAddress);
      measuring_acustic = false;
      msgToCoast.clear();
      emptyTab();
      msgReceived1 = 0, msgReceived2 = 0, msgReceived3 = 0;
    }
    //mSeqDone = false;
    accoustic.clear();                 // clear SafeString
  }

}

// Receive a message from slave buoys
void callback(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  Serial.println("Message has arrived.");

  response_ping_t data = *(response_ping_t*)incomingData;

  // Read the received message
  String messageTemp;
  for (int i = 0; i < strlen(data.response); i++) 
    messageTemp += (char)data.response[i];

  Serial.println(messageTemp.c_str());

  // Treatment of the received message depending on the type of message
  if (data.slave_id == 1){
    msgReceived2 ++;               // 1 RNG data was received from slave 1
    
    // Concatenate the 2 messages in one
    const char* one = "\n[RNG] B2,";
    const char* two = &messageTemp[0];
    
    char buf[100];
    strcpy(buf, one);
    strcat(buf, two);

    // Stock the message in the RNG tab
    range += buf;              
    Serial.println("Message from slave buoy #1");
    Serial.println(range);
  }
  else if (data.slave_id == 2){
    msgReceived3 ++;                // 1 RNG data was received from slave 2
    
    // Concatenate the 2 messages in one
    const char* one = "\n[RNG] B3,";
    const char* two = &messageTemp[0];  
  
    char buf[100];
    strcpy(buf, one);
    strcat(buf, two);

    // Stock the message in the RNG tab
    range +=  buf;               
    Serial.println("Message from slave buoy #2");
    Serial.println(range);
  }
  
  Serial.println();
}

int parseAccoustic(String inputString) 
{
  int result = 0;
  int colonIndex = inputString.indexOf(","); 
  char flatStr[20];
  char flonStr[20];

  String auxStr = inputString.substring(17, 20);
  Serial.println(auxStr);

  dtostrf(flat, 18, 15, flatStr);
  dtostrf(flon, 18, 15, flonStr);

  if (colonIndex >= 0) {
    if (auxStr == "ACK"){
      Serial.println("ACK Received");
      result = 1;
    }
    else if (auxStr == "LOG"){
      Serial.println("LOG Received");
      range += (inputString.substring(1, 10) + inputString.substring(17, 20) + ",Timeout").c_str();
      range += String(",") + flatStr + String(",") + flonStr + String(",") + String(prec);
      result = 2;
    }
    else if (auxStr == "RNG"){
      Serial.println("RNG Received");
      range += (inputString.substring(1, 10) + inputString.substring(17, 20) + "," + inputString.substring(21, 25)).c_str();
      range += String(",") + flatStr + String(",") + flonStr + String(",") + String(prec);
      result = 3;
    }
    else{
      Serial.println("Serial error");
      result = 0;
    }
  }
  else{
    Serial.println("Serial error");
    result = 0;
  }

  return result; 
}

// Stock the ping response received by master buoy in the RNG tab
bool responseB1()
{
  String B1range = "";
  // If something is written on Serial2
  if (sfReader.read()){
    msgReceived1 ++;                    // 1 RNG data was received from master
    
    B1range += "\n[RNG] B1,";
    B1range += sfReader.c_str();
    Serial.println(B1range);

    int res = parseAccoustic(B1range);
    Serial.print("Type received: ");
    Serial.println(res);
    switch(res)
    {
      case 0:           
      case 1:
      default:
        return false;
        break;
      case 2:          
      case 3: 
        return true;
        break;
    }
  }
  return false;
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
}


/************************************************ MULTITHREADING FUNCTIONS ******************************************/

void masterBuoyLoop(void * parameter)
{ 
  // Prepare Serial2 to be read
  SafeString::setOutput(Serial);      // enable error messages and SafeString.debug() output to be sent to Serial
  output.connect(Serial2);            // where "output" will read from
  sfReader.connect(output);

  while(1)
  {
    TaskHandle_t holderTask = xSemaphoreGetMutexHolder(myMutex);
    
    if (holderTask == NULL) {
    
      // Receive data from the coast board
      onReceive(LoRa.parsePacket());
    
      // Send the RNG tab to the board on the coast over LoRa
      if(isTabFilled()){                            // if at least 1 of the 3 RNG is received
        msgToCoast = range.c_str();
        LORA_sendMessage(msgToCoast, LORA_MasterAddress, LORA_CoastAddress);
        Serial.println("MESSAGE SENT.\n");

        // Reinitialization of the variable for the receive check
        msgReceived1 = 0, msgReceived2 = 0, msgReceived3 = 0;
        measuring_acustic = false;
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
      msgReceived1 = 0, msgReceived2 = 0, msgReceived3 = 0;
      emptyTab();
    }
  }
  
}


void GPS_Manager()
{
  bool newData = false;
  char charArray[50];

  while (ss.available())
  {
    xSemaphoreTake(myMutex, (TickType_t)0);
    char c = ss.read();

    if (gps.encode(c)) // Did a new valid sentence come in?
      newData = true;
    
    delayMicroseconds(500);
  }

  xSemaphoreGive(myMutex);

  if (newData)
  {
    gps.f_get_position(&flat, &flon, &age);

    if (gps.hdop() == TinyGPS::GPS_INVALID_HDOP) {
      prec = 0;
    } else {
      prec = gps.hdop();
    }

    if(GPS_LOG_LOCATION){
      Serial.print("LAT=");
      Serial.println(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 10);
      Serial.print("LON=");
      Serial.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 10);
      Serial.print("SAT=");
      Serial.println(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
      Serial.print("PREC=");
      Serial.println(prec + "\n");
    }

  }
}

void setup() {
  Serial.begin(9600);

  // Initialize serial
  
  Serial2.begin(9600);  // initialize serial2 for nano modem pins 16 (receiver) and 17 (transmitter)
  ss.begin(9600);

  Serial.println("MAIN BUOY");

  // Initialize ESP_NOW
  WiFi.mode(WIFI_STA);
  
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
      Serial.println("[ESP_NOW] Error initializing ESP-NOW");
      while(1);
  }
  
  // Register the send and recieve callback
  esp_now_register_send_cb(esp_now_custom_send_callback);
  esp_now_register_recv_cb(callback);

  esp_now_setup_master();
  
  // Initialize LoRa
  LORA_init();

  //GPS Initialized
  ss.println(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  ss.println(PMTK_SET_NMEA_UPDATE_1HZ);
  
  //Semaphore
  myMutex = xSemaphoreCreateMutex();

  // Multi threading 
  
  xTaskCreatePinnedToCore(
    masterBuoyLoop,    
    "Master Buoy Task",    
    10000, NULL, 1,
    &Task1, 1
  );
}

void loop() 
{
  if (!measuring_acustic) GPS_Manager();
}

#endif
