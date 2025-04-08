#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"

int main() {
    // Initialize stdio for debugging
    stdio_init_all();
    
    // Create the APNode object
    APNode node;
    //STANode node;

    //node.set_node_id(2);

    sleep_ms(5000);
        /*
    printf("Starting STA Mode\n");
    node.init_sta_mode();
    int err;
    err = node.connect_to_AP(ssid, pass, 10000);
    
    if(0 < err)
    {
        while(true) {
            printf("Wifi Error: %d\n", err);
            sleep_ms(1000);
            break;
        }
    }

    printf("Connected! Running Client Test\n");

    node.init_tcp();
    node.client_test();
    */
    

        
    // Initialize hardware and resources
    if (!node.init_ap_mode()) {
        printf("Failed to initialize MeshNode\n");
        return 1;
    }

    //node.set_ap_credentials(NULL, pass);
    
    // Configure AP (optional - uses defaults if not set)
    // defaults are below
    //node.set_ap_credentials("mesh_network", "password123");

    // Start AP mode
    if (!node.start_ap_mode()) {
        while(true) {
            printf("Failed to start AP mode\n");
            sleep_ms(1000);
        }
    }

    sleep_ms(5000);

    printf("Starting server test\n");

    node.server_start();

    while(node.server_running()) {
        printf("Server running, doing stuff...\n");
    }

    sleep_ms(5000);
    
    printf("MeshNode AP started with ID: %d\n", node.get_node_id());
    // Main loop
    while (true) {
        printf("Server over.\n");
        sleep_ms(1000);
        // Poll for network events
        //node.poll(1000);  // Poll with 1000ms timeout
    }


    return 0;
}
