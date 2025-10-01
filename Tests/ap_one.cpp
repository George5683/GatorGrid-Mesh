#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"


bool is_wifi_connected = false;
#define DEBUG 1

int main() {

    // initiate everything
    stdio_init_all();

    APNode node(1);

    sleep_ms(5000);

    //PicoUART uart;

    /*cyw43_arch_init();

    printf("starting uart\n");
    uart.picoUARTInit();
    printf("uart  nitalized\n");
    uart.picoUARTInterruptInit();
    printf("uart intterupts initalized\n");

    printf("Init totally finished\n");*/

    /*while(true) {
        printf("ping\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        sleep_ms(1000);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        sleep_ms(1000);
    }*/

    // initial delay to allow user to look at the serial monitor
    //sleep_ms(5000);

    //multicore_launch_core1(core1_entry);
    

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
    while (node.server_running()) {
        //cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        //cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));

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
        sleep_ms(1000);
    }


    for (;;) {
        puts("Exited");
        sleep_ms(1000);
    }
        
    

    return 0;
}