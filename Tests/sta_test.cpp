#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

/*
void core1_entry() {
    //stdio_init_all(); // Initialize stdio only once
    
    // Initialize the STA node
    STANode node;

    // Initialize STA mode
    if (!node.init_sta_mode()) {
        printf("Failed to initialize STA mode\n");
        return;
    }

    // Start STA mode
    if (!node.start_sta_mode()) {
        printf("Failed to start STA mode\n");
        return;
    }

    bool toggle_led = true;
    bool wifi_connected = false;

    // Initialize the SPI slave
    SPI Slave_Pico;
    Slave_Pico.SPI_init(false);
    
    uint8_t send_string[] = "ID-Received";
    uint8_t recv_buffer[2048] = {0};

    // Get the NodeID from the AP node through SPI
    printf("Reading message received through SPI\n");
    Slave_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
    printf("Sending message through SPI\n");
    Slave_Pico.SPI_send_message(send_string, sizeof(send_string));

    // Continue the loop until connected to a node
    while (!wifi_connected) {
        // Scan for nearby nodes
        if (!node.scan_for_nodes()) {
            printf("Scan failed, retrying...\n");
            sleep_ms(5000);
            continue;
        }

        // Toggle LED to indicate scanning
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle_led);
        toggle_led = !toggle_led;

        // Check if any nodes are found
        if (!node.known_nodes.empty()) {
            // Connect to the first node in the map
            auto entry = *node.known_nodes.begin();
            uint32_t connect_id = entry.first;

            printf("Attempting to connect to node: %u\n", connect_id);
            if (node.connect_to_node(connect_id)) {
                wifi_connected = true;
                printf("Connected to node: %u\n", connect_id);

                // Send a message to the connected node
                std::string message = "Hello from GatorGrid node!";
                if (node.send_string_data(message.c_str())) {
                    printf("Data sent successfully!\n");
                } else {
                    printf("Failed to send data\n");
                }
            } else {
                printf("Failed to connect to node: %u\n", connect_id);
            }
        }

        sleep_ms(5000); // Wait before retrying
    }

    // Keep the LED on to indicate successful connection
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}
    */

int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);

    // initiate everything
    

    // multicore_launch_core1(core1_entry);

    //SPI spi;
    STANode node;
    uint8_t id[4];
    printf("Now looking for SPI message");
    // if (spi.SPI_read_message(id, 4) != 1) {
    //     for (;;) { sleep_ms(1000); }
    // }
    // node.set_NodeID(*reinterpret_cast<uint32_t*>(id));
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
        node.send_tcp_data(msg.get_msg(), msg.get_len(), false);
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