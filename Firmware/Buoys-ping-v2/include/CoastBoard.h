#ifndef COASTBOARD_H
#define COASTBOARD_H

#include <Arduino.h>

#include <SPI.h>        
#include "BufferedOutput.h"
#include "SafeStringReader.h"

#include "configs/LoRa_config.h"

#define AUTOMATIC_MEASUREMENT true

// Empty the RNGdata tab 
void emptyTab(String tab[]) ;
// Split a message in several, put them in a tab and recover the size of the tab
int split(String tab[], char *str, const char * delimiter);

// Sort the RNG data message received from the master so it always shows data in the same order
String sortRNGmessage(String messsage);

// Receive a message from master buoy
void onReceive(int packetSize);
boolean runEvery(unsigned long interval);

void setup();
void loop();

#endif