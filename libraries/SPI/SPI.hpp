#pragma once

#include "pico/stdlib.h"
#include <vector>

class SPI{
    public:
        bool is_master;
        // Default constructor
        SPI();
        // function to initiate SPI (true for master | false for slave)
        bool SPI_init(bool mode);

        // function to send a message 
        int SPI_send_message_read_message(std::vector<uint8_t>& message, std::vector<uint8_t>& response);

        // function to read a message
        int SPI_read_message(std::vector<uint8_t>& message, std::vector<uint8_t>& response);

        bool SPI_is_write_available();

        bool SPI_is_read_available();
};