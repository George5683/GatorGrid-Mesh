#include <stdio.h>
#include "pico/stdlib.h"
#include "libraries/SPI/SPI.hpp"
#include <string.h>

#define TRUE_PIN 0
#define SEND_PIN 15

int main() {
    stdio_init_all();

    gpio_init(TRUE_PIN);
    gpio_set_dir(TRUE_PIN, GPIO_OUT);
    gpio_put(TRUE_PIN, 1);

    gpio_init(SEND_PIN);
    gpio_set_dir(SEND_PIN, GPIO_IN);

    sleep_ms(2000);  // Wait for master

    SPI Slave;
    Slave.SPI_init(false);

    uint8_t send_buffer[] = "Herobrine";        // Data to send to master
    uint8_t recv_buffer[sizeof(send_buffer)] = {0};
    bool ready_to_send = true;

    while (true) {
        bool read_trigger = gpio_get(SEND_PIN);

        if (read_trigger && ready_to_send) {
            // Slave prepares buffer for master
            ready_to_send = false;
        } else if (!read_trigger) {
            // Master is clocking SPI: read master's data and send response
            Slave.SPI_read_message(recv_buffer, sizeof(recv_buffer));
            Slave.SPI_send_message(send_buffer, sizeof(send_buffer));

            // Print received data if any
            bool has_data = false;
            for (size_t i = 0; i < sizeof(recv_buffer); i++)
                if (recv_buffer[i] != 0) has_data = true;

            if (has_data) printf("Slave received: %s\n", recv_buffer);

            ready_to_send = true;  // Reset for next transfer
        }

        sleep_ms(10);  // Small delay
    }
}