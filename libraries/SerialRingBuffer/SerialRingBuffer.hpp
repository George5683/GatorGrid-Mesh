//#pragma once
#ifndef SERIALRINGBUFFER_H
#define SERIALRINGBUFFER_H


using namespace std;
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <cstdio>

#define MAX_SERIAL_BUF_SIZE 50
#define MAX_SERIAL_BUF_LEN 128

// MAX_LEN from UART.hpp
extern uint8_t seral_bufs[MAX_SERIAL_BUF_SIZE][MAX_SERIAL_BUF_LEN];

// Should only be one per pico
class SerialRingBuffer {

    // These two pointers point to the start of bufs
    uint8_t putIdx;
    uint8_t getIdx;
    int number_of_messages;

    public:

    SerialRingBuffer();

    int num_of_messages();


    // These functions are intended to use the ring buffer to get pointers to read and write to
    // The pointers it returns are to not be freed.

    // Get a message buffer position to write data to
    uint8_t *buffer_put();
    // Get a message buffer
    uint8_t *buffer_get();

    // Used in UART, take a buffer from the stack but put it to the back
    //struct data digest_with_recycle();
};

#endif