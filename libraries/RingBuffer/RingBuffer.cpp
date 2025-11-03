#include <lwip/opt.h>
#include <cstring>

#include "display.hpp"
using namespace std;

#include "RingBuffer.hpp"

RingBuffer::RingBuffer(int size) {
    this->size = size;
    this->write_index = 0;
    this->read_index = 0;
    this->number_of_messages = 0;
    
    struct data tmp = {0};
    buf.reserve(size);  // Pre-allocate for efficiency
    for(int i = 0; i < size; i++) {
        buf.push_back(tmp);
    }
}

void RingBuffer::insert(uint8_t *data, ssize_t len, uint32_t source, uint32_t dest) {
    DEBUG_printf("Inserting data with length: %zd, from node %08x to node %08x\n", 
                 len, source, dest);
    
    // Validate inputs
    if (!data || len <= 0) {
        ERROR_printf("Invalid insert parameters\n");
        return;
    }
    
    if (len > 128) {
        ERROR_printf("Message too long (%zd), truncating to 128\n", len);
        len = 128;
    }
    
    // Check for overflow
    if (number_of_messages >= size) {
        ERROR_printf("RingBuffer full! Dropping oldest message\n");
        // Advance read pointer to make space (drops oldest)
        read_index = (read_index + 1) % size;
        number_of_messages--;
    }
    
    // Copy data
    memcpy(buf[write_index].data, data, len);
    buf[write_index].size = len;
    buf[write_index].source = source;
    buf[write_index].dest = dest;
    
    // Advance write pointer
    write_index = (write_index + 1) % size;
    number_of_messages++;
}

struct data RingBuffer::digest() {
    struct data ret = {0};
    
    if (number_of_messages == 0) {
        DEBUG_printf("RingBuffer empty, nothing to digest\n");
        return ret;
    }
    
    // Read from read_index (oldest message)
    ret = buf[read_index];
    
    // Advance read pointer
    read_index = (read_index + 1) % size;
    number_of_messages--;
    
    DEBUG_printf("Digested message: len=%zd, source=%08x, dest=%08x\n",
                 ret.size, ret.source, ret.dest);
    
    return ret;
}

int RingBuffer::get_size() {
    return number_of_messages;
}