# include "cyw43.hpp"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

// Initialize the driver for the wifi
bool cyw43::init(){
    cyw43_arch_init();
}

// MUST BE IN STA TO CONNECT TO A NETWORK
bool cyw43::sta_mode_enable(){

}

// MUST BE IN AP MODE TO HOST A NETWORK
bool cyw43::ap_mode_enable(){

}

// Enable bluetooth 
bool cyw43::bluetooth_enable(){
    // To enable bluetooth, you need to turn it on in the config of cyw43.h 
    //  (0)
    // or 
    // add this to cmake
    // add_compile_definitions(CYW43_ENABLE_BLUETOOTH=1)
    
    
    return true;
}


int main(){
    // initialize everything
    stdio_init_all();

}