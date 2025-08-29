//Example for Master
#include "libraries/SPI/SPI.hpp"
#include <stdio.h>
#include <vector>

int main() {
    stdio_init_all();

    // Delay to allow time to view serial monitor
    sleep_ms(10000);

    SPI Master_Pico;

    sleep_ms(2000);
    
    // set as the master 
    Master_Pico.SPI_init(true);

    std::vector<uint8_t> send_string = {'F', 'o', 'r', 't', 'n', 'i', 't', 'e', '!'};

    for (int i = 0; i<5 ;i++) {
        while(!Master_Pico.SPI_is_write_available());
        if(Master_Pico.SPI_is_write_available()){
            std::vector<uint8_t> recv_buffer(send_string.size());
            printf("Write available\n");
            Master_Pico.SPI_send_message_read_message(send_string, recv_buffer);

            // print the size of receive buffer
            printf("Size of Received Buffer: %zu\n", (int)recv_buffer.size());
        }
        sleep_ms(500); // Wait for 500 milliseconds before next transmission
    }
}