//Example for Slave
#include "libraries/SPI/SPI.hpp"
#include <stdio.h>

int main() {
    stdio_init_all();

    // Delay to allow time to view serial monitor
    sleep_ms(10000);

    SPI Slave_Pico;

    // set as the slave 
    Slave_Pico.SPI_init(false);
    
    uint8_t send_string[] = "Herobrine";
    uint8_t recv_buffer[sizeof(send_string)] = {0};
    
    for (int i = 0; i < 2; i++) {
        while(!Slave_Pico.SPI_is_read_available());
        if(Slave_Pico.SPI_is_read_available()){
            printf("Read available\n");
            Slave_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
        }
        Slave_Pico.SPI_send_message(send_string, sizeof(send_string));
        }
        sleep_ms(1000);  // Wait before next transmission
    }