#include "MeshNode.hpp"
#include "pico/stdlib.h"

// Main function for testing
int main(){
    MeshNode Node; 
    Node.init_ap_mode();
    while(true){
        // interrupt in the background
        sleep_ms(1000);
    }

}