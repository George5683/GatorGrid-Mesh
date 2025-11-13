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

extern "C" {
    #include "TicTacToe.h"
    #include "images.h"
    #include "DEV_Config.h"
    #include "GUI_Paint.h"
    #include "OLED_1in3_c.h"
}


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
    // sleep_ms(10000);

    // i2c_init(i2c_default, 400 * 1000);
    // gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    // gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    // gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    // gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    // mpu6050_reset();

    // int16_t acceleration[3], gyro[3], temp;

    //SPI spi;
    // STANode node;
    // node.init_sta_mode();
    // node.start_sta_mode();

    // cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    // if (!node.scan_for_nodes()) {
    //     return 0;
    // }
    // printf("known nodes map size: %d\n", node.known_nodes.size());
    // while (node.known_nodes.empty()) {
    //     //printf("known nodes map size: %d", node.known_nodes.size());
    //     if (!node.scan_for_nodes()) {

    //         return 0;
    //     }
    // }
    // sleep_ms(5000);
    // printf("Left searching for nodes\n");

    // while(!node.connect_to_node(0));
    // if (!node.tcp_init()) {
    //     // Failed to init TCP connection
    //     while(true);
    // }
    if(DEV_Module_Init()!=0){
        while(1){
            printf("END\r\n");
        }
    }
    // int count = 0;
    OLED_1in3_C_Init();
    OLED_1in3_C_Clear();

    // bool toggle = true;
    // uint32_t send_count = 0;
    UBYTE *BlackImage;
    UWORD Imagesize = ((OLED_1in3_C_WIDTH%8==0)? (OLED_1in3_C_WIDTH/8): (OLED_1in3_C_WIDTH/8+1)) * OLED_1in3_C_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        while(1){
            printf("Failed to apply for black memory...\r\n");
        }
    }

    Paint_NewImage(BlackImage, OLED_1in3_C_WIDTH, OLED_1in3_C_HEIGHT, 180, WHITE);	
    Paint_Clear(BLACK);
    int key0 = 15; 
    int key1 = 17;
    DEV_GPIO_Mode(key0, 0);
    DEV_GPIO_Mode(key1, 0);
    
    Paint_Clear(BLACK);
    OLED_1in3_C_Display(BlackImage);



    NetworkTTTGame TicTacToe(1);
    object o = O;
    bool key0_flag = false;
    bool key1_flag = false;
    DEBUG_printf("Entering loop");
    for (;;) {
        // DEBUG_printf("Before poll");
        // node.poll();

        // if (count++ >= 1000) {
        //     // DEBUG_printf("Before LED toggle");
        //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
        //     toggle = !toggle;
        //     if(!node.is_connected()) {
        //         printf("Not connected!\n");
        //     }

        //     count = 0;
        // }

        // sleep_ms(1);
        
        Paint_DrawBitMap(epd_bitmap_tictactoe);

        TicTacToe.draw_selector();

        if(DEV_Digital_Read(key0) == 0 && !key0_flag){
            TicTacToe.increment_position();
            key0_flag = true;
            
        } else if (DEV_Digital_Read(key0) == 1) {
            key0_flag = false;
        }

        if(DEV_Digital_Read(key1) == 0 && !key1_flag){
            pos_cords pos = TicTacToe.get_position();
            if (TicTacToe.game.placeObject(o, pos.x, pos.y)) {
                
                if (o == O) {
                    o = X;
                } else {
                    o = O;
                }
            }

            key1_flag = true;
            
        } else if (DEV_Digital_Read(key1) == 1) {
            key1_flag = false;
        }

        // if(DEV_Digital_Read(key1) == 0 && !key1_flag){
        //     key1_flag = true;
        //     TicTacToe.increment_position();
        // } else {
        //     key1_flag = false;
        // }

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                TicTacToe.draw_peice(TicTacToe.game.grid[i][j], i, j);
            }
        }

        switch(TicTacToe.game.checkWin()) {
            case 1: 
                TicTacToe.game.restartGame();
                break;
            case 2:
                TicTacToe.game.restartGame();
                break;
            default:
                if (TicTacToe.game.placed_pieces == 9) {
                    TicTacToe.game.restartGame();
                }
                break;
        }

        OLED_1in3_C_Display(BlackImage);
        Paint_Clear(BLACK);
        
    }

    return 0;
}