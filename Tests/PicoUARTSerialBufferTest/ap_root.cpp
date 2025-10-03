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
    sleep_ms(5000);
    display_test();

    //multicore_launch_core1(core1_entry);
    APNode node(420);

    printf("Init AP Mode\n");
    if (!node.init_ap_mode())
    {
        sleep_ms(1000);
        printf("inti fail\n");
        while(1);
    }

    printf("Starting AP Mode\n");
    if (!node.start_ap_mode()) {
        sleep_ms(1000);
    }

    printf("AP ID: %08x\n", node.get_NodeID());

    // Give salve time to process
    

    
    // if(spi.SPI_send_message(id, 4) != 1) {
    //     for (;;) { sleep_ms(1000); }
    // }

    uint8_t arr[128] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    bool toggle = true;
    int count = 69;
    char buf[128] = "This is a super long ass string haha %d";

    while (node.server_running()) {
        //puts("loop iter");
        //sleep_ms(9);

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

        

        

        //cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        //cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));

        /*
        if(node.number_of_messages() > 0) {
            puts("You got mail!");
            while(node.number_of_messages() != 0) {
                struct data tmp = node.digest_data();
                printf("Source: %08x\nData: \"%s\"\n", tmp.source, tmp.data);
            }
        }

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        //printf("core print...");
        */
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        sleep_ms(50);
    }


    for (;;) {
        puts("Exited");
        sleep_ms(1000);
    }

    return 0;
}