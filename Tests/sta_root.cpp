#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdint>
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "display.hpp"

extern "C" {
    //#include "TicTacToe.h"
    #include "images.h"
    #include "DEV_Config.h"
    #include "GUI_Paint.h"
    #include "OLED_1in3_c.h"
}

#define DEBUG 1


int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(10000);

    // initiate everything


    //SPI spi;
    STANode node;
    node.init_sta_mode();
    node.start_sta_mode();
    node.is_root = true;
    node.state = (TCP_CLIENT_T*)calloc(1, sizeof(TCP_CLIENT_T));

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    
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
    // printf("Left searching for nodes\n");
    // if (node.connect_to_node(1)) {
    //     node.tcp_init();
    // }

    // sleep_ms(1000);


    // --- init screen ---

    OLED_1in3_C_Init();
    OLED_1in3_C_Clear();

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

    // --- end init screen ---




    bool got_an_ack = false;
    int count = 0;
    int final_size = 0;

    char send_buf[50] = {0};

    bool toggle = true;


    uint32_t children_ids[4] = {};
    uint8_t number_of_children = 0;

    char hullo[] = {"Hello from the root!"};

    struct data tictactoe_message = {0};
    uint win_count[6] = {0};
    
    for (;;) {

        // DEBUG_printf("Loop start\n");
        node.poll();

        if (count == 500) {
            if(node.rb.get_size() >= 1) {
                tictactoe_message = node.rb.digest();

                printf("\n%d has won a game!\n", tictactoe_message.source);

                win_count[tictactoe_message.source]++;

                printf("--- SCORE BOARD ---\n"
                       "Node 1: %d\n"
                       "Node 2: %d\n"
                       "Node 4: %d\n"
                       "Node 5: %d\n"
                       "------- END -------\n", 
                        win_count[1],
                        win_count[2],
                        win_count[4],
                        win_count[5]);
                }


            // if (node.tree.node_exists(2)) {
            //     TCP_DATA_MSG msg(node.get_NodeID(), 2);
            //     msg.add_message((uint8_t*)hullo, sizeof(hullo));
            //     node.send_msg(msg.get_msg());
            // }
        }

        if (count++ >= 1000) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;
            count = 0;
        }

        sleep_ms(1);
    }

    return 0;
}