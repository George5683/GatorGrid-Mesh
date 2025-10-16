#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "libraries/RingBuffer/RingBuffer.hpp"

#include "libraries/display/display.hpp"
#include "libraries/DHT11/dht11-pico.h"

const uint DHT_PIN = 22;

#define DEBUG 1

int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor

    STANode node;

    
    sleep_ms(5000);

    // initiate everything


    //SPI spi;
    
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

    while(!node.connect_to_node(0));
    if (!node.tcp_init()) {
        // Failed to init TCP connection
        while(true);
    }


    bool got_an_ack = false;
    int count = 0;
    int final_size = 0;

    char send_buf[50] = {0};

    bool toggle = true;


    uint32_t children_ids[4] = {};
    uint8_t number_of_children = 0;

    Dht11 dht11_sensor(DHT_PIN);
    double temperature;
    double rel_humidity;
    
    char buf[50] = {};

    for (;;) {
        node.poll();

        if (count++ % 500 == 0) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;
            if(!node.is_connected()) {
                printf("Not connected!\n");
            }
        }

        if (count == 2000) {
            std::cout<<"Temp:"<<dht11_sensor.readT()<<" °C"<<std::endl;
        }

        if (count == 4000) {
            std::cout<<"RH:"<<dht11_sensor.readRH()<<" %"<<std::endl;
        }

        if (count == 6000) {
            dht11_sensor.readRHT(&temperature, &rel_humidity);
            snprintf(buf, sizeof buf, "Temp: %lf °C\tRH: %lf %", temperature, rel_humidity); 
            DEBUG_printf("%s", buf);

            
            node.send_data(0, sizeof buf, (uint8_t*)buf);

            count = 0;
        }

        

        sleep_ms(1);
    }
    

    return 0;
}