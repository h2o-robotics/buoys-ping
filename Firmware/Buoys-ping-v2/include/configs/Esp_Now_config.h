#ifndef ESP_NOW_CONFIG_H
#define ESP_NOW_CONFIG_H

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>

#define SLAVE_1_MAC {0xCC, 0xDB, 0xA7, 0x12, 0x66, 0x48}
#define SLAVE_2_MAC {0xCC, 0xDB, 0xA7, 0x12, 0x92, 0x84}
#define MASTER_MAC  {0x0C, 0xB8, 0x15, 0x75, 0xC1, 0x68}

#define REQUEST_MAX_STR_LENGTH  40
#define RESPONSE_MAX_STR_LENGTH 100

#define REQUEST_RETRY_MAX_NUM 10
#define SLEEP_BETWEEN_TRIES 2000

typedef struct {
    char request[REQUEST_MAX_STR_LENGTH];
} request_ping_t;

typedef struct {
    uint8_t slave_id;
    char response[RESPONSE_MAX_STR_LENGTH];
} response_ping_t;

bool esp_now_setup_master();
bool esp_now_setup_slave();

void esp_now_request_ping(const uint8_t *reciever_mac, request_ping_t data);
void esp_now_send_ping_data(response_ping_t data);

void esp_now_custom_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status);

#endif