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

class PicoUART {
    char rxBuffer[MAX_LEN];

    public:
    PicoUART();
    ~PicoUART() = default;
    bool picoUartInit();
    bool picoUARTInterruptInit();

    int sendMessage(const char* message);
    char* getReadBuffer();

};
