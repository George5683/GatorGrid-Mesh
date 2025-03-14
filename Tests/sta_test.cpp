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
        if (!node.scan_for_nodes()) {
            return;
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        printf("core print...");
        for (auto it = node.known_nodes.begin(); it != node.known_nodes.end(); it++)
        {
            printf("Knows node: %d\n", it->first);
        }
        sleep_ms(5000);
    }


}

int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);

    // initiate everything
    

    // multicore_launch_core1(core1_entry);

    STANode node;
    node.init_sta_mode();
    node.start_sta_mode();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);


    while (node.known_nodes.size() <= 0) {
        if (!node.scan_for_nodes()) {
            return 0;
        }
    }

    if (node.connect_to_node(node.known_nodes.begin()->first)) {
        node.tcp_init();
    }

    sleep_ms(1000);

    for (int i = 0; i < 10; i++)
    {
        uint8_t* data;
        data = new uint8_t[2048];
        data[0] = 0x7F;
        data[1] = 0x01;
        data[2] = i;
        if (!node.send_tcp_data(data, 2048)) {
            printf("Failed to send data\n");
            return 0;
        }
        delete[] data;
        sleep_ms(500);
    }

    bool toggle = true;
    for (;;) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        // for (auto it = node.known_nodes.begin(); it != node.known_nodes.end(); it++)
        // {
        //     printf("Knows node: %d\n", it->first);
        // }
        if(!node.is_connected()) {
            printf("Not connected!\n");
        }
        sleep_ms(5000);
    }

    return 0;
}