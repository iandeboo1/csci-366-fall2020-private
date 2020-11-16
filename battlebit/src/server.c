//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;

void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));
    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 9 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is their turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.
    char welcome[100];
    sprintf(welcome, "\nWelcome to BattleBit server Player %d\n\n", player);
    send(SERVER->player_sockets[player], welcome, strlen(welcome), 0);
    int playerConnected = 1;
    char_buff * client_command;

    do {
        client_command = cb_create(2000);
        char message[100];
        sprintf(message, "battleBit (? for help) > ");
        send(SERVER->player_sockets[player], message, strlen(message), 0);
        char buffer[2000];
        if (recv(SERVER->player_sockets[player], buffer, 2000, 0) < 0) {
            puts("Receive failed");
        } else {
            cb_append(client_command, buffer);
        }
        char * command = cb_tokenize(client_command, " \n\r");
        if (command) {
            char *arg1 = cb_next_token(client_command);
            char *arg2 = cb_next_token(client_command);
            if (strcmp(command, "exit") == 0) {
                char exit_message[50];
                sprintf(exit_message, "\nGoodbye!\n");
                send(SERVER->player_sockets[player], exit_message, strlen(exit_message), 0);
                playerConnected = 0;
            } else if (strcmp(command, "?") == 0) {
                char help_message[300];
                sprintf(help_message, "\n? - show help\n"
                                      "load <string> - load a ship layout\n"
                                      "show - shows the board\n"
                                      "fire [0-7] [0-7] - fire at the given position\n"
                                      "say <string> - Send the string to all players as part of a chat\n"
                                      "exit\n\n");
                send(SERVER->player_sockets[player], help_message, strlen(help_message), 0);
            } else if (strcmp(command, "load") == 0) {
                char load_message[100];
                game * current_game = game_get_current();
                if (game_load_board(current_game, player, arg1)) {
                    sprintf(load_message, "\nLoaded game board successfully\n");
                    send(SERVER->player_sockets[player], load_message, strlen(load_message), 0);
                    if (current_game->status == CREATED || current_game->status == INITIALIZED) {
                        sprintf(load_message, "\nWaiting on Player %d\n\n", 1 - player);
                        send(SERVER->player_sockets[player], load_message, strlen(load_message), 0);
                    } else {
                        sprintf(load_message, "\nAll player boards loaded\n\nPlayer 0 turn\n\n");
                        send(SERVER->player_sockets[player], load_message, strlen(load_message), 0);
                    }
                } else {
                    sprintf(load_message, "\nGame board was not loaded\n");
                    send(SERVER->player_sockets[player], load_message, strlen(load_message), 0);
                }
            } else if (strcmp(command, "show") == 0) {
                player_info player_info = game_get_current()->players[player];
                struct char_buff *board_buffer = cb_create(2000);
                char show_message[2000];
                cb_append(board_buffer, "\nbattleBit.........\n");
                cb_append(board_buffer, "-----[ ENEMY ]----\n");
                repl_print_hits(&player_info, board_buffer);
                cb_append(board_buffer, "==================\n");
                cb_append(board_buffer, "-----[ SHIPS ]----\n");
                repl_print_ships(&player_info, board_buffer);
                cb_append(board_buffer, ".........battleBit\n\n");
                sprintf(show_message, "%s", board_buffer->buffer);
                send(SERVER->player_sockets[player], show_message, strlen(show_message), 0);
            } else if (strcmp(command, "fire") == 0) {
                char fire_message[200];
                char opponent_message[200];
                game * current_game = game_get_current();
                if ((current_game->status == PLAYER_0_TURN && player == 0) || (current_game->status == PLAYER_1_TURN && player == 1)) {
                    //it's their turn
                    int x_pos = arg1[0] - '0';
                    int y_pos = arg2[0] - '0';
                    int state = game_fire(current_game, player, x_pos, y_pos);
                    if (state == 1) {
                        //it was a hit
                        sprintf(fire_message, "\nPlayer %d fires at %d %d -- HIT\n\n", player, x_pos, y_pos);
                        send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                        sprintf(opponent_message, "\n\nPlayer %d fires at %d %d -- HIT\n\nbattleBit (? for help) > ", player, x_pos, y_pos);
                        send(SERVER->player_sockets[1 - player], opponent_message, strlen(opponent_message), 0);
                        puts(opponent_message);
                    } else if (state == 2) {
                        //game was won
                        sprintf(fire_message, "\nPlayer %d fires at %d %d -- HIT -- PLAYER %d WINS!\n\n", player, x_pos, y_pos, player);
                        send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                        sprintf(opponent_message, "\n\nPlayer %d fires at %d %d -- HIT -- PLAYER %d WINS!\n\nbattleBit (? for help) > ", player, x_pos, y_pos, player);
                        send(SERVER->player_sockets[1 - player], opponent_message, strlen(opponent_message), 0);
                        puts(opponent_message);
                    } else {
                        //it was a miss
                        sprintf(fire_message, "\nPlayer %d fires at %d %d -- MISS\n\n", player, x_pos, y_pos);
                        send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                        sprintf(opponent_message, "\n\nPlayer %d fires at %d %d -- MISS\n\nbattleBit (? for help) > ", player, x_pos, y_pos);
                        send(SERVER->player_sockets[1 - player], opponent_message, strlen(opponent_message), 0);
                        puts(opponent_message);
                    }
                } else if (current_game->status == CREATED) {
                    sprintf(fire_message, "\nGame has not begun!\n\n");
                    send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                } else if (current_game->status == INITIALIZED) {
                    sprintf(fire_message, "\nWaiting on opponent to choose their ship layout\n\n");
                    send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                } else {
                    sprintf(fire_message, "\nPlayer %d turn\n\n", 1 - player);
                    send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                }
            } else if (strcmp(command, "say") == 0) {
                char_buff * broadcast_msg = cb_create(2000);
                cb_append(broadcast_msg, arg1);
                cb_append(broadcast_msg, " ");
                cb_append(broadcast_msg, arg2);
                cb_append(broadcast_msg, " ");
                while (true) {
                    char * word = cb_next_token(client_command);
                    if (word == NULL) {
                        break;
                    } else {
                        cb_append(broadcast_msg, word);
                        cb_append(broadcast_msg, " ");
                    }
                }
                server_broadcast(broadcast_msg, player);
            } else {
                char unknown_message[50];
                sprintf(unknown_message, "\nUnknown Command: %s\n", command);
                send(SERVER->player_sockets[player], unknown_message, strlen(unknown_message), 0);
            }
        }
        cb_free(client_command);
    } while (playerConnected);

    return 1;
}

void server_broadcast(char_buff *msg, int playerID) {
    char broadcastA[2000];
    char broadcast1[2000];
    char broadcast2[2000];
    if (playerID == 0) {
        sprintf(broadcastA, "\nPlayer %d says: %s\n\nbattleBit (? for help) > ", playerID, msg->buffer);
        sprintf(broadcast1, "\nPlayer %d says: %s\n\n", playerID, msg->buffer);
        sprintf(broadcast2, "\n\nPlayer %d says: %s\n\nbattleBit (? for help) > ", playerID, msg->buffer);
    } else if (playerID == 1) {
        sprintf(broadcastA, "\nPlayer %d says: %s\n\nbattleBit (? for help) > ", playerID, msg->buffer);
        sprintf(broadcast1, "\n\nPlayer %d says: %s\n\nbattleBit (? for help) > ", playerID, msg->buffer);
        sprintf(broadcast2, "\nPlayer %d says: %s\n\n", playerID, msg->buffer);
    } else {
        sprintf(broadcastA, "\nAdmin says: %s\n", msg->buffer);
        sprintf(broadcast1, "\n\nAdmin says: %s\n\nbattleBit (? for help) > ", msg->buffer);
        sprintf(broadcast2, "\n\nAdmin says: %s\n\nbattleBit (? for help) > ", msg->buffer);
    }
    puts(broadcastA);
    send(SERVER->player_sockets[0], broadcast1, strlen(broadcast1), 0);
    send(SERVER->player_sockets[1], broadcast2, strlen(broadcast2), 0);
}

int run_server() {
    // STEP 8 - implement the server code to put this on the network.
    // Here you will need to initialize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Couldn't create socket");
    }
    int yes = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in server;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(9876);
    if (bind(server_socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        puts("\nBind failed\n");
        return 0;
    } else {
        puts("\nBind worked!\n");
        listen(server_socket_fd, 2);
        puts("Waiting for incoming connections...");
    }
    struct sockaddr_in client;
    socklen_t size_from_connect;
    int client_socket_fd;
    while ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client, &size_from_connect)) > 0) {
        if ((SERVER->player_sockets[0]) == 0) {
            SERVER->player_sockets[0] = client_socket_fd;
            pthread_create(&SERVER->player_threads[0], NULL, (void *(*)(void *)) &handle_client_connect, 0);
            if (game_get_current()->players[1].ships != 0) {
                game_get_current()->status = INITIALIZED;
            }
            puts("\n\nPlayer 0 added to the game\n\nbattleBit (? for help) > ");
        } else if ((SERVER->player_sockets[1]) == 0){
            SERVER->player_sockets[1] = client_socket_fd;
            pthread_create(&SERVER->player_threads[1], NULL, (void *(*)(void *)) &handle_client_connect, (void *) 1);
            if (game_get_current()->players[0].ships != 0) {
                game_get_current()->status = INITIALIZED;
            }
            puts("\n\nPlayer 1 added to the game\n\nbattleBit (? for help) > ");
        } else {
            puts("Game is full!");
        }
    }
    return 1;
}

int server_start() {
    // STEP 7 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL
    init_server();
    pthread_create(&SERVER->server_thread, NULL, (void *(*)(void *)) run_server, NULL);
    sleep(1);   //just so the REPL loop prompt is displayed after the server starting messages
    return 1;
}
