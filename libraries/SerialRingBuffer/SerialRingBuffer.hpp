//#pragma once
#ifndef SERIALRINGBUFFER_H
#define SERIALRINGBUFFER_H


using namespace std;
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <cstdio>

#define MAX_SERIAL_BUF_SIZE 20
#define MAX_SERIAL_BUF_LEN 300

extern uint8_t seral_bufs[MAX_SERIAL_BUF_SIZE][MAX_SERIAL_BUF_LEN];

// Should only be ONE per pico
// This works by recycling the array in global memory to allow the pico
// to quickly write the UART output to the buffer
class SerialRingBuffer {

    // These two pointers point to the start of bufs
    uint8_t putIdx;
    uint8_t getIdx;
    int number_of_messages;

    public:

    SerialRingBuffer();

    // See how many messages are in the Ring Buffer
    int num_of_messages();

    // This returns a pointer to an array of size MAX_SERIAL_BUF_LEN
    // This array is to be written to immedently with the data you want to store in the ring buffer
    // You only need to write to the array to populate the ring buffer position
    // Do not free the pointer passed to you
    uint8_t *buffer_put();

    // Get a message buffer
    // Do not free the pointer passed to you
    // Make sure to use the data quickly after calling buffer_get because this pointer will be overwritten after 
    // MAX_SERIAL_BUF_SIZE number of messages have been added to the ring buffe 
    uint8_t *buffer_get();
};

#endif