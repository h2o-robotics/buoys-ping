#include "WiFi_config.h"

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