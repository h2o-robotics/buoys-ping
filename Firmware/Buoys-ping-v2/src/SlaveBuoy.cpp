/*
  SLAVE BUOY
  Receive the ping request from the master buoy, ping the desired
  diver and send the response to the master over ESP_NOW.
*/

#ifdef SLAVE_BUOY

#include "SlaveBuoy.h"

TinyGPS gps;
SoftwareSerial ss(14, 26);

float flat = 0.0, flon = 0.0;
unsigned long age = 0, prec = 0;

bool measuring = false;

String range = "";                        // stock the RNG calculated by each buoy

void reInit_esp_now() {
  // DISCONNECT

  WiFi.mode(WIFI_OFF);

  esp_now_unregister_recv_cb();
  esp_now_unregister_send_cb();

  uint8_t master_mac[] = MASTER_MAC;

  esp_now_del_peer(master_mac);

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

  esp_now_setup_slave();
}

// Receive a message
void callback(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  String messageTemp = "";                         // message received by Serial2

  Serial.println("Message has arrived. Message:");

  request_ping_t data = *(request_ping_t*)incomingData;

  // Read the received message
  for (int i = 0; i < strlen(data.request); i++)
  {
    Serial.print((char)data.request[i]);
    messageTemp += (char)data.request[i];
  }
  Serial.println();

  measuring = true;

  // Print the message on Serial2
  Serial.println("Message forwarding via Serial2 port (ESP32 -> nordic uC board -> nano modem)");
  Serial2.println(messageTemp);

}

// Send a RNG message
void publishData(const char* msg)
{
  response_ping_t buf;

  const char* two = msg;

  // Copy to message buffer
  strcpy(buf.response, two);
  buf.slave_id = SLAVE_ID;

  // Send the ping response
  esp_now_send_ping_data(buf);

  Serial.print("DATA PUBLISHED : ");
  Serial.println(buf.response);
}

int parseAccoustic(String inputString) 
{
  int result = -1;
  
  int start_index = inputString.indexOf("$", 0);
  int largest_start_ind = -1;

  while(start_index != -1){

    String auxStr = inputString.substring(start_index + 7, start_index + 10);

    if (auxStr == "ACK" && result < 1){
      Serial.println("ACK Received");
      result = result > 1 ? result : 1;
    }
    else if (auxStr == "LOG" && result < 2){
      Serial.println("LOG Received");
      result = 2;
      largest_start_ind = start_index;
    }
    else if (auxStr == "RNG" && result < 3){
      Serial.println("RNG Received");
      result = 3;
      largest_start_ind = start_index;
    }
    else{
      Serial.println("Serial error");
      result = -1;
    }

    start_index = inputString.indexOf("$", start_index+1);
  }

  if(result == -1){
    Serial.println("Serial error");
    return 0;

  } else if (result == 2){
    range += (inputString.substring(largest_start_ind + 7, 10) + ",Timeout").c_str();
    
    char flatStr[20];
    char flonStr[20];
    
    dtostrf(flat, 18, 15, flatStr);
    dtostrf(flon, 18, 15, flonStr);
    
    range += String(",") + flatStr + String(",") + flonStr + String(",") + String(prec);

  } else if (result == 3) {
    char flatStr[20];
    char flonStr[20];
    
    dtostrf(flat, 18, 15, flatStr);
    dtostrf(flon, 18, 15, flonStr);
    
    range += (inputString.substring(largest_start_ind + 7, largest_start_ind + 10) + "," + inputString.substring(largest_start_ind + 11, largest_start_ind + 15)).c_str();
    range += String(",") + flatStr + String(",") + flonStr + String(",") + String(prec);
  }

  return result;
}

// Stock the ping response received by buoy
bool responseB()
{
  bool msgRec = false;
  String Brange = "";

  // If something is written on Serial2
  while (Serial2.available())
  {
    char c = Serial2.read();
    if (c != '\r' && c != '\n')
    {
      Brange += c;
    }
    delay(5);
  }
  if (Brange != ""){
    msgRec = true;
  }
  else{
    msgRec = false;
  }
  if (msgRec)
  {
    Serial.println(Brange);

    int res = parseAccoustic(Brange);
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

void GPS_Manager()
{
  bool newData = false;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

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

void setup()
{
  // Initialize serial
  Serial.begin(9600);
  Serial2.begin(9600);  // initialize serial2 for nano modem pins 16 (receiver) and 17 (transmitter)
  ss.begin(9600);

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
      Serial.println("[ESP_NOW] Error initializing ESP-NOW");
      while(1);
  }
  
  // Register the send and recieve callback
  esp_now_register_send_cb(esp_now_custom_send_callback);
  
  esp_err_t status = esp_now_register_recv_cb(callback);
  if (status != ESP_OK){
    Serial.printf("[ESP_NOW] Couldn't add callback! - %s\n", esp_err_to_name(status));
    while(1);
  }

  esp_now_setup_slave();

  // Print to debug serial
  Serial.print("SLAVE BUOY #");
  Serial.println(SLAVE_ID);      
  
  // Setup GPS
  ss.println(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  ss.println(PMTK_SET_NMEA_UPDATE_1HZ);
}

void loop()
{
  String messageTemp = "";                         // message received by Serial2

  bool res = responseB();

  if (measuring == false)
  {
    GPS_Manager();
  }

  if (res)
  {
    Serial.println("Publicando...");
    publishData(range.c_str());
    range = "";
    measuring = false;
  }
}

#endif