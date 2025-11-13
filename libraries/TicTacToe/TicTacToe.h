#include "Messages.hpp"
#include <MeshNode.hpp>
#include <cstdint>
#include <pico/types.h>
#include <vector>

enum object {
    EMPTY,
    X,
    O
};

enum msg_type {
    update,
    victory,
    cat,
    restart
};


class TTTGame {
public:
    TTTGame(uint8_t game_id);

    bool placeObject(object o, int x, int y);
    /**
     * @brief Checks grid state for a win
     * 
     * @return uint8_t 0 for no win, 1 for player 1, 2 for player 2
     */
    uint8_t checkWin();

    /**
     * @brief Creates a string representation of the current state
     * 
     * @return std::string -> example state "X0O,X0O,OX0"
     */
    std::string currentState();

    void restartGame();

    void createNetworkMessage(TCP_DATA_MSG &msg, msg_type type);

private:
    object row1[3] = {EMPTY, EMPTY, EMPTY};
    object row2[3] = {EMPTY, EMPTY, EMPTY};
    object row3[3] = {EMPTY, EMPTY, EMPTY};

    object* grid[3] {row1, row2, row3};

    uint8_t placed_pieces = 0;
    uint8_t game_id;
};

