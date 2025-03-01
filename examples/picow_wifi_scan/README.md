# Driver Embedded Lab

This project integrates the LIS3DH accelerometer with a NeoMatrix LED display on a Raspberry Pi Pico W. It reads acceleration data and visually represents the movement using LEDs along with indicating when level.

## Features
- Reads acceleration data from the LIS3DH sensor.
- Controls a NeoMatrix LED display 
- Level program

## Run
1. Unzip the folder:
   ```sh
   unzip level.zip
   ```
2. Change the directory to the driver folder:
    ```sh
   cd /driver
   ```
3. Build the project using CMake:
    ```sh
   mkdir build && cd build
   cmake ..
   make
   ```
4. Flash the compiled binary to the Raspberry Pi Pico W.

## Functions Overview

### **LIS3DH**

### `LIS3DH()`
Constructor that initializes the x, y, and z values to 0

### `init()`
Initializes the accelerometer.

### `set_reg(uint8_t reg, uint8_t val)`
Set a register to a specific value

### `read_reg(uint8_t reg)`
read a register and output the value

### `update()`
Updates the x, y, and z values in the class with the accelerometer values.

### **NeoMatrix**

### `NeoMatrix(uint8_t width, uint8_t height)`
Constructor to set the width and height of the matrix

### `init()`
Initialize the NeoMatrix to get ready to shine

### `set_pixel(uint8_t row, uint8_t col, uint32_t color)`
set a pixel, within the 2D vector buffer in the class, to a specific color

### `clear_pixels()`
clear the pixels in the buffer

### `write()`
write the buffer data to the Matrix

### `urgb_u32(uint8_t r, uint8_t g, uint8_t b)`
Helper function to convert the values to a 32 bit value

## Author
George Salomon
