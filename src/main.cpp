#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include <cstdio>

int main() {
    // Initialize stdio for debugging
    stdio_init_all();
    
    // Create the MeshNode object
    MeshNode node;
    
    // Initialize hardware and resources
    if (!node.init_ap_mode()) {
        printf("Failed to initialize MeshNode\n");
        return 1;
    }
    
    // Configure AP (optional - uses defaults if not set)
    node.set_ap_credentials("mesh_network", "password123");
    
    // Start AP mode
    if (!node.start_ap_mode()) {
        printf("Failed to start AP mode\n");
        return 1;
    }
    
    printf("MeshNode AP started with ID: %d\n", node.get_node_id());
    
    // Main loop
    while (true) {
        // Poll for network events
        node.poll(1000);  // Poll with 1000ms timeout
    }
    
    return 0;
}