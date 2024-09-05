#ifndef SLAVE_BUOY_H
#define SLAVE_BUOY_H

#include <Arduino.h>

#include "configs/Esp_Now_config.h"

#include <SoftwareSerial.h>
#include <TinyGPS.h>

#include "SafeStringReader.h"
#include "BufferedOutput.h"


#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PMTK_SET_NMEA_UPDATE_1HZ "$PMTK220,1000*1F"
#define GPS_LOG_LOCATION (false)

// Receive a message
void callback(const uint8_t * mac, const uint8_t *incomingData, int len);

// Send a RNG message
void publishData(const char* msg);

int parseAccoustic(String inputString);

// Stock the ping response received by buoy
bool responseB();

void GPS_Manager();

void setup();
void loop();

#endif