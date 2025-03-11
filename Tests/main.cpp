#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

void core1_entry() {

    stdio_init_all();

    STANode node;

    if (!node.init_sta_mode())
    {
        return;
    }

    if (!node.start_sta_mode()) {
        return;
    }

    bool toggle = true;
    for (;;) {
        node.scan_for_nodes();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        printf("core print...");
        sleep_ms(5000);
    }


}

int main() {
    stdio_init_all();
    
    // Initial delay for serial monitor
    sleep_ms(10000);
    
    STANode Node;
    if (!Node.init_sta_mode()) {
        printf("Failed to initialize STA mode\n");
        return -1;
    }
    
    if (!Node.start_sta_mode()) {
        printf("Failed to start STA mode\n");
        return -1;
    }
    
    bool scan_ok = true;
    for (;;) {
        if (scan_ok) {
            printf("Starting WiFi scan...\n");
            scan_ok = Node.scan_for_nodes();
            if (!scan_ok) {
                printf("Scan failed, will retry after longer delay\n");
                sleep_ms(10000); // Longer recovery delay
                scan_ok = true;  // Reset to try again
                continue;
            }
        }
        
        // Allow some time between scans (at least 5 seconds)
        printf("Scan complete. Waiting before next scan...\n");
        sleep_ms(5000);
    }
    
    return 0;
}