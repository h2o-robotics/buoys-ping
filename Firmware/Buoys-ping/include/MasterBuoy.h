#ifndef MASTER_BUOY_H
#define MASTER_BUOY_H

#include <Arduino.h>

#include "configs/LoRa_config.h"
#include "configs/WiFi_config.h"
#include "configs/MQTT_config.h"

#include "SafeStringReader.h"
#include "BufferedOutput.h"

// Receive a message from the coast board
void onReceive_LoRa(int packetSize);

void setup();
void loop();

#endif