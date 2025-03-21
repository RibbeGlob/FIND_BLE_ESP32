#include "common.h"
#include "gap.h"
#include "gpio.h"
#include "gatt.h"
#include "nimble_stack.h"
#include "esp_log.h"
#include "mqtt_client_ble.h"
#include "mqtt_init_ble.h"
#include "nvs_ble.h"
#include "server_ble.h"
#include "udp_server.h"
#include "wifi_ble.h"

#define TAG "BLE_APP"

void app_main(void) {
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(nimble_port_init());
    ESP_ERROR_CHECK(gap_init());
    nimble_host_config_init();
    ESP_ERROR_CHECK(gatt_init());
    xTaskCreate(nimble_host_task, "NimBLE Host", 4 * 1024, NULL, 5, NULL);
    wifi_init_sta();
    start_webserver();
    start_udp_server();
    mqtt_start();
    initialize_mqtt_client();

}