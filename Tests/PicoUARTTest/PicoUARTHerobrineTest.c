#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

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

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    int UART_IRQ = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    sleep_ms(2000);

    while (true) {
        if (messageReady) {

            printf("Received: %s", rxBuffer);

            while (!uart_is_writable(UART_ID)) {
                tight_loop_contents();
            }

            uart_puts(UART_ID, "Herobrine\n");
            printf("Sending: Herobrine\n");

            messageReady = false;
        }

        sleep_ms(50);
    }
}
