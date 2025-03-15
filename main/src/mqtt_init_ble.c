#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client_ble.h"
#include "mqtt_init_ble.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "esp_event.h"
#include "wifi_ble.h"
#include "nvs_ble.h"

#define MAX_RETRY_COUNT 10
#define RECONNECT_DELAY_MS 30000

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t client = NULL;
static void (*message_callback)(const char *topic, const char *message) = NULL;
static char mqtt_password[64]; 
static char mqtt_username[64];
static char mqtt_broker_ip[20];
static char uri[70];
static int retry_count = 0;
static esp_err_t connected;
static char* mac_str;

void mqtt_register_callback(void (*callback)(const char *topic, const char *message)) {
    message_callback = callback;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    char topic_connected[64];
    char topic_subscribe[64];
    snprintf(topic_connected, sizeof(topic_connected), "ble/scanner/%s/connected", mac_str);
    snprintf(topic_subscribe, sizeof(topic_subscribe), "ble/scanner/%s/#", mac_str);

    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_publish(client, topic_connected, "online", 0, 1, 0);
            esp_mqtt_client_subscribe(client, topic_subscribe, 1);
            retry_count = 0;
            connected = ESP_OK;
            break;

        case MQTT_EVENT_DISCONNECTED:
            connected = ESP_FAIL;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            if (++retry_count >= MAX_RETRY_COUNT) {
                vTaskDelay(RECONNECT_DELAY_MS / portTICK_PERIOD_MS);
                esp_mqtt_client_reconnect(client);
            } else {
                esp_mqtt_client_reconnect(client);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        case MQTT_EVENT_DATA:
            if (message_callback != NULL) {
                char topic[event->topic_len + 1];
                strncpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';

                char message[event->data_len + 1];
                strncpy(message, event->data, event->data_len);
                message[event->data_len] = '\0';
                message_callback(topic, message);
            }
            break;
        
        default:
            break;
    }
}

esp_err_t mqtt_start(void) {
    if (client == NULL) {
        mac_str = get_mac_address();
        retry_count = 0;
        ESP_LOGI(TAG, "STARTING MQTT");
        nvs_read_string("MQTT_PASSWORD", mqtt_password, sizeof(mqtt_password));
        nvs_read_string("MQTT_IP", mqtt_broker_ip, sizeof(mqtt_broker_ip));
        nvs_read_string("MQTT_USERNAME", mqtt_username, sizeof(mqtt_username));
        snprintf(uri, sizeof(uri), "mqtt://%s:1883", mqtt_broker_ip);

        esp_mqtt_client_config_t mqtt_cfg = {
            .broker = {
                .address.uri = uri
            },
            .credentials = {
                .username = mqtt_username,
                .authentication.password = mqtt_password
            },
            .network = {
                .reconnect_timeout_ms = 5000
            },
            .session = {
                .keepalive = 10
            }
        };
        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
        if (esp_mqtt_client_start(client) == ESP_ERR_INVALID_ARG) {
            ESP_LOGI(TAG, "MQTT client start failed");
        }
    }
    vTaskDelay(100);
    ESP_LOGI(TAG, "MQTT client started");
    return connected;
}

void mqtt_send_message(const char *topic, const char *message) {
    if (client != NULL) {
        int msg_id = esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
        ESP_LOGI(TAG, "Message sent on topic: %s, msg_id: %d", topic, msg_id);
    } else {
        ESP_LOGW(TAG, "MQTT client not initialized. Cannot send message.");
    }
}

esp_err_t reinit_mqtt_task() {
    if (!client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return mqtt_start();
    }
    ESP_LOGI(TAG, "Reinitializing MQTT client");
    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
    client = NULL;
    return mqtt_start();
}

void reinit_task(void *param) {
    SemaphoreHandle_t sem = (SemaphoreHandle_t)param;
    esp_err_t err = reinit_mqtt_task();
    xSemaphoreGive(sem);
    vTaskDelete(NULL);
}

esp_err_t reinit_mqtt_with_new_credentials() {
    TaskHandle_t reinitTaskHandle = NULL;
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    if (sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return ESP_FAIL;
    }
    
    if (xTaskCreate(reinit_task, "reinit_task", 4096, (void*)sem, 5, &reinitTaskHandle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create reinit task");
        vSemaphoreDelete(sem);
        return ESP_FAIL;
    }
    
    if (xSemaphoreTake(sem, pdMS_TO_TICKS(4000)) == pdTRUE) {
        vSemaphoreDelete(sem);
        return ESP_OK;
    } else {
        if (eTaskGetState(reinitTaskHandle) != eDeleted) {
            ESP_LOGW(TAG, "Reinit task did not finish in time");
            vTaskDelete(reinitTaskHandle);
        }
        vSemaphoreDelete(sem);
        return ESP_FAIL;
    }
}