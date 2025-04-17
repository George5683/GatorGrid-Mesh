# SPI Library 

The SPI library is used in order to transfer data between Picos within a node. It consists of a slave and a master. 

## Features
- Configuring master
- Configuring slave
- Sending messages bidirectionally between master and slave
- Check if communication between nodes is ready

## Functions Overview

### SPI

### void SPI_init(bool mode)
Initialize the SPI module by passing in whether the node is a slave (false) or master (true)

### int SPI_send_message(uint8_t *message, size_t length)
Function to send a message by passing in the message as a byte pointer and its size

### int SPI_read_message(uint8_t *buffer, size_t buffer_size)
Function to read a message that was sent from SPI by passing in a byte pointer and its size to store result. 

### bool SPI_is_write_available()
Function to check if the bus is free for writing a message, for master checks if the CS pin is High to indicate it is available. For slave chekcs if the CS pin is Low to indicate it is available.

### bool SPI_is_read_available()
Function to check if the bus is idle for reading a message, for master checks if the CS pin is High and the spi is not busy. For slave chekcs if the CS pin is Low and the spi is not busy.

## Authors
George Salomon, Cole Smith, Jonathon Brown, Mateo Slivka, Maxwell Evans
