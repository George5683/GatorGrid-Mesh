using namespace std;

#include "SerialRingBuffer.hpp"
uint8_t seral_bufs[MAX_SERIAL_BUF_SIZE][MAX_SERIAL_BUF_LEN];

SerialRingBuffer::SerialRingBuffer() {
    putIdx = 0;
    getIdx = 0;
    number_of_messages = 0;
};

int SerialRingBuffer::num_of_messages(){
    return number_of_messages;
}

uint8_t *SerialRingBuffer::buffer_put() {

    if(number_of_messages == MAX_SERIAL_BUF_SIZE) {
        return nullptr;
    }

    uint8_t *buf = &seral_bufs[putIdx][0];
    //printf("put buf %d\n", buf);
    //printf("put idx %d\n", putIdx);

    putIdx++;
    number_of_messages++;

    if(putIdx == MAX_SERIAL_BUF_SIZE) {
        putIdx = 0;
    }

    return buf;
}

uint8_t *SerialRingBuffer::buffer_get() {
    if(number_of_messages == 0) {
        return nullptr;
    }

    uint8_t *buf = &seral_bufs[getIdx][0];
    //printf("get buf %d\n", buf);
    //printf("get idx %d\n", getIdx);

    getIdx++;
    number_of_messages--;

    if(getIdx == MAX_SERIAL_BUF_SIZE) {
        getIdx = 0;
    }

    return buf;
}