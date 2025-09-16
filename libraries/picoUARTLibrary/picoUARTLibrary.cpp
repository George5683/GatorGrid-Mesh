#include "picoUARTLibrary.hpp"

PicoUART::PicoUART(){
    //Empty Constructor
}

bool PicoUART::picoUartInit(){
    // Initialize UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);

    return true;
}

bool PicoUART::picoUARTInterruptInit(){
    // Set up and enable UART interrupt
    int UART_IRQ = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    return true;
}

int PicoUART::sendMessage(const char* message){
    // Send a message via UART
    while (uart_is_writable(UART_ID) == false) {
        tight_loop_contents();
    }

    uart_puts(UART_ID, message);
    return 0;
}
