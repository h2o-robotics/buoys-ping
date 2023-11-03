#ifndef MASTER_BUOY_H
#define MASTER_BUOY_H

#include <Arduino.h>

#include "configs/LoRa_config.h"
#include "configs/WiFi_config.h"
#include "configs/MQTT_config.h"

#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"

#define MEASURETIMEOUT 15000         // Timeout for Range Measurement Sequency
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

void setup();
void loop();

#endif