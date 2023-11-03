#ifndef SLAVE_BUOY_H
#define SLAVE_BUOY_H

#include <Arduino.h>

#include "configs/WiFi_config.h"
#include "configs/MQTT_config.h"

#include <SoftwareSerial.h>
#include <TinyGPS.h>

#include "SafeStringReader.h"
#include "BufferedOutput.h"


#define MQTT_BROKER "192.168.4.1"    // MQTT Broker info: has to be changed depending on the board the broker is running on

#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

void setup();
void loop();

#endif