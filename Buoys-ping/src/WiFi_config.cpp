#include "WiFi_config.h"

WiFiServer server(80);

void connect_to_wifi(){
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

bool create_wifi_ap(){
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