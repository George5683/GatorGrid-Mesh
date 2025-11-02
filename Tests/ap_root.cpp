#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/display/display.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

bool is_wifi_connected = false;

#define DEBUG 1


int main() {

    // initiate everything
    stdio_init_all();

    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);
    display_test();

    //multicore_launch_core1(core1_entry);
    APNode node(0);

    printf("Init AP Mode\n");
    if (!node.init_ap_mode())
    {
        sleep_ms(1000);
    }

    printf("Starting AP Mode\n");
    if (!node.start_ap_mode()) {
        sleep_ms(1000);
    }

    printf("AP ID: %08x\n", node.get_NodeID());

    
    // if(spi.SPI_send_message(id, 4) != 1) {
    //     for (;;) { sleep_ms(1000); }
    // }
    bool toggle = true;
    int count = 0;

    uint32_t children_ids[4] = {};
    uint8_t number_of_children = 0;
    char data[10] = "Test msg";
    
    while (node.server_running()) {

        node.poll();

        if (count++ >= 500) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;

            node.tree.get_children(node.get_NodeID(), children_ids, number_of_children);
            printf("\n\nChildren:\n");
            for (int i = 0; i < number_of_children; i++) {
                printf("%u\t", children_ids[i]);
            }
            printf("\n");

            /*if(number_of_children != 0) {
                printf("Attempting to send message to node 1\n");

                if(node.send_data(1, 10, (uint8_t*)data) == -1) {
                    printf("send_data returned -1\n");
                } else {
                    printf("send_data returned 0\n");
                }
            }*/

            count = 0;
        }

        sleep_ms(1);
    }


    for (;;) {
        puts("Exited");
        sleep_ms(1000);
    }

    return 0;
}