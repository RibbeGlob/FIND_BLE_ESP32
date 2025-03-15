#include "gatt.h"
#include "gpio.h"

#define TAG "GATT"

static uint8_t gpio_state = 0;

static int characteristic_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            return os_mbuf_append(ctxt->om, &gpio_state, sizeof(gpio_state));

        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            if (ctxt->om != NULL) {
                struct os_mbuf *om = ctxt->om;
                char received_data[512] = {0};
                size_t total_len = 0;

                while (om != NULL) {
                    size_t chunk_len = OS_MBUF_PKTLEN(om);
                    if (total_len + chunk_len > sizeof(received_data) - 1) {
                        ESP_LOGE(TAG, "Data too large to process");
                        return BLE_ATT_ERR_INSUFFICIENT_RES;
                    }
                    os_mbuf_copydata(om, 0, chunk_len, received_data + total_len);
                    total_len += chunk_len;
                    om = SLIST_NEXT(om, om_next);
                }

                received_data[total_len] = '\0';
                ESP_LOGI(TAG, "Received data: %s", received_data);
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
