//Example for Slave
#include "libraries/SPI/SPI.hpp"
#include <stdio.h>
#include <vector>

// Test 1: Message from Master to Slave
bool Test1(SPI Slave_Pico){
    printf("Running Test 1\n");

    // set as the slave 
    Slave_Pico.Set_Master(false);

    printf("Set Slave\n");

    while(true){
        // constantly poll for a message
        printf("Polling for message from Master...\n");
        if(Slave_Pico.SPI_POLL_MESSAGE()){
            printf("Message received from Master\n");
            std::vector<uint8_t> response;
            Slave_Pico.SPI_read_message(response);

            printf("Received Message from Master: ");
            for(char c : response){
                printf("%c", c);
            }
            printf("\n");
            break; // Exit after receiving one message
        }
    }

    return true;
}

// Test 2: Message from Slave to Master
bool Test2(SPI &Slave_Pico) {
    printf("Running Test 2\n");

    // set as the slave
    Slave_Pico.Set_Master(false);

    // create the string to send
    std::vector<uint8_t> send_string = {'T', 'E', 'S', 'T', '2'};

    // Pad the string to be 32 bytes
    send_string.resize(32, 0);

    printf("About to Send to Master\n");

    Slave_Pico.SPI_send_message(send_string);

    printf("Message Sent from Slave to Master\n");

    return true;
}

int main() {
    stdio_init_all();

    // Delay to allow time to view serial monitor
    sleep_ms(10000);

    // Create the SPI object
    SPI Slave_Pico;

    printf("This is the Slave\n");

    // Run Test1
    Test1(Slave_Pico);

    // Run Test2
    Test2(Slave_Pico);

}