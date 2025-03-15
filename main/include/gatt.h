#ifndef GATT_H
#define GATT_H

#include "common.h"
#include "services/gatt/ble_svc_gatt.h"

#define GPIO_SERVICE_UUID          0x1234
#define GPIO_CHARACTERISTIC_UUID   0x5678
#define DESCRIPTOR_UUID            0x2901

int gatt_init(void);

#endif
