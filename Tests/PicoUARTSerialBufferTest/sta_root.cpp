#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#define DEBUG 1


int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(5000);

    // initiate everything


    //SPI spi;
    STANode node;
    node.init_sta_mode();
    puts("init done");
    node.start_sta_mode();
    puts("start done");

    
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
    int count = 420;
    uint8_t arr[128] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    char buf[128] = "This is a super long ass string haha %d";


    for (;;) {
        //sleep_ms(10);
        //puts("loop iter");

        
        //memcpy(arr, &count, 4);
        //count++;
        //memcpy(arr+4, &count, 4);

        //node.uart.sendMessage((const char*)arr);
        snprintf(buf, 128, "super long ass string haha %d", count);
        node.uart.sendMessage(buf);

        count++;

        while(node.uart.BufferReady()) {
            //puts((char*)node.uart.getReadBuffer());
            char* tmp = (char*)node.uart.getReadBuffer();
            if(tmp == nullptr) {
                puts("freak");
            }
            //sleep_ms(10);
            //int *test = (int*)node.uart.getReadBuffer();
            //printf("recieved count value %d\n", *test);
            //printf("int right after %d\n", *(test + 4));
        }
        /*

        
        if(!node.is_connected()) {
            printf("Not connected!\n");
        }

        */

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        sleep_ms(50);
    }

    return 0;
}