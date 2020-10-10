//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <string.h>
#include "game.h"

// STEP 10 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.
//TODO: THIS
static game * GAME = NULL;

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need to update the players 'shots' value
    //  - You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.
    if (x < 0 || x > 7 || y < 0 || y > 7) {
        return 0;
    }
    int other_player = 1 - player;
    unsigned long long this_shot = xy_to_bitval(x, y);
    unsigned long long shots = game->players[player].shots;
    unsigned long long opponent_ships = game->players[other_player].ships;
    unsigned long long hits = game->players[player].hits;
    if ((shots & this_shot) == 0) {
        //shot hasn't already been made
        game->players[player].shots = shots | this_shot;    //record shot in shots
        if ((opponent_ships & this_shot) != 0) {
            //hit opponent ship
            game->players[player].hits = hits | this_shot;  //record shot in hits
            game->players[other_player].ships = opponent_ships ^ this_shot;     //record shot in opponent's ships
            if (game->players[other_player].ships == 0) {
                //opponent has no ships left, current player won
                if (player == 0) {
                    game->status = PLAYER_1_WINS;
                } else {
                    game->status = PLAYER_2_WINS;
                }
            }
            return 1;
        } else {
            //missed opponent ship
            return 0;
        }
    } else {
        return 0;
    }
}

unsigned long long int xy_to_bitval(int x, int y) {
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b1 (the one is in the first position)
    // x:1, y: 0 == 0b10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.
    if (x >= 0 && x <= 7 && y >= 0 && y <= 7) {
        //contains valid coordinates
        unsigned long long j = 1;
        int bit_shift = x + (8 * y);
        for (int i = 0; i < bit_shift; i++) {
            j = j << 1u;
        }
        return j;
    } else {
        return 0;
    }
}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spec) {
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //
    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1
    char * valid_inputs = "cCbBdDsSpP01234567";
    bool is_valid = true;
    if (spec != NULL && strlen(spec) == 15) {
        //spec isn't blank and is the correct length
        for (int i = 0; i < strlen(spec); i++) {
            if ((strchr(valid_inputs, spec[i])) == NULL) {
                is_valid = false;
                break;
            }
        }
        if (is_valid) {
            //spec contains all valid characters
            bool hasCarrier = false;
            bool hasBattle = false;
            bool hasDestroyer = false;
            bool hasSub = false;
            bool hasPatrol = false;
            bool no_space_errors = true;
            for (int i = 0; i < 5; i++) {
                //one loop per ship specification
                char ship_type = spec[(3 * i)];
                int x_coord = spec[(3 * i) + 1] - '0';
                int y_coord= spec[(3 * i) + 2] - '0';
                int ship_length;
                if (ship_type == 'c' || ship_type == 'C') {
                    ship_length = 5;
                    if (hasCarrier) {
                        return -1;
                    } else {
                        hasCarrier = true;
                    }
                } else if (ship_type == 'b' || ship_type == 'B') {
                    ship_length = 4;
                    if (hasBattle) {
                        return -1;
                    } else {
                        hasBattle = true;
                    }
                } else if (ship_type == 'd' || ship_type == 'D') {
                    ship_length = 3;
                    if (hasDestroyer) {
                        return -1;
                    } else {
                        hasDestroyer = true;
                    }
                } else if (ship_type == 's' || ship_type == 'S') {
                    ship_length = 3;
                    if (hasSub) {
                        return -1;
                    } else {
                        hasSub = true;
                    }
                } else {
                    ship_length = 2;
                    if (hasPatrol) {
                        return -1;
                    } else {
                        hasPatrol = true;
                    }
                }
                if (ship_type >= 'A' && ship_type <= 'Z') {
                    //letter is uppercase
                    if ((add_ship_horizontal(&game->players[player], x_coord, y_coord, ship_length)) != 1) {
                        no_space_errors = false;
                        break;
                    }
                } else {
                    //letter is lowercase
                    if ((add_ship_vertical(&game->players[player], x_coord, y_coord, ship_length)) != 1) {
                        no_space_errors = false;
                        break;
                    }
                }
            }
            if (no_space_errors) {
                return 1;
            } else {
                return -1;  //at least one of the ship locations was not valid
            }
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

int add_ship_horizontal(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    unsigned long long ships = player->ships;
    if (length != 0) {
        if (x < 8) {
            //position is not off-the-board
            unsigned long long bitval = xy_to_bitval(x, y);
            if ((ships & bitval) == 0) {
                //spot open on board
                player->ships = ships | bitval;
                return add_ship_horizontal(player, x + 1, y, length - 1);
            } else {
                //overlapping ship
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        //ship position has been fully checked and is valid
        return 1;
    }
}

int add_ship_vertical(player_info *player, int x, int y, int length) {
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively
    unsigned long long ships = player->ships;
    if (length != 0) {
        if (y < 8) {
            //position is not off-the-board
            unsigned long long bitval = xy_to_bitval(x, y);
            if ((ships & bitval) == 0) {
                //spot open on board
                player->ships = ships | bitval;
                return add_ship_vertical(player, x, y + 1, length - 1);
            } else {
                //overlapping ship
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        //ship position has been fully checked and is valid
        return 1;
    }
}