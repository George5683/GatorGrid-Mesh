#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/SPI/SPI.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

void core1_entry1() {

    stdio_init_all();

    STANode node;

    if (!node.init_sta_mode())
    {
        return;
    }

    if (!node.start_sta_mode()) {
        return;
    }

    bool toggle_led = true;
    bool wifi_connected = false;
    // Continue the loop until you connect to a wifi
    while (wifi_connected == false) {
        node.scan_for_nodes();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle_led);
        toggle_led = !toggle_led;
        
        // Check if the map if empty and if not then connect to the node on the map
        if(!node.known_nodes.empty()){
            //printf("Connecting to a Node...\n");
            
            auto entry = *node.known_nodes.begin();
            // get the id from the only node 
            uint32_t connect_id = entry.first;
            if(node.connect_to_node(connect_id)){
                wifi_connected = true;
            };
        }
        sleep_ms(5000);
    }


}

int main() {
    stdio_init_all();

    sleep_ms(10000);

    multicore_launch_core1(core1_entry1);

    for (;;) {
        sleep_ms(1000);
    }

    return 0;
}