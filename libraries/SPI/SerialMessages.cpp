#include <cstdint>
#include <cstring>
#include <cstdio>
#include "SerialMessages.hpp"

// SERIAL_MESSAGE* parseSerialMessage(uint8_t* data) {
    
// }

uint8_t serialMessageType(uint8_t* data) {
    return data[0];
}
