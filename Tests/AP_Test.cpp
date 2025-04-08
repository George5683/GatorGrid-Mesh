#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/SPI/SPI.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

bool is_wifi_connected = false;

void core1_entry() {

    APNode node;

    if (!node.init_ap_mode())
    {
        return;
    }

    if (!node.start_ap_mode()) {
        return;
    }

    bool toggle = true;
    for (;;) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        //printf("core print...");
        sleep_ms(1000);
    }


}

int main() {

    // initiate everything
    stdio_init_all();

    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);

    //multicore_launch_core1(core1_entry);
    APNode node;
    SPI spi;

    printf("Init AP Mode\n");
    if (!node.init_ap_mode())
    {
        sleep_ms(1000);
    }

    printf("Starting AP Mode\n");
    if (!node.start_ap_mode()) {
        sleep_ms(1000);
    }

    uint8_t id[4];
    uint32_t node_id = node.get_NodeID();
    *id = *reinterpret_cast<uint8_t*>(&node_id);
    printf("Now sending for SPI message");
    // if(spi.SPI_send_message(id, 4) != 1) {
    //     for (;;) { sleep_ms(1000); }
    // }
    bool toggle = true;
    for (;;) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        //printf("core print...");
        sleep_ms(1000);
    }


    for (;;) {
        sleep_ms(1000);
    }

    return 0;
}