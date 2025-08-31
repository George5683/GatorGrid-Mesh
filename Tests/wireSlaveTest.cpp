#include <stdio.h>
#include "pico/stdlib.h"
#include "libraries/SPI/SPI.hpp"

#define TRUE_PIN 0
#define SEND_PIN 15

int main() {
    stdio_init_all();

    gpio_init(TRUE_PIN);
    gpio_set_dir(TRUE_PIN, GPIO_OUT);
    gpio_put(TRUE_PIN, 1);

    gpio_init(SEND_PIN);
    gpio_set_dir(SEND_PIN, GPIO_IN);

    sleep_ms(2000);  // Wait for slave

    SPI Master;
    Master.SPI_init(true);

    uint8_t send_buffer[] = "Fortnite!";
    uint8_t recv_buffer[sizeof(send_buffer)] = {0};
    bool sent = false;

    while (true) {
        bool read_trigger = gpio_get(SEND_PIN);

        if (read_trigger && !sent) {
            printf("Master sending: %s\n", send_buffer);

            // Send message to slave
            Master.SPI_send_message(send_buffer, sizeof(send_buffer));

            // Read response from slave
            Master.SPI_read_message(recv_buffer, sizeof(recv_buffer));

            // Only print if slave sent something
            bool has_data = false;
            for (size_t i = 0; i < sizeof(recv_buffer); i++)
                if (recv_buffer[i] != 0) has_data = true;

            if (has_data) printf("Master received: %s\n", recv_buffer);

            sent = true;
        } else if (!read_trigger) {
            // Reset for next transfer
            sent = false;
        }

        sleep_ms(10);  // Small debounce
    }
}