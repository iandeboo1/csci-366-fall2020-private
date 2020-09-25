#include <stdio.h>
#include <stdlib.h>
#include "repl.h"
#include "game.h"

int main() {
    printf("Welcome to BattleBit\n\n");
    /**
     * Step 0 - Debug this main function with a break point on the line after
     * the repl_read_command, and step through some input of the various
     * commands you are expected to implement.  Notice how we are reading
     * into a buffer and then freeing the buffer.
     */

    game_init();
    struct game * gameon = game_get_current();
    struct player_info *player_info = &gameon->players[0];
    //game_init_player_info(player_info);
    char * spec = "C60P02D23S47p71";//incomplete spec
    game_load_board(gameon,0,spec);

//    char_buff * command;
//
//    game_init(); // NB: game init initializes the game state, all held in game.c
//
//    do {
//        // This is the classic Read, Evaluate, Print Loop, hence REPL
//        command = repl_read_command("battleBit (? for help) > ");
//        repl_execute_command(command);
//        cb_free(command);
//    } while (command);
    return 0;

}
