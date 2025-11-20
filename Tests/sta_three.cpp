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
    node.is_root = false;

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

        node.poll();

        if (count == 500) {
            // if (node.tree.node_exists(2)) {
            //     TCP_DATA_MSG msg(node.get_NodeID(), 2);
            //     msg.add_message((uint8_t*)hullo, sizeof(hullo));
            //     node.send_msg(msg.get_msg());
            // }
        }

        if (count++ >= 1000) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;
            if(!node.is_connected()) {
                printf("Not connected!\n");
            }
            // node.tree.get_children(node.get_NodeID(), children_ids, number_of_children);
            // printf("\n\nChildren:\n");
            // for (int i = 0; i < number_of_children; i++) {
            //     printf("%u\t", children_ids[i]);
            // }
            // printf("\n");

            // node.tree.send_tree_serial();
            // if (node.rb.get_size() > 0) {
            //     struct data received = node.rb.digest();
            //     //DUMP_BYTES(received.data, received.size);
            //     printf("RINGBUFFER:%s\n", (char*)received.data);
            // }

            count = 0;
        }
       

        sleep_ms(1);
    }

    return 0;
}