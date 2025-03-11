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
        node.scan_for_nodes();
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        toggle = !toggle;
        printf("core print...");
        sleep_ms(5000);
    }


}

int main() {
    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);

    // initiate everything
    stdio_init_all();

    //multicore_launch_core1(core1_entry);

    STANode Node;
    Node.init_sta_mode();
    Node.start_sta_mode();

    for (;;) {
        sleep_ms(1000);
        Node.scan_for_nodes();
    }

    return 0;
}