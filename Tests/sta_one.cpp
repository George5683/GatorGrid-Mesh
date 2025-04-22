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

    //sleep_ms(1000);

    /*for (int i = 0; i < 10; i++)
    {
        TCP_DATA_MSG msg(node.get_NodeID(), 2);
        uint8_t arr[] = "hello, this is message:  ";
        arr[24] = 48 + i;
        msg.add_message(25, arr);
        printf("Sending message: %d/10\n", i+1);
        while (!node.send_tcp_data_blocking(msg.get_msg(), msg.get_len(), false));
    }*/

    bool got_an_ack = false;
    int count = 0;
    int final_size = 0;

    char send_buf[50] = {0};

    bool toggle = true;
    for (;;) {
        sleep_ms(250);

        //while(got_an_ack) {
            
        //}

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        if(!node.is_connected()) {
            printf("Not connected!\n");
        }

        final_size = snprintf(send_buf, 50, "FORTNITE %d\n", count);

        while(node.send_data(2, final_size, (uint8_t*)send_buf) != 0) {
            printf(ANSI_RED_TEXT "Attempting send\n");
            sleep_ms(1000);
        }

        printf("Sent data \"FORTNITE %i\"\n", count);
        count++;

        while (node.number_of_messages() == 0) {
            sleep_ms(2);
        }

        struct data tmp = node.digest_data();
        tmp.data[tmp.size] = 0;
        printf(ANSI_GREEN_TEXT "Received message from node %08x: %s\n", tmp.source, (char*)tmp.data);
        printf( ANSI_CLEAR_SCROLLBACK ANSI_CLEAR_SCREEN ANSI_CURSOR_HOME );
        sleep_ms(250);
    }
    

    return 0;
}