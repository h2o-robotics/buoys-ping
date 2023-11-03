#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>

#define WIFI_SSID      "ESP32-Access-Point"
#define WIFI_PASSWORD  "123456789"

// Connect the board to a Wifi Access Point using prefefined macros
void WIFI_connect_macro();

// Connect the board to a Wifi Access Point with credentials recieved by user via Serial prompt
void WIFI_connect_userPrompt();

// Force Wifi to reconnect to the Access Point
void WIFI_force_reconnect();

// Create Wifi connection from the ESP32
bool WIFI_create_ap();

#endif