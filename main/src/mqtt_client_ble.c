#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_init_ble.h"
#include "mqtt_client_ble.h"
#include "wifi_ble.h"
#include "nvs_ble.h" 
#include "gpio.h"

#define TAG "MQTT_CLIENT"

static char* mac_str;
static uint8_t duration;
static bool offline;
static char topic_template[64];
static char nvs_value[64];

static void subscribe_mqtt_client(const char* topic, const char* message) {

    snprintf(topic_template, sizeof(topic_template), "ble/scanner/%s/value/get", mac_str);
    if (strncmp(topic, topic_template, strlen(topic_template)) == 0) {
        snprintf(topic_template, sizeof(topic_template), "ble/scanner/%s/value/state", mac_str);
        mqtt_send_message(topic_template, "0");
        ESP_LOGI(TAG, "Received GET request for scanner value");
        return;
    } 

    ESP_LOGW(TAG, "Unknown command on topic: %s", topic);
    return;
}

void initialize_mqtt_client(void) {
    mqtt_register_callback(subscribe_mqtt_client);
    mac_str = get_mac_address();
    ESP_LOGI(TAG, "MQTT Client Initialized");
}
