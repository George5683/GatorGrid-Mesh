#include "UART.hpp"

#if DEBUG 
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

PicoUART* PicoUART::instance = nullptr;

// Constructor
PicoUART::PicoUART() {
    // Empty constructor
}

// Initialize UART hardware
bool PicoUART::picoUARTInit() {
    instance = this;
    
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_fifo_enabled(UART_ID, false);

    return true;
}

// Initialize UART interrupt
bool PicoUART::picoUARTInterruptInit() {
    instance = this;

    int UART_IRQ = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, this->on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_fifo_enabled(UART_ID, true);
    uart_set_irq_enables(UART_ID, true, false);

    return true;
}

// While reads do go out of bounds that is not likely to cause a crash on this uP
// Send message over UART
int PicoUART::sendMessage(const char* message) {

    while (!uart_is_writable(UART_ID)) {
        tight_loop_contents();
    }

    // If UART FIFO has data in it then don't write yet to avoid collision
    while(uart_is_readable(UART_ID)) {
        tight_loop_contents();
    }

    //uart_set_irq_enables(UART_ID, false, false);
    uart_write_blocking(UART_ID, (uint8_t *)message, MAX_LEN);
    //uart_set_irq_enables(UART_ID, true, false);
    return 0;
}

// Return pointer to received buffer
// Only call while BufferReady returns true
uint8_t* PicoUART::getReadBuffer() {
    uint8_t *buffer = instance->srb.buffer_get();

    // if the ring buffer is empty and they call getReadBuffer stall because something has gone wrong
    if(buffer == nullptr) {
        while(1) {
            puts("Fatal Error: read buffer empty\n");
        }
    }

    return buffer;
}

// check to see if we have a message waiting for us
bool PicoUART::BufferReady() {
    if (instance->srb.num_of_messages() > 0) {
        return true;
    } else {
        return false;
    }
}

static bool toggle;

// Static ISR
void PicoUART::on_uart_rx() {

    DEBUG_printf("ISR Triggered\n");

    if (!instance) {
        DEBUG_printf("instance is nullptr\n");
        return;
    }

    // ISR thrown twice, so just have a toggle gate guard it
    if (instance->toggle == false) {
        instance->toggle = true;
        DEBUG_printf("Toggle protection thrown\n");
        return;
    }

    instance->toggle = false;

    uart_get_hw(UART_ID)->icr = UART_UARTICR_RXIC_BITS;
    DEBUG_printf("get hw successful\n");

    

    uint8_t *buffer = instance->srb.buffer_put();

    DEBUG_printf("got ISR buffer\n");

    // Should the ring buffer ever be full stall the pico
    if(buffer == nullptr) {
        while(1) {
            printf("Fatal Error: Serial recieved messages faster then poll could digest them\n");
        }
    }

    while (uart_is_readable(UART_ID)) {
        uart_read_blocking(UART_ID, buffer, MAX_LEN);
    }

    return;
}