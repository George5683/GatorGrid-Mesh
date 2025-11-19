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

typedef struct pos_cords_t { 
    uint8_t x;
    uint8_t y; 
}pos_cords;

static constexpr pos_cords piece_positions [3][3] = {
    {{43, 49}, {43, 26}, {43, 3}},
    {{65, 49}, {65, 26}, {65, 3}},
    {{87, 49}, {87, 26}, {87, 3}}};

enum msg_type {
    update,
    victory,
    cat,
    restart
};

class TTTGame {
public:
    TTTGame();

    void setID(uint8_t game_id) { this->game_id = game_id; }

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
    void updateFromString(std::string state);

    void restartGame();

    void createNetworkMessage(TCP_DATA_MSG &msg, msg_type type);

    object row1[3] = {EMPTY, EMPTY, EMPTY};
    object row2[3] = {EMPTY, EMPTY, EMPTY};
    object row3[3] = {EMPTY, EMPTY, EMPTY};

    object* grid[3] {row1, row2, row3};

    uint8_t placed_pieces = 0;
    bool is_my_turn = false;

private:
    
    
    uint8_t game_id;
};

class NetworkTTTGame {
public:

    TTTGame game;
    
    pos_cords user_position = {.x = 0, .y = 0};

    NetworkTTTGame(uint8_t id) {
        game.setID(id);
    }

    void increment_position();
    pos_cords get_position();
    void draw_peice(object o, int x, int y);
    void draw_selector();
};
