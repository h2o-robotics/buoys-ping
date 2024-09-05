#include "configs/Esp_Now_config.h"

uint8_t *sending_data;
uint8_t sending_data_size;
uint8_t sending_address[6];

esp_now_peer_info_t peerInfo;

void esp_now_custom_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    static int repetition_counter = 0;

    Serial.print("[ESP_NOW] Last Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

    if(status == ESP_NOW_SEND_SUCCESS){
        repetition_counter = 0;
    } else {
        repetition_counter += 1;

        if(repetition_counter < REQUEST_RETRY_MAX_NUM){
            Serial.println("[ESP_NOW] Trying to send again...\n");

            delay(SLEEP_BETWEEN_TRIES);

            esp_now_send(
                sending_address, 
                sending_data, 
                sending_data_size
            );
        } else {
            Serial.println("[ESP_NOW] Max number of retries reached!\n");
            repetition_counter = 0;
        }
    }
}

bool esp_now_basic_setup(esp_now_recv_cb_t recv_callback_f) {
    // Set ESP32 as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    
    // Initilize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP_NOW] Error initializing ESP-NOW");
        return false;
    }
    
    // Register the send and recieve callback
    esp_now_register_send_cb(esp_now_custom_send_callback);
    esp_now_register_recv_cb(recv_callback_f);

    // Allocate the buffer for sending data
    *sending_data = 0;

    return true;
}

bool esp_now_setup_master() {
    uint8_t addr_peer_1[] = SLAVE_1_MAC;
    uint8_t addr_peer_2[] = SLAVE_2_MAC;

    // Register slave #1
    memcpy(peerInfo.peer_addr, addr_peer_1, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
        
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("[ESP_NOW] Failed to add SLAVE 1");
        return false;
    }

    
    // Register slave #2
    memcpy(peerInfo.peer_addr, addr_peer_2, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
     
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("[ESP_NOW] Failed to add SLAVE 2");
        return false;
    }

    return true;
}

bool esp_now_setup_slave() {
    uint8_t addr_peer[] = MASTER_MAC;
    
    // Register master
    memcpy(peerInfo.peer_addr, addr_peer, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    esp_err_t ret = esp_now_add_peer(&peerInfo);
    if (ret != ESP_OK){
        Serial.printf("[ESP_NOW] Failed to add MASTER - %s\n", esp_err_to_name(ret));
        return false;
    }

    return true;
}

void esp_now_request_ping(const uint8_t *reciever_mac, request_ping_t data) {
    sending_data_size = sizeof(data);
    
    free(sending_data);
    sending_data = (uint8_t*) malloc(sending_data_size);
    memcpy(sending_data, &data, sending_data_size);
    
    memcpy(sending_address, reciever_mac, 6);

    esp_now_send(
        sending_address, 
        sending_data, 
        sending_data_size
    );
}

void esp_now_send_ping_data(response_ping_t data) {
    sending_data_size = sizeof(data);
    
    free(sending_data);
    sending_data = (uint8_t*) malloc(sending_data_size);
    memcpy(sending_data, &data, sending_data_size);
    
    uint8_t address[] = MASTER_MAC;
    memcpy(sending_address, address, 6);

    esp_now_send(
        sending_address, 
        sending_data, 
        sending_data_size
    );
}