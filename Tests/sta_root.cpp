#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdint>
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "display.hpp"

#define DEBUG 1


int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);

    // initiate everything


    //SPI spi;
    STANode node;
    node.init_sta_mode();
    node.start_sta_mode();
    node.is_root = true;
    node.state = (TCP_CLIENT_T*)calloc(1, sizeof(TCP_CLIENT_T));

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    
    // if (!node.scan_for_nodes()) {
    //     return 0;
    // }
    // printf("known nodes map size: %d\n", node.known_nodes.size());
    // while (node.known_nodes.empty()) {
    //     //printf("known nodes map size: %d", node.known_nodes.size());
    //     if (!node.scan_for_nodes()) {

    //         return 0;
    //     }
    // }
    // printf("Left searching for nodes\n");
    // if (node.connect_to_node(1)) {
    //     node.tcp_init();
    // }

    // sleep_ms(1000);


    bool got_an_ack = false;
    int count = 0;
    int final_size = 0;

    char send_buf[50] = {0};

    bool toggle = true;


    uint32_t children_ids[4] = {};
    uint8_t number_of_children = 0;

    char hullo[] = {"Hello from the root!"};
    
    for (;;) {

        DEBUG_printf("Loop start\n");

        // Mark before poll
        DEBUG_printf("Before poll\n");
        node.poll();
        // Mark after poll
        DEBUG_printf("After poll\n");

        if (count++ >= 1000) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;
            count = 0;
        }

        sleep_ms(1);
    }

    return 0;
}