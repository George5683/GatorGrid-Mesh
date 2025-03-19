#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/SPI/SPI.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

void core1_entry() {
    //stdio_init_all(); // Initialize stdio only once
    
    STANode node;

    // Initialize STA mode
    if (!node.init_sta_mode()) {
        printf("Failed to initialize STA mode\n");
        return;
    }

    // Start STA mode
    if (!node.start_sta_mode()) {
        printf("Failed to start STA mode\n");
        return;
    }

    bool toggle_led = true;
    bool wifi_connected = false;

    // Continue the loop until connected to a node
    while (!wifi_connected) {
        // Scan for nearby nodes
        if (!node.scan_for_nodes()) {
            printf("Scan failed, retrying...\n");
            sleep_ms(5000);
            continue;
        }

        // Toggle LED to indicate scanning
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle_led);
        toggle_led = !toggle_led;

        // Check if any nodes are found
        if (!node.known_nodes.empty()) {
            // Connect to the first node in the map
            auto entry = *node.known_nodes.begin();
            uint32_t connect_id = entry.first;

            printf("Attempting to connect to node: %u\n", connect_id);
            if (node.connect_to_node(connect_id)) {
                wifi_connected = true;
                printf("Connected to node: %u\n", connect_id);

                // Send a message to the connected node
                std::string message = "Hello from GatorGrid node!";
                if (node.send_string_data(message.c_str())) {
                    printf("Data sent successfully!\n");
                } else {
                    printf("Failed to send data\n");
                }
            } else {
                printf("Failed to connect to node: %u\n", connect_id);
            }
        }

        sleep_ms(5000); // Wait before retrying
    }

    // Keep the LED on to indicate successful connection
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

int main() {
    stdio_init_all(); // Initialize stdio only once

    // Wait for 10 seconds (optional)
    sleep_ms(10000);

    // Launch core1_entry on Core 1
    multicore_launch_core1(core1_entry);

    // Core 0 main loop
    for (;;) {
        sleep_ms(1000); // Core 0 can perform other tasks here
    }

    return 0;
}