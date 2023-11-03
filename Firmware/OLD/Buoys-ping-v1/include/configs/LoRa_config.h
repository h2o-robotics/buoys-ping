#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

#include <Arduino.h>
#include <LoRa.h>
#include "SafeStringReader.h"

#define LORA_frequency 866E6                   // LoRa Frequency 
#define LORA_csPin     5                       // LoRa radio chip select
#define LORA_resetPin  4                       // LoRa radio reset
#define LORA_irqPin    27                      // change for your board; must be a hardware interrupt pin

#define LORA_MasterAddress    0xAA             // address of this device
#define LORA_CoastAddress     0xFF             // destination of board on the coast

// Initialize LORA communication
void LORA_init();

// Send a LORA message with "outgoing" data from "sourceAddress" to "destinationAddress"
void LORA_sendMessage(String& outgoing, byte sourceAddress, byte destinationAddress);

#endif