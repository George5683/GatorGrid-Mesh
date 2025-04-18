#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"


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
    if (node.connect_to_node(0)) {
        node.tcp_init();
    }

    sleep_ms(1000);

    for (int i = 0; i < 10; i++)
    {
        TCP_DATA_MSG msg(node.get_NodeID(), 2);
        uint8_t arr[] = "hello, this is message:  ";
        arr[24] = 48 + i;
        msg.add_message(25, arr);
        while (!node.send_tcp_data_blocking(msg.get_msg(), msg.get_len(), false));
    }


    bool toggle = true;
    for (;;) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        if(!node.is_connected()) {
            printf("Not connected!\n");
        }
        sleep_ms(5000);
    }

    return 0;
}