#include "TicTacToe.h"
#include <cstdint>
#include <pico/types.h>

TTTGame::TTTGame(uint8_t game_id) {
    this->game_id = game_id;
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
            gameState += std::to_string(grid[i][j]);
        }
        if (i != 2) gameState += ",";
    }

    return gameState;
}

void TTTGame::restartGame() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            grid[i][j] = EMPTY;
        }
    }
    placed_pieces = 0;
}

void TTTGame::createNetworkMessage(TCP_DATA_MSG &msg, msg_type type) {

    if (update == type) {
        std::string gameState = currentState();

        char buffer[13] = {};
        buffer[0] = update;
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