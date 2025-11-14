#pragma once
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "../SerialRingBuffer/SerialRingBuffer.hpp"

#define UART_ID    uart1
#define BAUD_RATE  115200
#define DATA_BITS  8
#define STOP_BITS  1
#define PARITY     UART_PARITY_NONE


// TODO: IFDEF based off of pico2_w or pico_2
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define MAX_LEN     128

class PicoUART {
private:
    //uint8_t rxBuffer[MAX_LEN];
    SerialRingBuffer srb;
    static PicoUART* instance;
    static int    dma_chan;

    bool toggle = true;

public:
    PicoUART();
    ~PicoUART() = default;

    bool picoUARTInit();
    bool BufferReady(); // check to see if we have a message waiting for us
    bool picoUARTInterruptInit();
    int sendMessage(const char* message);
    uint8_t* getReadBuffer();

    static void on_uart_rx();
};