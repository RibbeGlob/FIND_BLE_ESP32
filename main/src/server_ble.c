#include "server_ble.h"
// #include "version.h"
#include "esp_log.h"
#include <esp_err.h> 
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_http_server.h>
#include "esp_system.h"
#include "esp_mac.h"
#include "wifi_ble.h"
#include "cJSON.h"
#include "mqtt_init_ble.h"
#include "nvs_ble.h"

static const char *TAG = "server";
static httpd_handle_t server = NULL;

static esp_err_t info_get_handler(httpd_req_t *req) {
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    char* mac_str = get_mac_address();
    char resp_str[256];
    snprintf(resp_str, sizeof(resp_str),
         "{\"devId\": \"%s\","
         "\"hwVersion\": \"0\","
         "\"model\": 70,"
         "\"name\": \"BLE\","
         "\"fwVersion\": 0.0.1}",
         mac_str);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t set_find_handler(httpd_req_t *req) {
    char content[200] = {0};
    if (httpd_req_recv(req, content, sizeof(content) - 1) <= 0) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Received content: %s", content);

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON structure");
        return ESP_FAIL;
    }

    cJSON *ip_item = cJSON_GetObjectItemCaseSensitive(json, "ip");
    cJSON *mqtt_item = cJSON_GetObjectItemCaseSensitive(json, "mqtt");
    cJSON *mqtt_user = mqtt_item ? cJSON_GetObjectItemCaseSensitive(mqtt_item, "user") : NULL;
    cJSON *mqtt_pass = mqtt_item ? cJSON_GetObjectItemCaseSensitive(mqtt_item, "pass") : NULL;
    
    if (cJSON_IsString(ip_item) && ip_item->valuestring &&
        cJSON_IsString(mqtt_user) && mqtt_user->valuestring &&
        cJSON_IsString(mqtt_pass) && mqtt_pass->valuestring) {
        
        ESP_LOGI(TAG, "MQTT IP: %s", ip_item->valuestring);
        ESP_LOGI(TAG, "MQTT User: %s", mqtt_user->valuestring);
        ESP_LOGI(TAG, "MQTT Pass: %s", mqtt_pass->valuestring);
        nvs_write_string("MQTT_IP", ip_item->valuestring);
        nvs_write_string("MQTT_USERNAME", mqtt_user->valuestring);
        nvs_write_string("MQTT_PASSWORD", mqtt_pass->valuestring);
        httpd_resp_set_type(req, "application/json");
        cJSON_Delete(json);
        if (reinit_mqtt_with_new_credentials() == ESP_OK){
            httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to reinitialize MQTT client");
            return ESP_FAIL;
        }
    }
    
    cJSON_Delete(json);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON structure");
    return ESP_FAIL;
}

static const httpd_uri_t info = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = info_get_handler
};

static const httpd_uri_t pairing = {
    .uri = "/pairing",
    .method = HTTP_POST,
    .handler = set_find_handler
};

esp_err_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server listening on port %d", config.server_port);
        httpd_uri_t info_uri = info;
        httpd_register_uri_handler(server, &info_uri);
        httpd_register_uri_handler(server, &pairing);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Error starting server");
        return ESP_FAIL;
    }
}
