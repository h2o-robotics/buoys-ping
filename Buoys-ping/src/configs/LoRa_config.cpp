#include "configs/LoRa_config.h"

byte msgCount = 0;                 // count of outgoing messages

void LORA_init(){
  LoRa.setPins(LORA_csPin, LORA_resetPin, LORA_irqPin);

  if (!LoRa.begin(LORA_frequency)) {           
    Serial.println("LoRa init failed. Check your connections.");
    ESP.restart();                     // if failed, restart ESP
  }

  Serial.println("LoRa init succeeded.\n");
}

void LORA_sendMessage(SafeString& outgoing, byte destination) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(LORA_localAddress);        // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}