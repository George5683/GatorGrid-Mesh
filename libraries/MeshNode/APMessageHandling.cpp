#include "MeshNode.hpp"
#include <cstdint>
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "SerialMessages.hpp"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"

#define DEBUG 0

#if DEBUG 
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

#if DEBUG
 static void dump_bytes(const uint8_t *bptr, uint32_t len) {
     unsigned int i = 0;
 
    DEBUG_printf("dump_bytes %d", len);
     for (i = 0; i < len;) {
         if ((i & 0x0f) == 0) {
            DEBUG_printf("\n");
         } else if ((i & 0x07) == 0) {
            DEBUG_printf(" ");
         }
        DEBUG_printf("%02x ", bptr[i++]);
     }
    DEBUG_printf("\n");
 }
 #define DUMP_BYTES dump_bytes
 #else
 #define DUMP_BYTES(A,B)
 #endif

