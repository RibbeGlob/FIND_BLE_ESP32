#include "common.h"
#include "nimble/ble.h"
#include "nimble_stack.h"
#include "host/ble_hs.h"
#include "gap.h"
#include "gatt.h"

#define TAG "NIMBLE"

void ble_store_config_init(void);

void on_stack_reset(int reason) {
    ESP_LOGW(TAG, "Stack reset, reason: %d", reason);
}

void on_stack_sync(void) {
    ESP_LOGI(TAG, "Stack synchronized, initializing advertising");
    adv_init();
}

void nimble_host_config_init(void) {
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_store_config_init();
}

void nimble_host_task(void *param) {
    ESP_LOGI(TAG, "Starting NimBLE host task");
    nimble_port_run();
    vTaskDelete(NULL);
}
