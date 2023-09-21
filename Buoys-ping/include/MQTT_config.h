#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <sMQTTBroker.h>

#define MQTT_password "public"
#define MQTT_port     1883

#ifdef MASTER_BUOY
    #define MQTT_username "master"
#elif SLAVE_BUOY
    #ifndef SLAVE_ID
        #error "Slave ID not defined!"
    #endif

    #define STR(ID) #ID
    #define MQTT_username "slave" STR(SLAVE_ID)
#endif

#endif