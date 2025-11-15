#include "TicTacToe.h"
#include <cstdint>
#include <pico/types.h>

extern "C" {
    #include "images.h"
    #include "DEV_Config.h"
    #include "GUI_Paint.h"
}

TTTGame::TTTGame() {
    ;
}

bool TTTGame::placeObject(object o, int x, int y) {
    if (EMPTY == grid[x][y]) {
        grid[x][y] = o;
        placed_pieces++;
        return true;
    }
    return false;
}

uint8_t TTTGame::checkWin() {
    object check = EMPTY;

    if (EMPTY != grid[0][0]) {
        check = grid[0][0];

        if (check == grid[0][1] && check == grid[0][2]) {
            return check;
        } else if (check == grid[1][0] && check == grid[2][0]) {
            return check;
        } else if (check == grid[1][1] && check == grid[2][2]) {
            return check;
        }
    }

    if (EMPTY != grid[1][0]) {
        check = grid[1][0];

        if (check == grid[1][1] && check == grid[1][2]) {
            return check;
        }
    }

    if (EMPTY != grid[0][1]) {
        check =  grid[0][1];

        if (check == grid[1][1] && check == grid[2][1]) {
            return check;
        }
    }

    if (EMPTY != grid[2][0]) {
        check = grid[2][0];

        if (check == grid[2][1] && check == grid[2][2]) {
            return check;
        } else if (check == grid[1][1] && check == grid[0][2]) {
            return check;
        }
    }

    if (EMPTY != grid[0][2]) {
        check = grid[0][2];

        if (check == grid[1][2] && check == grid[2][2]) {
            return check;
        }
    }

    return 0;
}


std::string TTTGame::currentState() {
    std::string gameState;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            gameState += std::to_string(grid[j][i]);
        }
        if (i != 2) gameState += ",";
    }

    return gameState;
}

void TTTGame::updateFromString(std::string state) {
    int x_pos = 0;
    int y_pos = 0;
    placed_pieces = 0;
    for (int i = 0; i < state.length(); i++) {
        if (state.at(i) == ',') continue;
        if ((object)(state.at(i) - '0') != EMPTY) placed_pieces++;
        grid[x_pos][y_pos] = (object)(state.at(i) - '0');
        if (++x_pos == 3) {
            x_pos = 0;
            y_pos++;
        }
        if (y_pos == 3) {
            y_pos = 0;
        }
              
    }
}

void TTTGame::restartGame() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            grid[j][i] = EMPTY;
        }
    }
    placed_pieces = 0;
}

void TTTGame::createNetworkMessage(TCP_DATA_MSG &msg, msg_type type) {

    if (update == type) {
        std::string gameState = currentState();

        char buffer[14] = {};
        buffer[0] = game_id;
        buffer[1] = update;
        gameState.copy(buffer+1, 11, 0);
        buffer[12] = '\0';

        msg.add_message(reinterpret_cast<uint8_t*>(&buffer[0]), 13);
    } else if (victory == type) {
        // probably don't need, each node will check and find the win
    } else if (cat == type) {
        // probably don't need, each node will check and find the cat
    } else if (restart == type) {
        uint8_t buffer[1] = {static_cast<uint8_t>(type)};
        msg.add_message(buffer, 1);
    }
}

void NetworkTTTGame::increment_position() {
     if (++user_position.x == 3) {
        user_position.x = 0;
        user_position.y++;
     }
     if (user_position.y == 3) {
        user_position.y = 0;
     }
}
pos_cords NetworkTTTGame::get_position() {
    return user_position;
}

void NetworkTTTGame::draw_peice(object o, int x, int y) {
    UBYTE* image = NULL;
    
    switch(o) {
        case X:
            image = (UBYTE*)(epd_bitmap_X);
            break;
        case O:
            image = (UBYTE*)(epd_bitmap_O);
            break;
        default:
            return;
            //image = (UBYTE*)(epd_bitmap_O);
            //break;
    }

    Paint_BmpWindows(piece_positions[y][x].x, piece_positions[y][x].y, image, 14, 14);
}

void NetworkTTTGame::draw_selector() {
    Paint_DrawRectangle(piece_positions[user_position.y][user_position.x].x, 
                        piece_positions[user_position.y][user_position.x].y, 
                        piece_positions[user_position.y][user_position.x].x+15, 
                        piece_positions[user_position.y][user_position.x].y+15, 
                        WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
}