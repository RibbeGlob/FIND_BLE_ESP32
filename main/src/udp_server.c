#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "udp_server.h"
#include "esp_err.h"

#define PORT 5109

static TaskHandle_t udp_server_task_handle = NULL;
static const char *TAG = "udp_server";
const char* WHERE_ARE_YOU = "WHERE_ARE_YOU";
const char* IM_HERE = "IM_HERE";
char rx_buffer[128];
char addr_str[128];

static void udp_server_task(void *pvParameters)
{
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
    
    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        udp_server_task_handle = NULL;
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        ESP_LOGE(TAG, "Unable to set socket timeout: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        udp_server_task_handle = NULL;
        return;
    }

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        udp_server_task_handle = NULL;
        return;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);

    while (1) {
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        if (len < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            } else {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
        } else {
            if (source_addr.ss_family == PF_INET) {
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            }

            rx_buffer[len] = 0;
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
            ESP_LOGI(TAG, "%s", rx_buffer);

            if (strncmp(rx_buffer, WHERE_ARE_YOU, strlen(WHERE_ARE_YOU)) == 0) {
                struct sockaddr_in *source_addr_ip4 = (struct sockaddr_in *)&source_addr;
                source_addr_ip4->sin_port = htons(PORT);
                int err = sendto(sock, IM_HERE, strlen(IM_HERE), 0, (struct sockaddr *)source_addr_ip4, sizeof(struct sockaddr_in));
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                } else {
                    ESP_LOGI(TAG, "WHERE_ARE_YOU received, sent IM_HERE to %s:%d", addr_str, PORT);
                }
            }
        }
    }

    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
    }
    udp_server_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t start_udp_server(void) {
    if (udp_server_task_handle != NULL) {
        ESP_LOGI(TAG, "udp_server_task already running");
        return ESP_OK;
    }

    if (xTaskCreate(udp_server_task, "udp_server", 4096, (void*)AF_INET, 5, &udp_server_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create udp_server_task");
        return ESP_FAIL;
    }
    return ESP_OK;
}
