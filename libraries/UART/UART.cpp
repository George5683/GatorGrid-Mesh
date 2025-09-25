#include "UART.hpp"

PicoUART* PicoUART::instance = nullptr;

// Constructor
PicoUART::PicoUART() {
    // Empty constructor
}

// Initialize UART hardware
bool PicoUART::picoUARTInit() {
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
    //uart_puts(UART_ID, message);
    uart_write_blocking(UART_ID, (uint8_t *)message, MAX_LEN);
    return 0;
}

// Return pointer to received buffer
uint8_t* PicoUART::getReadBuffer() {
    instance->buffer_ready = false;
    instance->rxIndex = MAX_LEN;
    return rxBuffer;
}

// check to see if we have a message waiting for us
bool PicoUART::BufferReady() {
    return instance->buffer_ready;
}

// Static ISR
void PicoUART::on_uart_rx() {
    if (!instance) return;

    instance->buffer_ready = true;

    while (uart_is_readable(UART_ID)) {
        printf("UART READ LOOP, should only run once");
        uart_read_blocking(UART_ID, instance->rxBuffer, MAX_LEN);
        instance->rxIndex = MAX_LEN;
    }


}