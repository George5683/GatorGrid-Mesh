/**
 * @file dht11-pico.h
 *
 * @brief DHT11 Sensor Library Header for Raspberry Pi Pico
 *
 * This file contains the class definition and constants for the DHT11 sensor library
 * for Raspberry Pi Pico, which provides functionality to read temperature and humidity
 * values from the DHT11 sensor.
 */

#ifndef DHT11_PICO_H
#define DHT11_PICO_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define POLLING_LIMIT 1000
#define THRESHOLD 4
#define TRANSMISSION_ERROR -999

// Bit masks for extracting data from raw reading
#define RH_INT_MASK     0xFF00000000LL
#define RH_DEC_MASK     0x00FF000000LL
#define TEMP_INT_MASK   0x0000FF0000LL
#define TEMP_DEC_MASK   0x000000FF00LL
#define CHECKSUM_MASK   0x00000000FFLL

class Dht11 {
private:
    uint gpioPin;
    long long read();

public:
    /**
     * @brief Constructor for DHT11 sensor
     * @param pin GPIO pin number where DHT11 is connected
     */
    Dht11(uint pin);
    
    /**
     * @brief Destructor for DHT11 sensor
     */
    ~Dht11();
    
    /**
     * @brief Read temperature from DHT11 sensor
     * @return Temperature in Celsius, or TRANSMISSION_ERROR if read failed
     */
    double readT();
    
    /**
     * @brief Read relative humidity from DHT11 sensor
     * @return Relative humidity in percentage, or TRANSMISSION_ERROR if read failed
     */
    double readRH();
    
    /**
     * @brief Read both temperature and humidity from DHT11 sensor
     * @param temp Pointer to store temperature value
     * @param rh Pointer to store relative humidity value
     */
    void readRHT(double *temp, double *rh);
};

#endif // DHT11_PICO_H