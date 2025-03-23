# SPI Library 

The SPI library is used in order to transfer data between Picos within a node. it consists of a slave and a master. 

## Features
- Configuring master
- Configuring slave

## Functions Overview

### **SPI**

### `void SPI_init(bool mode)`
Initialize the SPI module by passing in whether the node is a slave (false) or master (true)

### `int SPI_send_message(uint8_t *message, size_t length)`
Function to send a message by passing in the message as a byte pointer and its size

### `int SPI_read_message(uint8_t *buffer, size_t buffer_size)`
Function to read a message that was sent from SPI by passing in a byte pointer and its size to store result. 

## Author
George Salomon, Cole Smith, Jonathon Brown, Mateo Slivka, Maxwell Evans
