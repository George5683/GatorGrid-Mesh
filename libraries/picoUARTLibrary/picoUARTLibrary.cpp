#include "picoUART.hpp"

PicoUART* PicoUART::instance = nullptr;

// Constructor
PicoUART::PicoUART() {
    // Empty constructor
}

// Initialize UART hardware
bool PicoUART::picoUartInit() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    return true;
}

// Initialize UART interrupt
bool PicoUART::picoUARTInterruptInit() {
    instance = this;

    int UART_IRQ = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    return true;
}

// Send message over UART
int PicoUART::sendMessage(const char* message) {
    while (!uart_is_writable(UART_ID)) {
        tight_loop_contents();
    }
    uart_puts(UART_ID, message);
    return 0;
}

// Return pointer to received buffer
char* PicoUART::getReadBuffer() {
    return rxBuffer;
}

// Static ISR
void PicoUART::on_uart_rx() {
    if (!instance) return;

    while (uart_is_readable(UART_ID)) {
        char c = uart_getc(UART_ID);
        if (instance->rxIndex < MAX_LEN - 1) {
            instance->rxBuffer[instance->rxIndex++] = c;
            instance->rxBuffer[instance->rxIndex] = '\0';
        }

        if (c == '\n') {
            instance->rxIndex = 0;
        }
    }
}
