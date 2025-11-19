#include "pico/stdlib.h"
#include "libraries/MeshNode/MeshNode.hpp"
#include "libraries/MeshNode/Messages.hpp"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "libraries/RingBuffer/RingBuffer.hpp"

#include "libraries/display/display.hpp"
#include "libraries/DHT11/dht11-pico.h"

extern "C" {
    #include "TicTacToe.h"
    #include "images.h"
    #include "DEV_Config.h"
    #include "GUI_Paint.h"
    #include "OLED_1in3_c.h"
}

const uint DHT_PIN = 22;

#define DEBUG 1

int main() {
    stdio_init_all();
    // initial delay to allow user to look at the serial monitor
    sleep_ms(5000);

    STANode node;

    
    

    // initiate everything


    //SPI spi;
    
    node.init_sta_mode();
    node.start_sta_mode();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    
    if (!node.scan_for_nodes()) {
        return 0;
    }
    printf("known nodes map size: %d\n", node.known_nodes.size());
    while (node.known_nodes.empty()) {
        //printf("known nodes map size: %d", node.known_nodes.size());
        if (!node.scan_for_nodes()) {

            return 0;
        }
    }
    printf("Left searching for nodes\n");

    while(!node.connect_to_node(0));
    while (!node.tcp_init()) {
        ERROR_printf("Failed to init connection... Retrying");
    }


    if(DEV_Module_Init()!=0){
        while(1){
            printf("END\r\n");
        }
    }
    int count = 0;
    bool toggle = true;


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


    const int game_id = 1;
    NetworkTTTGame TicTacToe(game_id);
    object o = X;
    bool key0_flag = false;
    bool key1_flag = false;
    DEBUG_printf("Entering loop");

    TCP_DATA_MSG game_updates(node.get_NodeID(), 2);
    SERIAL_DATA_MESSAGE test_serial;
    // std::string test_state = "011,220,102";
    // TicTacToe.game.updateFromString(test_state);
    TicTacToe.game.is_my_turn = false;

    bool reset_flag = false;
    // int win_wait = 0;
    bool flag = false;
    bool update_flag = false;
    absolute_time_t win_wait;

    TicTacToe.game.createNetworkMessage(game_updates, restart);
    node.send_msg(game_updates.get_msg());

    for (;;) {
        // DEBUG_printf("Before poll");
        node.poll();

        // if(node.uart.BufferReady()) {
        //     node.handle_serial_message(node.uart.getReadBuffer());
        // }

        if (count++ >= 1000) {
            // DEBUG_printf("Before LED toggle");
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, toggle);
            toggle = !toggle;
            // if(!node.is_connected()) {
            //     printf("Not connected!\n");
            // }

            count = 0;
        }

        // sleep_ms(1);
        switch (TicTacToe.game.checkWin()) {
            
            case 1:
                if (o == 1) {
                    Paint_DrawBitMap(epd_bitmap_win);
                } else {
                    Paint_DrawBitMap(epd_bitmap_lose);
                }

                if (!flag) { win_wait = get_absolute_time(); flag = true; }
                
                if(DEV_Digital_Read(key0) == 0 || DEV_Digital_Read(key1) == 0) {
                    reset_flag = true;
                } else {
                    if (reset_flag) {
                        if (get_absolute_time() - win_wait > 1000) {
                            TicTacToe.game.restartGame();
                            flag = false;
                        }
                    }
                }
                break;
            case 2:
                if (o == 2) {
                    Paint_DrawBitMap(epd_bitmap_win);
                } else {
                    Paint_DrawBitMap(epd_bitmap_lose);
                }

                if (!flag) {
                    TicTacToe.game.createNetworkMessage(game_updates, victory);
                    update_flag = true;
                    win_wait = get_absolute_time(); 
                    flag = true;
                }

                if(DEV_Digital_Read(key0) == 0 || DEV_Digital_Read(key1) == 0) {
                    reset_flag = true;
                } else {
                    if (reset_flag) {
                        if (win_wait - get_absolute_time() > 1000) {
                            TicTacToe.game.restartGame();
                            flag = false;
                        }
                    }
                }
                break;
            default:
            
                if (TicTacToe.game.placed_pieces == 9) {
                    TicTacToe.game.restartGame();
                    break;
                }
                Paint_DrawBitMap(epd_bitmap_tictactoe);


                if (TicTacToe.game.is_my_turn) {

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
                            
                            TicTacToe.game.createNetworkMessage(game_updates, update);
                            DEBUG_printf(TicTacToe.game.currentState().c_str());
                            // test_serial.add_message((uint8_t*)TicTacToe.game.currentState().c_str(), TicTacToe.game.currentState().length());
                            // node.uart.sendMessage((char*)test_serial.get_msg());
                            DUMP_BYTES(game_updates.get_msg(), game_updates.get_len());
                            update_flag = true;
                            TicTacToe.game.is_my_turn = false;
                            key1_flag = true;
                        }
                        
                        
                    } else if (DEV_Digital_Read(key1) == 1) {
                        key1_flag = false;
                    }

                } else {
                    
                }


                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        TicTacToe.draw_peice(TicTacToe.game.grid[i][j], i, j);
                    }
                }
                break;
        }

        if (update_flag) {
            if (node.send_msg(game_updates.get_msg()) != ERR_OK) {
                ERROR_printf("Failed to send update msg\n");
            } else {
                update_flag = false;
            }
        }

        if(node.rb.get_size()) {
            struct data r= node.rb.digest();
            if (r.data[0] != game_id) continue;
            DUMP_BYTES(r.data, r.size);

            std::string game_state;
            

            switch ((msg_type)r.data[1]) {
                case update:
                    for (int i = 2; i < 13; i++) {
                        game_state += (char)r.data[i];
                    }
                    std::cout << game_state << "\n";
                    TicTacToe.game.updateFromString(game_state);
                    TicTacToe.game.is_my_turn = true;
                    break;

                case restart:
                    TicTacToe.game.restartGame();
                    TicTacToe.game.is_my_turn = !(r.data[2]); // Opponent saying it is or is not their turn
                    break;

                default:
                    break;
            }
        }

        if (update_flag) {
            Paint_SetPixel(2,2,0xF0);
        } if (!node.is_connected()) {
            Paint_SetPixel(2, 4, 0x0F);
        }
        

        OLED_1in3_C_Display(BlackImage);
        Paint_Clear(BLACK);
        
    }

    return 0;
}