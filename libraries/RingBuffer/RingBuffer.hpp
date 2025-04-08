using namespace std;
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

struct data {
    uint8_t *data;
    ssize_t size;
};

class RingBuffer {
    public:
    vector<struct data> buf;
    int index;
    int size;

    RingBuffer(int size);

    void insert(uint8_t *data, ssize_t len);

    struct data digest();

};