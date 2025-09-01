#pragma once

#include "pico/stdlib.h"
#include <vector>

class SPI{
    public:
        // Default constructor
        SPI();
        // function to initiate SPI (true for master | false for slave)
        bool SPI_init();

        void Set_Master(bool master);

        // function to send a message 
        int SPI_send_message(std::vector<uint8_t>& message);

        // function to read a message
        void SPI_read_message(std::vector<uint8_t>& response);

        bool SPI_POLL_MESSAGE();

        void SPI_deinit_to_gpio();
    private:
        bool is_master;
        // vector for the response message
        std::vector<uint8_t> response_message;

};