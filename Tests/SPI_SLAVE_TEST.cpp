//Example for Slave
#include "libraries/SPI/SPI.hpp"
#include <stdio.h>
#include <vector>

int main() {
    stdio_init_all();

    // Delay to allow time to view serial monitor
    sleep_ms(10000);

    SPI Slave_Pico;

    // set as the slave 
    Slave_Pico.SPI_init(false);
    
    std::vector<uint8_t> send_string = {'H', 'e', 'r', 'o', 'b', 'r', 'i', 'n', 'e'};

    for (int i = 0; i < 5; i++) {
        // Wait until the master is ready to send data
        std::vector<uint8_t> recv_buffer(send_string.size());
        Slave_Pico.SPI_send_message_read_message(send_string, recv_buffer);
        printf("Size of Received Buffer: %zu\n", recv_buffer.size());
        sleep_ms(500); // Wait for 500 milliseconds before next transmission
    }
}