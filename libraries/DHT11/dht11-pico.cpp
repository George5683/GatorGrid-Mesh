#include "pico/stdlib.h"
#include <limits>
#include <cmath> 

// dht11-pico.cpp
#include "dht11-pico.h"
#include "hardware/sync.h"

#ifndef NAN
#  define NAN (std::numeric_limits<double>::quiet_NaN())
#endif




static inline bool wait_level(uint pin, bool level, uint32_t timeout_us) {
    absolute_time_t deadline = make_timeout_time_us(timeout_us);
    while (gpio_get(pin) != level) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) return false;
    }
    return true;
}

Dht11::Dht11(uint pin) : gpioPin(pin) {
    gpio_init(pin);
    gpio_pull_up(pin);               // DHT11 requires pull-up
    gpio_set_dir(pin, GPIO_IN);
    sleep_ms(1000);
}
Dht11::~Dht11(){ gpio_deinit(gpioPin); }

bool Dht11::read_bytes(uint8_t out[5]) {
    // 1) Start signal
    gpio_set_dir(gpioPin, GPIO_OUT);
    gpio_put(gpioPin, 0);
    sleep_ms(20);                    // ≥18 ms
    gpio_put(gpioPin, 1);
    sleep_us(30);
    gpio_set_dir(gpioPin, GPIO_IN);  // release
    gpio_pull_up(gpioPin);

    // 2) DHT response: ~80us low then ~80us high
    if (!wait_level(gpioPin, 0, 200)) return false;
    if (!wait_level(gpioPin, 1, 200)) return false;
    if (!wait_level(gpioPin, 0, 200)) return false; // start of first bit window

    // 3) Read 40 bits by timing the high pulse
    uint32_t irq_save = save_and_disable_interrupts(); // avoid preemption noise
    uint8_t bytes[5] = {0,0,0,0,0};
    for (int bit = 0; bit < 40; ++bit) {
        // Each bit starts with ~50us low
        if (!wait_level(gpioPin, 1, 100)) { restore_interrupts(irq_save); return false; }

        // Measure length of the subsequent high pulse
        absolute_time_t t0 = get_absolute_time();
        if (!wait_level(gpioPin, 0, 200)) { restore_interrupts(irq_save); return false; }
        int high_us = (int)absolute_time_diff_us(t0, get_absolute_time()); // positive

        // DHT11: ~26–28us = 0, ~70us = 1. Use mid-threshold ~50us.
        int byte_index = bit / 8;
        bytes[byte_index] <<= 1;
        if (high_us > 50) bytes[byte_index] |= 1;
    }
    restore_interrupts(irq_save);

    // 4) Checksum: (b0 + b1 + b2 + b3) & 0xFF == b4
    uint8_t sum = (uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    if ((sum & 0xFF) != bytes[4]) return false;

    // DHT11 spec: bytes[1] and bytes[3] (decimals) are 0
    for (int i = 0; i < 5; ++i) out[i] = bytes[i];
    return true;
}

double Dht11::readT() {
    uint8_t d[5];
    if (!read_bytes(d)) return NAN;
    // DHT11 temperature is integer °C in d[2]
    return (double)((int8_t)d[2]); // handles 0–50°C (signed cast safe)
}

double Dht11::readRH() {
    uint8_t d[5];
    if (!read_bytes(d)) return NAN;
    // DHT11 humidity is integer % in d[0]
    return (double)d[0];
}

void Dht11::readRHT(double *temp, double *rh) {
    uint8_t d[5];
    if (!read_bytes(d)) { *temp = *rh = NAN; return; }
    *temp = (double)((int8_t)d[2]);
    *rh   = (double)d[0];
}