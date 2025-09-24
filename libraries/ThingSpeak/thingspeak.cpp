#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include <string>
#include <stdio.h>

// Replace with your WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"

// ThingSpeak settings
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define WRITE_API_KEY "219T1QBQ77Y0GOYT"   // Your Write API Key

void send_to_thingspeak(int value) {
    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Failed to create socket\n");
        return;
    }

    // Resolve server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(THINGSPEAK_PORT);
    ip4addr_aton("184.106.153.149", &server_addr.sin_addr); // api.thingspeak.com

    if (lwip_connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Failed to connect\n");
        lwip_close(sock);
        return;
    }

    // Build HTTP request
    char request[256];
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field1=%d HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             WRITE_API_KEY, value, THINGSPEAK_HOST);

    // Send request
    lwip_write(sock, request, strlen(request));

    // Close socket
    lwip_close(sock);

    printf("Sent value %d to ThingSpeak!\n", value);
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to WiFi\n");
        return -1;
    }

    printf("Connected to WiFi!\n");

    while (true) {
        send_to_thingspeak(42); // Example: send the value 42
        sleep_ms(20000);        // ThingSpeak allows 1 update every 15s
    }
}
