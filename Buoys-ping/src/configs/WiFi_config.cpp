#include "configs/WiFi_config.h"

WiFiServer server(80);

char wifi_ssid[30];                                        // Wifi AP
char wifi_password[30];                                    // password

void WIFI_connect_macro(){
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
      Serial.println("Wifi connection failed, retry in 1 sec");
      delay(1000);
  }

  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void WIFI_connect_userPrompt(){

  // Ask the user to enter the name and password of the WiFi AP they want to be connected on
  Serial.println("Enter WiFi name :");
  while (Serial.available() == 0 );             
  
  String entry = Serial.readString();              // read the entry from Serial Monitor
  entry.toCharArray(wifi_ssid, entry.length());    // convert String into char array
  Serial.println(wifi_ssid); 

  Serial.println("Enter WiFi password :");
  while (Serial.available() == 0 );           
  
  entry = Serial.readString();
  entry.toCharArray(wifi_password, entry.length());   
  Serial.println(wifi_password);

  // Connect the board to the selected WiFi AP
  Serial.print("Connecting to AP ...");

  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(WiFi.status());
  }

  Serial.println("\nConnected to AP");
}

void WIFI_force_reconnect(){
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.begin(wifi_ssid, wifi_password);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("Connected to AP");
  }
}

bool WIFI_create_ap(){
  try{
    Serial.print("Setting AP (Access Point)…");
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    server.begin();

    Serial.println("Wifi creation succeed !\n");
    return true;
  }
  catch(bool wifiConnected){
    Serial.println("AP Wifi creation failed !\n");
    return false;
  }
}