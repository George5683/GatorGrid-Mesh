#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdint>
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

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


    bool toggle = true;
    
    for (;;) {

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        if(!node.is_connected()) {
            printf("Not connected!\n");
        } else {
            break;
        }
        sleep_ms(5000);
    }

    for (;;) {

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;

        sleep_ms(1000);
    }

    return 0;
}