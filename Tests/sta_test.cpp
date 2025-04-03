#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "libraries/MeshNode/Messages.hpp"

#if 1
 static void dump_bytes(const void *ptr, uint32_t len) {
    const uint8_t* bptr = (uint8_t*)ptr;
     unsigned int i = 0;
 
     printf("dump_bytes %d", len);
     for (i = 0; i < len;) {
         if ((i & 0x0f) == 0) {
             printf("\n");
         } else if ((i & 0x07) == 0) {
             printf(" ");
         }
         printf("%02x ", bptr[i++]);
     }
     printf("\n");
 }
 #define DUMP_BYTES dump_bytes
 #else
 #define DUMP_BYTES(A,B)
 #endif

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


    // TCP_DATA_MSG msg(node.get_NodeID(), 0x12345678);
    // uint8_t arr[] = {1,2,3,4,5,6,7,8,9};
    // msg.add_message(9, arr);
    // DUMP_BYTES(msg.get_msg(), msg.get_len());

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
        TCP_DATA_MSG msg(node.get_NodeID(), 0x12345678);
        uint8_t arr[] = "hello, this is message:  ";
        arr[24] = 48 + i;
        msg.add_message(25, arr);
        node.send_tcp_data(msg.get_msg(), msg.get_len());
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