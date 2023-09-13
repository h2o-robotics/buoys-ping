#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <Arduino.h>
#include <LoRa.h>

const long frequency = 866E6;               // LoRa Frequency 
const int csPin = 5;                        // LoRa radio chip select
const int resetPin = 4;                     // LoRa radio reset
const int irqPin = 27;                      // change for your board; must be a hardware interrupt pin

void LoRaInit();

#endif