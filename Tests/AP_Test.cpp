#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/SPI/SPI.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

// Global variables for inter-core communication
APNode* global_node = nullptr;
bool node_ready = false;
bool is_wifi_connected = false;
uint32_t node_id = 0;

// Mutex for safe access to shared resources
mutex_t node_mutex;

// Core1 entry point - handles SPI communication
void core1_entry() {
    printf("Core1 starting (SPI operations)...\n");
    
    // Initialize the SPI on core1
    SPI Master_Pico;
    if (!Master_Pico.SPI_init(true)) {
        printf("SPI initialization failed on core1\n");
        return;
    }
    printf("SPI initialized on core1\n");
    
    // Wait for the node to be ready before starting SPI communications
    while (!node_ready) {
        sleep_ms(100);
    }
    
    for (;;) {
        // Create a buffer for SPI communication
        uint8_t send_string[2048] = {0};
        
        // Get the current node ID (safely)
        // mutex_enter_blocking(&node_mutex);
        uint32_t current_node_id = global_node->get_node_id();
        // mutex_exit(&node_mutex);
        
        // Format the send_string with the ID
        snprintf((char*)send_string, sizeof(send_string), "ID:%lu", current_node_id);
        printf("Core1: Formatted message: %s\n", send_string);
        
        // Send the message over SPI
        printf("Core1: Sending SPI message...\n");
        Master_Pico.SPI_send_message(send_string, strlen((char*)send_string));
        
        // Get the response
        uint8_t recv_buffer[2048] = {0};
        Master_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
        //printf("Core1: SPI response: %s\n", recv_buffer);
        
        sleep_ms(5000);  // Wait 5 seconds between iterations
        break;
    }
}

int main() {
    // Initialize everything
    stdio_init_all();
    mutex_init(&node_mutex);
    
    // Initial delay to allow user to connect to the serial monitor
    sleep_ms(10000);
    printf("Main core (Core0) starting...\n");
    
    // Launch core1 to handle SPI communication
    printf("Launching core1 for SPI operations...\n");
    multicore_launch_core1(core1_entry);
    
    // Initialize AP node on core0
    printf("Initializing AP node on core0...\n");
    APNode local_node;
    global_node = &local_node;
    
    if (!global_node->init_ap_mode()) {
        printf("Failed to initialize AP mode\n");
        return -1;
    }
    
    if (!global_node->start_ap_mode()) {
        printf("Failed to start AP mode\n");
        return -1;
    }
    
    printf("AP mode initialized successfully on core0\n");
    
    // Update the node ID
    // mutex_enter_blocking(&node_mutex);
    // node_id = global_node->get_node_id();
    // mutex_exit(&node_mutex);
    
    // Signal that the node is ready
    node_ready = true;
    
    // Main loop for core0
    bool toggle = true;
    for (;;) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        
        sleep_ms(2000);  // LED and status update interval
    }
    
    return 0;
}