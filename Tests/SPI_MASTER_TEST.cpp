//Example for Master
#include "libraries/SPI/SPI.hpp"
#include <stdio.h>

int main() {
    stdio_init_all();

    // Delay to allow time to view serial monitor
    sleep_ms(10000);

    // Delay to wait for Slave to turn on and initialize
    sleep_ms(2000);

    SPI Master_Pico;
    
    // set as the master 
    Master_Pico.SPI_init(true);
    
    uint8_t send_string[] = "Fortnite!";
    uint8_t recv_buffer[sizeof(send_string)] = {0};
    
    for (int i = 0; i<2 ;i++) {
        Master_Pico.SPI_send_message(send_string, sizeof(send_string));
        Master_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
        sleep_ms(1000);  // Wait before next transmission
    }
}