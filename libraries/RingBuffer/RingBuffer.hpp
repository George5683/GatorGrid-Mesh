#pragma once

using namespace std;
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <cstdio>

struct data {
    uint8_t data[128];
    ssize_t size;
    uint32_t source;
    uint32_t dest;
};

class RingBuffer {
    public:
    vector<struct data> buf;
    int index;
    int size;
    int number_of_messages;

    RingBuffer(int size);

    void insert(uint8_t *data, ssize_t len, uint32_t source, uint32_t dest);
    int get_size();
    struct data digest();

};