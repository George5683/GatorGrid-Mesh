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
    spi_set_slave(SPI_PORT, false);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    uint8_t sendString[] = "Fortnite!";  
    
    uint8_t recvBuffer[sizeof(sendString)] = {0};

    uint8_t tempOutBuffer[] = {0};
    uint8_t tempInBuffer[] = {0};

    while (true) {
        memset(recvBuffer, 0, sizeof(recvBuffer));

        printf("Sending Data: %s\n", sendString);

        for(int i = 0; i < sizeof(sendString); i++){
            tempOutBuffer[0] = sendString[i];
            gpio_put(PIN_CS, 0);
            spi_write_read_blocking(SPI_PORT, tempOutBuffer, tempInBuffer, 1);

            //We have absolutely no idea why this sleep command is needed, but we do not get any reply without it.
            sleep_ms(1);

            gpio_put(PIN_CS, 1);
            recvBuffer[i] = tempInBuffer[0];
        }   

        printf("Received Response: %s\n", recvBuffer);

        sleep_ms(1000);
    }
}
