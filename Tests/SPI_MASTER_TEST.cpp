//Example for Master
#include "libraries/SPI/SPI.hpp"
#include <stdio.h>
#include <vector>

// Test 1: Message from Master to Slave
bool Test1(SPI &Master_Pico){
    printf("Running Test 1\n");

    // set as the master
    Master_Pico.Set_Master(true);

    printf("Set Master\n");

    // create the string to send
    std::vector<uint8_t> send_string = {'T', 'E', 'S', 'T', '1'};

    // Pad the string to be 32 bytes
    send_string.resize(32, 0);

    printf("About to Send to Slave\n");

    Master_Pico.SPI_send_message(send_string);

    printf("Message Sent from Master to Slave\n");

    return true;
}

// Test 2: Message from Slave to Master
bool Test2(SPI &Master_Pico){
    printf("Running Test 2\n");

    // set as the Master
    Master_Pico.Set_Master(true);

    while(true){
        // constantly poll for a message
        if(Master_Pico.SPI_POLL_MESSAGE()){
            std::vector<uint8_t> response;
            Master_Pico.SPI_read_message(response);

            printf("Received Message from Slave: ");
            for(char c : response){
                printf("%c", c);
            }
            printf("\n");
            break; // Exit after receiving one message
        }
    }

    return true;
}

int main() {
    stdio_init_all();

    // Delay to allow time to view serial monitor
    sleep_ms(10000);

    SPI Master_Pico;

    printf("This is the Master\n");

    // Run Test1
    Test1(Master_Pico);

    // Run Test2
    Test2(Master_Pico);
}