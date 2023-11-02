#ifndef COASTBOARD_H
#define COASTBOARD_H

#include <Arduino.h>

#include <SPI.h>        
#include "BufferedOutput.h"
#include "SafeStringReader.h"

#include "configs/LoRa_config.h"
#include "configs/WiFi_config.h"

void setup();
void loop();

#endif