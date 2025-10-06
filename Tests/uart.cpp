#include "libraries/UART/UART.hpp"

int main() {
    stdio_init_all();

    PicoUART uart;
    uart.picoUARTInit();
    uart.picoUARTInterruptInit();

    const char* testMessage = "Hello, UART!\n";
    uart.sendMessage(testMessage);

    while (true) {
        // Main loop can perform other tasks
        sleep_ms(1000);
    }

    return 0;
}