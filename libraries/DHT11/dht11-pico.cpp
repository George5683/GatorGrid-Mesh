/**
 * @file dht11-pico.cpp
 *
 * @brief DHT11 Sensor Library Implementation for Raspberry Pi Pico
 *
 * This file contains the implementation of the DHT11 sensor library for Raspberry Pi Pico,
 * which includes the initialization, reading, and retrieval of temperature and humidity values
 * from the DHT11 sensor.
 */

#include "dht11-pico.h"

Dht11::Dht11(uint pin){
    gpioPin=pin;
    gpio_init(pin);
    sleep_ms(1000);
}

Dht11::~Dht11(){
    gpio_deinit(gpioPin);
}

long long Dht11::read(){
    int count=0;
    long long raw=0;
    gpio_set_dir(gpioPin, GPIO_OUT);
    gpio_put(gpioPin,0);
    sleep_ms(20);
    gpio_set_dir(gpioPin, GPIO_IN);

    while(gpio_get(gpioPin)==1){
        count++;
        sleep_us(5);
        if(count==POLLING_LIMIT){
            return TRANSMISSION_ERROR;
        }
    }

    count=0;
    while(gpio_get(gpioPin)==0){
        count++;
        sleep_us(5);
        if(count==POLLING_LIMIT){
            return TRANSMISSION_ERROR;
        }
    }

    count=0;
    while(gpio_get(gpioPin)==1){
        count++;
        sleep_us(5);
        if(count==POLLING_LIMIT){
            return TRANSMISSION_ERROR;
        }
    }

    //transmission start
    for(int i=0;i<40;i++){
        count=0;
        //~50us
        while(gpio_get(gpioPin)==0){
            sleep_us(5);
        }
        //bit 0 or 1
        while(gpio_get(gpioPin)==1){
            sleep_us(5);
            count++;
        }
        if(count>=THRESHOLD){
            raw|=1;
        }
        raw=raw<<1;
    }
    //check if raw data is valid
    if(
        ((raw & RH_INT_MASK)>>32)   + 
        ((raw & RH_DEC_MASK)>>24)   +
        ((raw & TEMP_INT_MASK)>>16) +
        ((raw & TEMP_DEC_MASK)>>8)  -
        ((raw & CHECKSUM_MASK))     > 1
        ){
        return TRANSMISSION_ERROR;
    }    
    return raw;
}

double Dht11::readT(){
    long long raw=read();
    if(raw==TRANSMISSION_ERROR){
        return TRANSMISSION_ERROR;
    }
    int temp_int    =    (raw & TEMP_INT_MASK)>>16;
    uint temp_dec   =    (raw & TEMP_DEC_MASK)>>8;
    if(temp_int<0)
        temp_dec=-temp_dec;
    double temp     =    temp_int + 0.1*temp_dec; 
    return temp;
}

double Dht11::readRH(){
    long long raw=read();
    if(raw==TRANSMISSION_ERROR){
        return TRANSMISSION_ERROR;
    }
    int rh_int    =    (raw & RH_INT_MASK)>>32;
    uint rh_dec   =    (raw & RH_DEC_MASK)>>24;
    double rh     =    rh_int + 0.1*rh_dec; 
    return rh;
}

void Dht11::readRHT(double *temp, double *rh){
    long long raw=read();
    if(raw==TRANSMISSION_ERROR){
        *temp= *rh = TRANSMISSION_ERROR;
        return;
    }
    int temp_int    =    (raw & TEMP_INT_MASK)>>16;
    uint temp_dec   =    (raw & TEMP_DEC_MASK)>>8;
    int rh_int      =    (raw & RH_INT_MASK)>>32;
    uint rh_dec     =    (raw & RH_DEC_MASK)>>24;
    
    *temp           =    temp_int + 0.1*temp_dec;
    *rh             =    rh_int + 0.1*rh_dec; 
}

/*
 * An example program that demonstrates the usage of the DHT11 sensor library
 * for Raspberry Pi Pico. It initializes the DHT11 sensor, reads temperature 
 * and humidity values and prints them to the console.
 */
// #include "pico/stdlib.h"
// #include <iostream>

// #include "libraries/DHT11/dht11-pico.h"

// const uint DHT_PIN = 22;

// int main() {
//     stdio_init_all();
//     sleep_ms(2000);
//     Dht11 dht11_sensor(DHT_PIN);
//     double temperature;
//     double rel_humidity;
//     while(1){
//     std::cout<<"Temp:"<<dht11_sensor.readT()<<" °C"<<std::endl;
//     sleep_ms(1000);
//     std::cout<<"RH:"<<dht11_sensor.readRH()<<" %"<<std::endl;
//     sleep_ms(1000);
//     dht11_sensor.readRHT(&temperature, &rel_humidity);
//     std::cout<<"Temp:"<<temperature<<"°C"<<"   RH:"<<rel_humidity<<" %"<<std::endl;
//     sleep_ms(1000);
//     }
//     return 0;
// }