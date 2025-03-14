#include "SPI.hpp"
#include <stdio.h>
#include <string>
#include <cstring>

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// Default constructor
SPI::SPI(){};

// Initiate for the master
void SPI::SPI_init(bool mode){
    is_master = mode;
    if(mode == true){
        // Setting up the Master
        spi_init(SPI_PORT, 1000 * 1000);
        spi_set_slave(SPI_PORT, false);

        gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
        gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

        gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
        gpio_set_dir(PIN_CS, GPIO_OUT);
        gpio_put(PIN_CS, 1);
    }
    else{
        // Set as slave
        spi_init(SPI_PORT, 1000 * 1000);
        spi_set_slave(SPI_PORT, true);

        gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
        gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
        gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
    }
};

int SPI::SPI_send_message(uint8_t *message, size_t length){
    uint8_t tempOutBuffer = 0;
    uint8_t tempInBuffer = 0;

    printf("Sending Data: %s\n", message);

    if(is_master == true){
        for(int i = 0; i < length; i++){
            tempOutBuffer = message[i];
            gpio_put(PIN_CS, 0);  // Assert CS
            sleep_ms(1);  // Delay for timing requirements
            spi_write_read_blocking(SPI_PORT, &tempOutBuffer, &tempInBuffer, 1);
            gpio_put(PIN_CS, 1);  // Deassert CS
        }
    }
    else{
        for(int i = 0; i < length; i++){
            tempOutBuffer = message[i];
            spi_write_read_blocking(SPI_PORT, &tempOutBuffer, &tempInBuffer, 1);
        }

    }

    return 1;
};

int SPI::SPI_read_message(uint8_t *buffer, size_t buffer_size){
    uint8_t tempOutBuffer = 0;
    uint8_t tempInBuffer = 0;

    // Clear buffer before reading
    memset(buffer, 0, buffer_size);

    if(is_master == true){

        for(int i = 0; i < buffer_size - 1; i++){
            gpio_put(PIN_CS, 0);
            sleep_ms(1);
            spi_write_read_blocking(SPI_PORT, &tempOutBuffer, &tempInBuffer, 1);
            gpio_put(PIN_CS, 1);
            buffer[i] = tempInBuffer;

            // Break if we received a null terminator (end of string)
            if (tempInBuffer == 0) {
                break;
            }
        }
        // Ensure null-termination
        buffer[buffer_size - 1] = 0;
        printf("Received Data: %s\n", buffer);
    }
    else{
        for(int i = 0; i < buffer_size - 1; i++){

            spi_write_read_blocking(SPI_PORT, &tempOutBuffer, &tempInBuffer, 1);
            buffer[i] = tempInBuffer;

            // Break if we received a null terminator (end of string)
            if (tempInBuffer == 0) {
                break;
            }
        }
        // Ensure null-termination
        buffer[buffer_size - 1] = 0;
        printf("Received Data: %s\n", buffer);
    }
    return 1;
};


