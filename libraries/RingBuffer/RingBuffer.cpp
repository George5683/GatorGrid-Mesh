using namespace std;

#include "RingBuffer.hpp"

RingBuffer::RingBuffer(int size) {
    this->size = size;
    this->index = 0;
    struct data tmp = {0};
    this->number_of_messages = 0;

    for(int i = 0; i < 5; i++) {
        buf.push_back(tmp);
    }
};

void RingBuffer::insert(uint8_t *data, ssize_t len) {
    number_of_messages++;

    if(buf[index].data != nullptr)
        free(buf[index].data);

    buf[index].data = data;
    buf[index].size = len;

    index = (index + 1) % size;
}

struct data RingBuffer::digest() {
    struct data ret = {0};

    if(number_of_messages == 0) {
        return ret;
    }
    
    number_of_messages--;

    if(this->index != 0)
        this->index--;

    ret = this->buf[this->index];

    return ret;
}

int RingBuffer::get_size(){
    return size;
}