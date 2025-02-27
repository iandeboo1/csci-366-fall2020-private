//
// Created by carson on 5/20/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "server.h"
#include "char_buff.h"

extern void nasm_hello_world();

struct char_buff * repl_read_command(char * prompt) {
    printf("%s", prompt);
    char *line = NULL;
    size_t buffer_size = 0; // let getline autosize it
    if (getline(&line, &buffer_size, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);  // We received an EOF
        } else  {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    if (strcmp(line, "") == 0) {
        free(line);
        return NULL;
    } else {
        struct char_buff *buffer = cb_create(2000);
        cb_append(buffer, line);
        free(line);
        return buffer;
    }
}

void repl_execute_command(struct char_buff * buffer) {
    char* command = cb_tokenize(buffer, " \n");
    if (command) {
        char* arg1 = cb_next_token(buffer);
        char* arg2 = cb_next_token(buffer);
        char* arg3 = cb_next_token(buffer);
        if (strcmp(command, "exit") == 0) {
            printf("\nGoodbye!\n");
            exit(EXIT_SUCCESS);
        } else if(strcmp(command, "?") == 0) {
            printf("\n? - show help\n");
            printf("load [0-1] <string> - load a ship layout file for the given player\n");
            printf("show [0-1] - shows the board for the given player\n");
            printf("fire [0-1] [0-7] [0-7] - fire at the given position\n");
            printf("say <string> - Send the string to all players as part of a chat\n");
            printf("reset - reset the game\n");
            printf("server - start the server\n");
            printf("exit - quit the server\n");
        } else if(strcmp(command, "server") == 0) {
            if (server_start() == 1) {
                printf("\nServer started successfully\n\n");
            } else {
                printf("\nServer could not be started\n");
            }
        } else if(strcmp(command, "show") == 0) {
            game * current_game = game_get_current();
            int player_as_int = arg1[0] - '0';
            struct char_buff *board_buffer = cb_create(2000);
            repl_print_board(current_game, player_as_int, board_buffer);
        } else if(strcmp(command, "reset") == 0) {
            game_init();
            printf("\nGame has been reset\n\n");
        } else if (strcmp(command, "load") == 0) {
            game * current_game = game_get_current();
            int player_as_int = arg1[0] - '0';
            game_load_board(current_game, player_as_int, arg2);
        } else if (strcmp(command, "fire") == 0) {
            game * current_game = game_get_current();
            int player_as_int = arg1[0] - '0';
            int x_pos = arg2[0] - '0';
            int y_pos = arg3[0] - '0';
            game_fire(current_game, player_as_int, x_pos, y_pos);
        } else if (strcmp(command, "nasm") == 0) {
            nasm_hello_world();
        } else if (strcmp(command, "say") == 0) {
            struct char_buff *chat_buffer = cb_create(2000);
            cb_append(chat_buffer, arg1);
            server_broadcast(chat_buffer, 2);
        } else if (strcmp(command, "shortcut") == 0) {
            game_get_current()->players[1].ships = 1ull;
            printf("\nSet Player 1's board to have only the 0,0 bit position\n\n");
        } else {
            printf("\nUnknown Command: %s\n", command);
        }
    }
}

void repl_print_board(game *game, int player, char_buff * buffer) {
    player_info player_info = game->players[player];
    cb_append(buffer, "\nbattleBit.........\n");
    cb_append(buffer, "-----[ ENEMY ]----\n");
    repl_print_hits(&player_info, buffer);
    cb_append(buffer, "==================\n");
    cb_append(buffer, "-----[ SHIPS ]----\n");
    repl_print_ships(&player_info, buffer);
    cb_append(buffer, ".........battleBit\n\n");
    cb_print(buffer);
}

void repl_print_ships(player_info *player_info, char_buff *buffer) {
    // Step 4 - Implement this to print out the visual ships representation
    //  for the console.  You will need to use bit masking for each position
    //  to determine if a ship is at the position or not.  If it is present
    //  you need to print an X.  If not, you need to print a space character ' '
    cb_append(buffer, "  0 1 2 3 4 5 6 7 \n");
    unsigned long long ships = player_info->ships;
    for (int i = 0; i < 8; i++) {
        //loop through rows of game board
        cb_append_int(buffer, i);
        cb_append(buffer, " ");
        for (int j = 0; j < 8; j++) {
            //loop through positions in row
            unsigned int position = (i * 8) + j;
            unsigned long long bitmask = 1ull << position;
            if ((ships & bitmask) != 0) {
                //ship at position
                cb_append(buffer, "* ");
            } else {
                cb_append(buffer, "  ");
            }
        }
        cb_append(buffer, "\n");
    }
}

void repl_print_hits(struct player_info *player_info, struct char_buff *buffer) {
    // Step 6 - Implement this to print out a visual representation of the shots
    // that the player has taken and if they are a hit or not.  You will again need
    // to use bit-masking, but this time you will need to consult two values: both
    // hits and shots values in the players game struct.  If a shot was fired at
    // a given spot and it was a hit, print 'H', if it was a miss, print 'M'.  If
    // no shot was taken at a position, print a space character ' '
    cb_append(buffer, "  0 1 2 3 4 5 6 7 \n");
    unsigned long long shots = player_info->shots;
    unsigned long long hits = player_info->hits;
    for (int i = 0; i < 8; i++) {
        //loop through rows of game board
        cb_append_int(buffer, i);
        cb_append(buffer, " ");
        for (int j = 0; j < 8; j++) {
            //loop through positions in row
            unsigned int position = (i * 8) + j;
            unsigned long long bitmask = 1ull << position;
            if ((shots & bitmask) != 0) {
                //shot a position
                if ((hits & bitmask) != 0) {
                    //hit at position
                    cb_append(buffer, "H ");
                } else {
                    //miss at position
                    cb_append(buffer, "M ");
                }
            } else {
                cb_append(buffer, "  ");
            }
        }
        cb_append(buffer, "\n");
    }
}
