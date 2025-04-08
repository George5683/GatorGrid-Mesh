#include "libraries/SPI/SPI.hpp"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/binary_info.h"

#define BUF_LEN         0x100

int main() {
    stdio_init_all();
    
    // Give time for serial console to connect
    sleep_ms(10000);
    
    printf("Starting slave device...\n");
    
    uint8_t send_string[] = "Fortnite!";
    uint8_t recv_buffer[2048] = {0};

    SPI Slave_Pico;
    Slave_Pico.SPI_init(false);
    
    uint8_t out_buf[BUF_LEN], in_buf[BUF_LEN];
    
    for (size_t i = 0; i < BUF_LEN; ++i) {
        out_buf[i] = 'a';
    }

    Slave_Pico.SPI_read_message(out_buf, BUF_LEN);
    Slave_Pico.SPI_send_message(in_buf, BUF_LEN);

    for (size_t j = 0; j < BUF_LEN; ++j) {
        printf("%02x ", in_buf[j]);
        if ((j + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    
    sleep_ms(1000);
}