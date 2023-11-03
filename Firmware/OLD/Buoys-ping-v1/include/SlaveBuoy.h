#ifndef SLAVE_BUOY_H
#define SLAVE_BUOY_H

#include <Arduino.h>

#include "configs/WiFi_config.h"
#include "configs/MQTT_config.h"

#include "SafeStringReader.h"
#include "BufferedOutput.h"

void setup();
void loop();

#endif