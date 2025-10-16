#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdint>
#include <cstdio>
#include "pico/cyw43_arch.h"

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "display.hpp"


// By default these devices  are on bus address 0x68
static int addr = 0x68;
 
#ifdef i2c_default
static void mpu6050_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(i2c_default, addr, buf, 2, false);
    sleep_ms(100); // Allow device to reset and stabilize

    // Clear sleep mode (0x6B register, 0x00 value)
    buf[1] = 0x00;  // Clear sleep mode by writing 0x00 to the 0x6B register
    i2c_write_blocking(i2c_default, addr, buf, 2, false); 
    sleep_ms(10); // Allow stabilization after waking up
}

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c_default, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(i2c_default, addr, &val, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 6, false);  // False - finished with bus

    for (int i = 0; i < 3; i++) {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(i2c_default, addr, &val, 1, true);
    i2c_read_blocking(i2c_default, addr, buffer, 2, false);  // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}
#endif

int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(5000);

    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    mpu6050_reset();

    int16_t acceleration[3], gyro[3], temp;

    //SPI spi;
    STANode node;
    node.init_sta_mode();
    node.start_sta_mode();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    /*while(true) {
        mpu6050_read_raw(acceleration, gyro, &temp);
        printf("Current Temp (C) = %f\n", ((temp / 340.0) + 36.53));
        sleep_ms(1000);
    }*/

    printf("Starting scan...\n");
    if (!node.scan_for_nodes()) {
        return 0;
    }
    //printf("known nodes map size: %d\n", node.known_nodes.size());
    while (!node.connect_to_node(2)) {
        //printf("known nodes map size: %d", node.known_nodes.size());
        printf("Looking for node 2\n");
        if (!node.scan_for_nodes()) {
            return 0;
        }
    }

    node.tcp_init();

   char format_string[50] = {0};
   int final_size;
   bool got_an_ack = false;

    bool toggle = true;
    uint32_t count = 0;
    
    for (;;) {
        node.poll();

        if (count++ %  500 == 0) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;

            if (node.number_of_messages() > 0) {
                struct data tmp = node.digest_data();
                DEBUG_printf("Received message from node %08x: %s", tmp.source, (char*)tmp.data);
                if(tmp.data[0] == 1) {
                    // set LED on
                    gpio_put(1, 1);

                } else if(tmp.data[0] == 0) {
                    // set LED OFF
                    gpio_put(1, 0);
                }
            }
        }

        if (count == 5000) {
            mpu6050_read_raw(acceleration, gyro, &temp);

            final_size = snprintf(format_string, 50, "X Accel = %d, Y Accel = %d, Z Accel = %d", gyro[0], gyro[1], gyro[2]);

            if(final_size < 0)
                puts("Size is neg error snprintf");

            // DEBUG_printf("Sending \"Current Temp (C) = %f\" to node %08x\n", ((temp / 340.0) + 36.53), 1);

            got_an_ack = (node.send_data(0, final_size, (uint8_t*)format_string) == 0);
            count = 0;
        }

        // else
        // // Toggle LEDS
        // //sleep_ms(1000);
        // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        // toggle = !toggle;
        // if(!node.is_connected()) {
        //     printf("Not connected!\n");
        // } else {

        //     while (node.number_of_messages() == 0) {
        //         sleep_ms(2);
        //     }

        //     struct data tmp = node.digest_data();
        //     DEBUG_printf("Received message from node %08x: %s", tmp.source, (char*)tmp.data);
        //     if(tmp.data[0] == 1) {
        //         // set LED on
        //         gpio_put(1, 1);

        //     } else if(tmp.data[0] == 0) {
        //         // set LED OFF
        //         gpio_put(1, 0);
        //     }

        //     sleep_ms(500);

        //     mpu6050_read_raw(acceleration, gyro, &temp);

        //     final_size = snprintf(format_string, 50, "Current Temp (C) = %f", ((temp / 340.0) + 36.53));

        //     if(final_size < 0)
        //         puts("Size is neg error snprintf");

        //     DEBUG_printf("Sending \"Current Temp (C) = %f\" to node %08x\n", ((temp / 340.0) + 36.53), 1);

        //     got_an_ack = (node.send_data(0, final_size, (uint8_t*)format_string) == 0);
        // }

        sleep_ms(1);
    }

    return 0;
}