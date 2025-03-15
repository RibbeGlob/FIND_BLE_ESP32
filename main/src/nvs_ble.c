#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "nvs_ble.h"

#define TAG "NVS"

esp_err_t nvs_write_string(const char *key, const char *value)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS key %s: %s", key, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing string for key %s: %s", key, esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing changes to key %s: %s", key, esp_err_to_name(err));
    }
    nvs_close(handle);
    return err;
}


esp_err_t nvs_read_string(const char *key, char *out_value, size_t max_length)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS key %s: %s", key, esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(handle, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        if (max_length > 0) {
            out_value[0] = '\0';
        }
        nvs_close(handle);
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting size for key %s: %s", key, esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    if (required_size > max_length) {
        ESP_LOGE(TAG, "Error getting size for key %s: expected %d, available %d", key, required_size, max_length);
        nvs_close(handle);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    err = nvs_get_str(handle, key, out_value, &max_length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading string for key %s: %s", key, esp_err_to_name(err));
    }
    nvs_close(handle);
    return err;
}
