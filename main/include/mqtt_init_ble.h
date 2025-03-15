#ifndef MQTT_INIT_BLE_H
#define MQTT_INIT_BLE_H
#include "mqtt_client.h"
#include <stdbool.h>

esp_err_t mqtt_start(void);
void mqtt_register_callback(void (*callback)(const char *topic, const char *message));
void mqtt_send_message(const char *topic, const char *message);
esp_err_t reinit_mqtt_with_new_credentials();
#endif
