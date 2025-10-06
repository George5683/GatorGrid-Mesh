#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "libraries/UART/UART.hpp"

#define UART_ID    uart0
#define BAUD_RATE  115200
#define DATA_BITS  8
#define STOP_BITS  1
#define PARITY     UART_PARITY_NONE

#define UART_TX_PIN 16
#define UART_RX_PIN 17

#define MAX_LEN 128

char rxBuffer[MAX_LEN];
int rxIndex = 0;
volatile bool messageReady = false;

void on_uart_rx(void) {

    printf("Interrupt\n");

    while (uart_is_readable(UART_ID)) {
        char c = uart_getc(UART_ID);

        if (rxIndex < MAX_LEN - 1) {
            rxBuffer[rxIndex++] = c;
            rxBuffer[rxIndex] = '\0';
        }

        if (c == '\n') {
            messageReady = true;
            rxIndex = 0;
        }
    }
}

int main() {
    stdio_init_all();

    PicoUART uart;

    uart.picoUARTInit();
    uart.picoUARTInterruptInit();
    printf("UART Initialized\n");

    sleep_ms(2000);

    while (true) {

        printf("Sending: Fortniter\n");
        while (uart_is_writable(UART_ID) == false) {
            tight_loop_contents();
        }

        uart_puts(UART_ID, "Fortniter\n");

        while (messageReady == false) {
            tight_loop_contents();
        }

        printf("Received: %s", rxBuffer);

        messageReady = false;
        sleep_ms(2000);
    }
}
