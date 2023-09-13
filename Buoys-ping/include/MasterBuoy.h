#ifndef MASTER_BUOY_H
#define MASTER_BUOY_H

#include<Arduino.h>

#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"
#include "sMQTTBroker.h"

#include "LoRa_config.h"

// Initialize variables
const char* ssid = "ESP32-Access-Point";  // Wifi 
const char* password = "123456789";

// MQTT Broker info
String mqtt_broker;                        // IP address of the board the broker is running on (ie. master)
const char *mqtt_username = "master";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
sMQTTBroker broker;                       // broker instanciation
bool MQTTconnection = false;              // stock the MQTT connection state

WiFiClient espClient;                     // creation of a client for the broker
PubSubClient client(espClient);           // client instanciation

const byte localAddress = 0xAA;           // address of this device
const byte destinationCoast = 0xFF;       // destination of board on the coast
byte msgCount = 0;                        // count of outgoing messages
byte sender;                              // sender address

String range = "";                        // stock the RNG calculated by each buoy

int nbPing = 0, lastPing = 0;                                // current and last ping counters
int msgReceived1 = 0, msgReceived2 = 0, msgReceived3 = 0;    // variables to check if a RNG data from slaves 1/2 or master has been received

createBufferedOutput(output, 255, DROP_UNTIL_EMPTY);  // create an extra output buffer for the Serial2
createSafeStringReader(sfReader, 50, "\r\n");         // create SafeStringReader to hold messages written on Serial2
createSafeString(accoustic, 50);                      // SafeStringReader holding the accoustic signal to cast
createSafeString(msgToCoast, 2000);                   // SafeStringReader holding the final message to sent to the coast board

void setup();
void loop();

#endif