#ifndef SLAVE_BUOY_H
#define SLAVE_BUOY_H

// Include libraries
#include <Arduino.h>
#include <SPI.h>              
#include <PubSubClient.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"

#include "configs/WiFi_config.h"
#include "configs/MQTT_config.h"

void setup();
void loop();

#endif