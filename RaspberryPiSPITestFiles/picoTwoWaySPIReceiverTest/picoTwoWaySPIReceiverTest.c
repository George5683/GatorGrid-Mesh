#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

int main() {
    stdio_init_all();

    spi_init(SPI_PORT, 1000 * 1000);
    spi_set_slave(SPI_PORT, true);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

    uint8_t recvBuffer[10] = {0};  
    uint8_t response[] = "Herobrine";
    
    uint8_t tempOutBuffer[] = {0};
    uint8_t tempInBuffer[] = {0};

    while (true) {
        memset(recvBuffer, 0, sizeof(recvBuffer));

        for(int i = 0; i < sizeof(response); i++){
            tempOutBuffer[0] = response[i];
            spi_write_read_blocking(SPI_PORT, tempOutBuffer, tempInBuffer, 1);
            recvBuffer[i] = tempInBuffer[0];
        }
        printf("Received Data: %s\n", recvBuffer);
        printf("Sending Response: %s\n", response);

        sleep_ms(500);
    }
}
