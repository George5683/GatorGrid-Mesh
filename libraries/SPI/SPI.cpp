#include "SPI.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
#include "hardware/spi.h"
#include "hardware/sync.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_MOSI 19
#define PIN_MISO_MASTER 16 
#define PIN_MOSI_MASTER 19
#define PIN_MISO_SLAVE 19
#define PIN_MOSI_SLAVE 16
#define PIN_CS   17
#define PIN_SCK  18

// Default constructor
SPI::SPI(){};

// Function to set master or slave
void SPI::Set_Master(bool master){
    is_master = master;
}

// Initiate for the master
bool SPI::SPI_init(){
    /**
    * Initialize the SPI module by passing in whether the node is a slave (false) or master (true)
    */
    if(is_master){
        printf("Setting up SPI Master\n");
        // Setting up the Master
        spi_init(SPI_PORT, 1000 * 1000);
        spi_set_slave(SPI_PORT, false);
    }
    else{
        printf("Setting up SPI Slave\n");
        // Set as slave
        spi_init(SPI_PORT, 1000 * 1000);
        spi_set_slave(SPI_PORT, true);
    }

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

    printf("SPI initialized successfully\n");
    return true;
};

void SPI::SPI_deinit_to_gpio() {
    // Wait for any in-flight transfer to finish
    while (spi_is_busy(SPI_PORT));

    // Disable the SPI peripheral
    spi_deinit(SPI_PORT);

    // Return pins to GPIO (SIO) and set as inputs with no pulls 
    gpio_set_function(PIN_MISO, GPIO_FUNC_SIO);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SIO);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);

    gpio_disable_pulls(PIN_MISO);
    gpio_disable_pulls(PIN_MOSI);
    gpio_disable_pulls(PIN_SCK);
    gpio_disable_pulls(PIN_CS);

    gpio_set_dir(PIN_MISO, GPIO_IN);
    gpio_set_dir(PIN_MOSI, GPIO_IN);
    gpio_set_dir(PIN_SCK,  GPIO_IN);
    gpio_set_dir(PIN_CS,   GPIO_IN);

    printf("SPI disabled\n");
}

int SPI::SPI_send_message(std::vector<uint8_t>& message){
    /**
    * Function to send a message 
    */

    if(is_master){
        // Master specific behavior

        gpio_init(PIN_MOSI_MASTER);

        // Set MOSI direction to output
        gpio_set_dir(PIN_MOSI_MASTER, GPIO_OUT);
        gpio_set_dir(PIN_MISO_MASTER, GPIO_IN);

        // Assert MOSI high
        gpio_put(PIN_MOSI_MASTER, 1);

        printf("Asserted MOSI High\n");

        // print status of MOSI
        printf("MOSI Status: %d\n", gpio_get(PIN_MOSI_MASTER));

        // short delay for stabilization
        //sleep_ms(10); 

        printf("Waiting for MISO to go High\n");

        // wait for MISO to be high
        while(!gpio_get(PIN_MISO_MASTER)) {
            puts("Master polling for slave to be ready");
        }

        printf("Deasserting MOSI\n");

        // Deassert MOSI
        gpio_put(PIN_MOSI_MASTER, 0);

        // short delay for stabilization
        sleep_ms(10);

        // Turn SPI on
        SPI_init();

        // Delay for Slave to initialize
        sleep_ms(110);

        printf("Sending Data: ");
        for(char c : message){
            printf("%c", c);
        }
        printf("\n");

        // Buffer for response
        std::vector<uint8_t> response(message.size());

        spi_write_read_blocking(SPI_PORT, message.data(), response.data(), message.size());

        while(spi_is_busy(SPI_PORT));

        printf("Data Sent!\n");
        printf("Response Data: ");
        for(char c : response){
            printf("%c", c);
        }
        printf("\n");

        // turn off SPI
        SPI_deinit_to_gpio();
    }
    else{
        // Slave specific behavior
        gpio_init(PIN_MISO_SLAVE);

        // Set MISO direction to output
        gpio_set_dir(PIN_MISO_SLAVE, GPIO_OUT);
        gpio_set_dir(PIN_MOSI_SLAVE, GPIO_IN);
        // Assert MISO high
        gpio_put(PIN_MISO_SLAVE, 1);
        printf("Asserted MISO High\n");
        // short delay for stabilization
        sleep_ms(10);

        printf("Waiting for MOSI to go High\n");
        // wait for MOSI to be high
        while(!gpio_get(PIN_MOSI_SLAVE));

        // Deassert MISO
        gpio_put(PIN_MISO_SLAVE, 0);

        // short delay for stabilization
        sleep_ms(10);

        // initialize SPI
        SPI_init();

        printf("Sending Data: ");
        for(char c : message){
            printf("%c", c);
        }
        printf("\n");

        // buffer for the response
        std::vector<uint8_t> response(message.size());

        spi_write_read_blocking(SPI_PORT, message.data(), response.data(), message.size());

        while(spi_is_busy(SPI_PORT));

        printf("Data Sent!\n");
        printf("Response Data: ");
        for(char c : response){
            printf("%c", c);
        }
        printf("\n");

        // turn off SPI
        SPI_deinit_to_gpio();

    }

    return 1;
};

void SPI::SPI_read_message(std::vector<uint8_t>& response){
    /**
    * Function to read a message that was sent from SPI by passing in a byte pointer and its size to store result. 
    */
    response = response_message;
};

bool SPI::SPI_POLL_MESSAGE(){
    /**
    * Function to poll for messages
    */

    puts("polling");

    if(is_master){
       // Master specific polling

       // Check if MISO is high
        if(gpio_get(PIN_MISO_MASTER)){
            gpio_init(PIN_MOSI_MASTER);
           // Assert MOSI to high
           gpio_set_dir(PIN_MOSI_MASTER, GPIO_OUT);
           gpio_put(PIN_MOSI_MASTER, 1);

           // short delay for stabilization
           sleep_ms(10);

           // Delay to wait for slave to initialize
           sleep_ms(110);

           SPI_init();

           // message of 32 char f
           std::vector<uint8_t> message = { 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f' };
           std::vector<uint8_t> response(message.size());

           spi_write_read_blocking(SPI_PORT, message.data(), response.data(), message.size());

            while(spi_is_busy(SPI_PORT));

            response_message = response;

            // deinitialize SPI
            SPI_deinit_to_gpio();

            return true;
       }
   }
   else{
       // Slave specific polling

       // Check if MOSI is high
       printf("Waiting for MOSI to go High\n");

       // print status of MOSI
       printf("MOSI Status: %d\n", gpio_get(PIN_MOSI_SLAVE));
       if(gpio_get(PIN_MOSI_SLAVE)){
           // Assert MISO to high
           gpio_init(PIN_MISO_SLAVE);
           gpio_set_dir(PIN_MISO_SLAVE, GPIO_OUT);
           gpio_put(PIN_MISO_SLAVE, 1);
            // print status of MISO
           printf("MISO Status: %d\n", gpio_get(PIN_MISO_SLAVE));

           // short delay
           sleep_ms(10);

           // init spi
           SPI_init();

           // message of 32 char f
           std::vector<uint8_t> message = { 'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
                                              'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f' };
           std::vector<uint8_t> response(message.size());

           spi_write_read_blocking(SPI_PORT, message.data(), response.data(), message.size());

            //while(spi_is_busy(SPI_PORT));

            response_message = response;

            // deinitialize SPI
            SPI_deinit_to_gpio();

            return true;
       }
   }
   return false;
}

// ____________________________________ Examples Below for Main _______________________________________

// Example for Master
// #include "SPI/SPI.hpp"
// #include <stdio.h>

// int main() {
//     stdio_init_all();

//     SPI Master_Pico;
    
//     // set as the master 
//     Master_Pico.SPI_init(true);
    
//     uint8_t send_string[] = "Fortnite!";
//     uint8_t recv_buffer[sizeof(send_string)] = {0};
    
//     while (true) {
//         Master_Pico.SPI_send_message(send_string, sizeof(send_string));
//         Master_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
//         sleep_ms(1000);  // Wait before next transmission
//     }
// }

// Example for Slave
// #include "SPI/SPI.hpp"
// #include <stdio.h>

// int main() {
//     stdio_init_all();

//     SPI Slave_Pico;

//     // set as the slave 
//     Slave_Pico.SPI_init(false);
    
//     uint8_t send_string[] = "Herobrine";
//     uint8_t recv_buffer[sizeof(send_string)] = {0};
    
//     while (true) {
//         Slave_Pico.SPI_read_message(recv_buffer, sizeof(recv_buffer));
//         Slave_Pico.SPI_send_message(send_string, sizeof(send_string));
//         sleep_ms(500);  // Wait before next transmission
//     }
// }
