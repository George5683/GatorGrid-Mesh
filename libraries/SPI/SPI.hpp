#pragma once

#include "pico/stdlib.h"

class SPI{
    public:
        bool is_master;
        // Default constructor
        SPI();
        // function to initiate SPI (true for master | false for slave)
        bool SPI_init(bool mode);

        // function to send a message 
        int SPI_send_message(uint8_t *message, size_t length);

        // function to read a message
        int SPI_read_message(uint8_t *buffer, size_t buffer_size);

        bool SPI_is_write_available();

        bool SPI_is_read_available();
};