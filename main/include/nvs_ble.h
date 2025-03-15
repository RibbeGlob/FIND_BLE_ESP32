#ifndef NVS_BLE_H
#define NVS_BLE_H

#define NAMESPACE "NVS_STORAGE"
#define NVS_KEY_SSID      "SSID"
#define NVS_KEY_PASS      "PASS"
#define NVS_KEY_NAME      "NAME"

esp_err_t nvs_write_string(const char *key, const char *value);
esp_err_t nvs_read_string(const char *key, char *out_value, size_t max_length);

#endif