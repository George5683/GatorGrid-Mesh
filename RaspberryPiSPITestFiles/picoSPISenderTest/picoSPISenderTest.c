#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

int main() {
    stdio_init_all();
    cyw43_arch_init();

    spi_init(SPI_PORT, 1000 * 1000);
    spi_set_slave(SPI_PORT, false);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    uint8_t tempString[10] = "Fortniter";

    uint8_t dataBuffer[10] = {0};
    uint8_t tempBuffer[] = {0};

    int bytesSent = 0;

    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        printf("Sending Data: %s\n", dataBuffer);

        bytesSent = 0;

        for(int i = 0; i < sizeof(tempString); i++){
            dataBuffer[0] = tempString[i];
            gpio_put(PIN_CS, 0);
            bytesSent = bytesSent + spi_write_read_blocking(SPI_PORT, dataBuffer, tempBuffer, 1);
            gpio_put(PIN_CS, 1);
        }

        printf("Bytes sent: %d\n", bytesSent);

        printf("Sent bytes: ");
        for (int i = 0; i < sizeof(tempString) - 1; i++) {
            printf("%02X ", tempString[i]);
        }
        printf("\n");

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(1000);
    }
}
