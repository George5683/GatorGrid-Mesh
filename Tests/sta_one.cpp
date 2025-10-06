#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "libraries/RingBuffer/RingBuffer.hpp"

#include "libraries/display/display.hpp"

#define DEBUG 1

int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor

    STANode node;

    
    sleep_ms(1000);

    // initiate everything


    //SPI spi;
    
    node.init_sta_mode();
    node.start_sta_mode();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    
    if (!node.scan_for_nodes()) {
        return 0;
    }
    printf("known nodes map size: %d\n", node.known_nodes.size());
    while (node.known_nodes.empty()) {
        //printf("known nodes map size: %d", node.known_nodes.size());
        if (!node.scan_for_nodes()) {

            return 0;
        }
    }
    printf("Left searching for nodes\n");

    while(!node.connect_to_network());
    if (!node.tcp_init()) {
        // Failed to init TCP connection
        while(true);
    }


    bool got_an_ack = false;
    int count = 0;
    int final_size = 0;

    char send_buf[50] = {0};

    bool toggle = true;


    uint32_t children_ids[4] = {};
    uint8_t number_of_children = 0;

    for (;;) {
        node.poll();

        if (count++ >= 500) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;
            if(!node.is_connected()) {
                printf("Not connected!\n");
            }

            node.tree.get_children(node.get_NodeID(), children_ids, number_of_children);
            printf("\n\nChildren:\n");
            for (int i = 0; i < number_of_children; i++) {
                printf("%u\t", children_ids[i]);
            }
            printf("\n");

            count = 0;
        }

        

        sleep_ms(1);
    }
    

    return 0;
}