#include "SPI/SPI.hpp"
#include <stdio.h>

int main() {
    stdio_init_all();

    SPI Slave_Pico;

    // set as the slave 
    Slave_Pico.SPI_init(false);
    
    uint8_t send_string[] = "Herobrine";
    uint8_t recv_buffer[sizeof(send_string)] = {0};
    
    while (true) {
        Slave_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
        Slave_Pico.SPI_send_message(send_string, sizeof(send_string));
        sleep_ms(500);  // Wait before next transmission
    }
}