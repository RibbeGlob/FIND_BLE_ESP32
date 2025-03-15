#ifndef WIFI_BLE_H
#define WIFI_BLE_H

void wifi_init_sta(void);
void set_wifi_ssid(const char *ssid);
void set_wifi_password(const char *password);
void wifi_disconnect();
void connect_wifi();
char* get_mac_address();

#endif
