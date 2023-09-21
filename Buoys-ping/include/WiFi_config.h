#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>

#define WIFI_SSID      "ESP32-Access-Point"
#define WIFI_PASSWORD  "123456789"

// Connect the board to a Wifi Access Point
void connect_to_wifi();

#endif