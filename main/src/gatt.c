#include "gatt.h"
#include "gpio.h"
#include "wifi_ble.h" 
#include "cJSON.h"
#include "nvs_ble.h" 
#include "esp_wifi.h"
#include "mqtt_init_ble.h"

#define TAG "GATT"

static uint8_t gpio_state = 0;
static char received_buffer[1024] = {0};
static size_t received_length = 0;
static char* mac_str;


void handle_received_json(const char *json_string) {
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        ESP_LOGE(TAG, "Invalid JSON received!");
        return;
    }

    cJSON *password = cJSON_GetObjectItem(json, "password");
    if (password && cJSON_IsString(password)) {
        ESP_LOGI(TAG, "Setting new WiFi password.");
        set_wifi_password(password->valuestring);
        if (nvs_write_string("PASS", password->valuestring) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write WiFi password to NVS");
        }
        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    if (ssid && cJSON_IsString(ssid)) {
        ESP_LOGI(TAG, "Setting new SSID: %s", ssid->valuestring);
        set_wifi_ssid(ssid->valuestring);
        if (nvs_write_string("SSID", ssid->valuestring) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write SSID to NVS");
        }
    }

    cJSON_Delete(json);
}

static int characteristic_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            return os_mbuf_append(ctxt->om, &gpio_state, sizeof(gpio_state));

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            if (ctxt->om != NULL) {
                struct os_mbuf *om = ctxt->om;
                size_t total_len = OS_MBUF_PKTLEN(om);

                if (received_length + total_len > sizeof(received_buffer) - 1) {
                    ESP_LOGE(TAG, "Received data is too large!");
                    received_length = 0;
                    return BLE_ATT_ERR_INSUFFICIENT_RES;
                }

                os_mbuf_copydata(om, 0, total_len, received_buffer + received_length);
                received_length += total_len;
                received_buffer[received_length] = '\0';

                ESP_LOGI(TAG, "Received partial data: %s", received_buffer);

                if (strchr(received_buffer, '}') != NULL) {
                    ESP_LOGI(TAG, "Full JSON received: %s", received_buffer);
                    handle_received_json(received_buffer);
                    memset(received_buffer, 0, sizeof(received_buffer));
                    received_length = 0;
                }
                return 0;
            }
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

static int descriptor_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg) {
    const char *description = "BLE Output";
    return os_mbuf_append(ctxt->om, description, strlen(description));
}

static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GPIO_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(GPIO_CHARACTERISTIC_UUID),
                .access_cb = characteristic_access_cb,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(DESCRIPTOR_UUID),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = descriptor_access_cb,
                    },
                    { 0 }
                },
            },
            { 0 }
        },
    },
    { 0 }
};

int gatt_init(void) {
    int rc = ble_gatts_count_cfg(gatt_services);
    if (rc) {
        ESP_LOGE("GATT", "Failed to count GATT services, rc=%d", rc);
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_services);
    if (rc) {
        ESP_LOGE("GATT", "Failed to add GATT services, rc=%d", rc);
        return rc;
    }

    ESP_LOGI("GATT", "GATT initialized successfully");
    return 0;
}
