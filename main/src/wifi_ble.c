#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi_ble.h"
#include "esp_log.h"
#include "common.h"
#include "esp_mac.h"
#include "nvs_ble.h"

#define TAG "WIFI"

char* get_mac_address() {
    static char mac_str[18];
    uint8_t mac[6];

    esp_err_t ret = esp_base_mac_addr_get(mac);
    if (ret == ESP_OK) {
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return mac_str;
    } else {
        return "00:00:00:00:00:00";
    }
}

void wifi_init_sta(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi initialized");
}

void set_wifi_ssid(const char *ssid) {
    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    memset(wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "SSID changed to: %s", ssid);
}

void set_wifi_password(const char *password) {
    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "WiFi password changed.");
}

void wifi_disconnect() {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_LOGI(TAG, "WiFi disconnected.");
}

void connect_wifi() {
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "Reconnecting to WiFi...");
}
