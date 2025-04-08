using namespace std;

#include "RingBuffer.hpp"

RingBuffer::RingBuffer(int size) {
    this->size = size;
    this->index = 0;
    struct data tmp = {0};

    for(int i = 0; i < 5; i++) {
        buf.push_back(tmp);
    }
};

void RingBuffer::insert(uint8_t *data, ssize_t len) {
    if(buf[index].data != nullptr)
        free(buf[index].data);

    buf[index].data = data;
    buf[index].size = len;

    index = (index + 1) % size;
}

struct data RingBuffer::digest() {
    if(this->index != 0)
        this->index--;

    struct data ret = this->buf[this->index];

    return ret;
}