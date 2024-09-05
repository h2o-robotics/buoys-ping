#ifndef MASTER_BUOY_H
#define MASTER_BUOY_H

#include <Arduino.h>

#include "configs/LoRa_config.h"
#include "configs/Esp_Now_config.h"

#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include "SafeStringReader.h"
#include "BufferedOutput.h"

#define MEASURETIMEOUT 25000         // Timeout for Range Measurement Sequency

#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PMTK_SET_NMEA_UPDATE_1HZ "$PMTK220,1000*1F"
#define GPS_LOG_LOCATION (false)

void onReceive(int packetSize);

void publishData(SafeString& msg, int slave);
void callback(const uint8_t * mac, const uint8_t *incomingData, int len);

void reInit_esp_now();

int parseAccoustic(String inputString);
bool responseB1();
bool isTabFilled();
void emptyTab();

void masterBuoyLoop(void * parameter);
void GPS_Manager();

void setup();
void loop();

#endif