// (c) 2018-2024 HomeAccessoryKid

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/socket.h>
#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <udplogger.h>

char udplogstring[2900]={0}; //in the end I do not know to prevent overflow, so I use the max size of 2 UDP packets ??
int  udplogstring_len=0;

static bool netif_has_ip(esp_netif_t *netif, void *ctx) {
    esp_netif_ip_info_t ip;
    return esp_netif_is_netif_up(netif) &&
           esp_netif_get_ip_info(netif, &ip) == ESP_OK &&
           ip.ip.addr != 0;
}

static void udplog_send(void *pvParameters){
    int udp_socket, wait = 0;
    struct sockaddr_in local_addr, dest_addr;

//     while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) vTaskDelay(20); //Check if we have an IP every 200ms
    esp_netif_t* esp_netif = NULL;
    esp_netif_ip_info_t info;
    do {
        esp_netif = esp_netif_find_if(netif_has_ip, NULL);
        if (esp_netif) {
            esp_netif_get_ip_info(esp_netif, &info);
        }
        if (!esp_netif || info.ip.addr == 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    } while (!esp_netif || info.ip.addr == 0);

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&local_addr, 0, sizeof(local_addr));
    memset(&dest_addr,  0, sizeof(dest_addr));
    /*Destination*/
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    dest_addr.sin_port = 28338; //= 45678; //reversed bytes
    /*Source*/
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = 40109; //= 44444; //reversed bytes
    bind(udp_socket, (struct sockaddr *)&local_addr, sizeof(local_addr));

    while (1) {
        if ((!wait && udplogstring_len) || udplogstring_len > 700) {
            sendto(udp_socket, udplogstring, udplogstring_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            udplogstring_len = 0;
            wait = 10;
        }
        if (!wait) {
            wait = 10; // send output every 100ms
        }
        wait--;
        vTaskDelay(pdMS_TO_TICKS(10)); //with len>1000 and delay=10ms, we might handle 800kbps throughput
    }
}

void udplogger_init(int prio) {
    xTaskCreate(udplog_send, "logsend", 4096, NULL, prio, NULL);
}