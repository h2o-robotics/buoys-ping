#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <Arduino.h>
#include <LoRa.h>
#include "SafeStringReader.h"

#define LORA_frequency 866E6                   // LoRa Frequency 
#define LORA_csPin     5                       // LoRa radio chip select
#define LORA_resetPin  4                       // LoRa radio reset
#define LORA_irqPin    27                      // change for your board; must be a hardware interrupt pin

#define LORA_localAddress     0xAA             // address of this device
#define LORA_destinationCoast 0xFF             // destination of board on the coast

void LORA_init();
void LORA_sendMessage(SafeString& outgoing, byte destination);

#endif