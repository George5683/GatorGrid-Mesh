#include "SPI/SPI.hpp"
#include <stdio.h>


int main() {
    stdio_init_all();

    SPI Master_Pico;

    // set as the master 
    Master_Pico.SPI_init(true);
    
    uint8_t send_string[] = "Fortnite!";
    uint8_t recv_buffer[sizeof(send_string)] = {0};
    
    while (true) {
        Master_Pico.SPI_send_message(send_string, sizeof(send_string));
        Master_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
        sleep_ms(1000);  // Wait before next transmission
    }
}