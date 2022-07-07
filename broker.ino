#include"sMQTTBroker.h"

sMQTTBroker broker;

void setup()
{
    Serial.begin(115200);
    const char* ssid = "ESP32-Access-Point";
    const char* password = "123456789";
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
    }
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    
    const unsigned short mqttPort=1883;
    broker.init(mqttPort);
    Serial.println("Broker initialized");
    // all done
}
void loop()
{
    broker.update();
}
