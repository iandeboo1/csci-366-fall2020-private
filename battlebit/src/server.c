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

void* handle_client_connect(int player) {
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
    sprintf(welcome, "\nWelcome to BattleBit\n\n");
    send(SERVER->player_sockets[player], welcome, strlen(welcome), 0);
    int playerConnected = 1;
    char_buff  * client_command;

    do {
        char message[100];
        sprintf(message, "battleBit (? for help) > ");
        send(SERVER->player_sockets[player], message, strlen(message), 0);
        char buffer[2000];
        if (recv(SERVER->player_sockets[player], buffer, 2000, 0) < 0) {
            puts("Receive failed");
        } else {
            puts(buffer);
        }
//        client_command = repl_read_command("?");
//TODO: FIGURE OUT WHY THIS LINE WON'T EXECUTE
        //for testing only
        char* command = "say";
        //for testing only
//        char* command = cb_tokenize(client_command, " \n");
        if (command) {
//            char *arg1 = cb_next_token(client_command);
//            char *arg2 = cb_next_token(client_command);
//            char *arg3 = cb_next_token(client_command);
            if (strcmp(command, "exit") == 0) {
                char exit_message[50];
                sprintf(exit_message, "\nGoodbye\n");
                send(SERVER->player_sockets[player], exit_message, strlen(exit_message), 0);
                playerConnected = 0;
                exit(EXIT_SUCCESS);
            } else if (strcmp(command, "?") == 0) {
                char help_message[300];
                sprintf(help_message, "\n? - show help\n"
                                      "load <string> - load a ship layout file for the given player\n"
                                      "show - shows the board for the given player\n"
                                      "fire [0-7] [0-7] - fire at the given position\n"
                                      "say <string> - Send the string to all players as part of a chat\n"
                                      "exit - quit the server\n");
                send(SERVER->player_sockets[player], help_message, strlen(help_message), 0);
            } else if (strcmp(command, "load") == 0) {
                char load_message[50];
                game * current_game = game_get_current();
                if (game_load_board(current_game, player, "C00b02D23S47p71")) {
                    sprintf(load_message, "\nLoaded game board successfully\n");
                    send(SERVER->player_sockets[player], load_message, strlen(load_message), 0);
                } else {
                    sprintf(load_message, "\nGame board was not loaded\n");
                    send(SERVER->player_sockets[player], load_message, strlen(load_message), 0);
                }
            } else if (strcmp(command, "show") == 0) {
                game * current_game = game_get_current();
                struct char_buff *board_buffer = cb_create(2000);
                repl_print_board(current_game, player, board_buffer);
                //TODO: NOT SURE HOW TO SEND THIS TO CLIENT IN A MESSAGE BECAUSE FUNC DOESN'T RETURN ANYTHING
            } else if (strcmp(command, "fire") == 0) {
                char fire_message[50];
                game * current_game = game_get_current();
                int x_pos = 1;
                int y_pos = 1;
                if (game_fire(current_game, player, x_pos, y_pos)) {
                    sprintf(fire_message, "\nFired shot successfully\n");
                    send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                } else {
                    sprintf(fire_message, "\nShot was not fired\n");
                    send(SERVER->player_sockets[player], fire_message, strlen(fire_message), 0);
                }
            } else if (strcmp(command, "say") == 0) {
                struct char_buff *chat_buffer = cb_create(2000);
                cb_append(chat_buffer, "Hello, everyone!");
                server_broadcast(chat_buffer);
            } else {
                char unknown_message[50];
                sprintf(unknown_message, "\nUnknown Command: %s\n", command);
                send(SERVER->player_sockets[player], unknown_message, strlen(unknown_message), 0);
            }
        }
//        cb_free(client_command);
    } while (playerConnected);

    return (void *) 1;
}

void server_broadcast(char_buff *msg) {
    puts((const char *) msg);
    send(SERVER->player_sockets[0], msg, strlen((const char *) msg), 0);
    send(SERVER->player_sockets[1], msg, strlen((const char *) msg), 0);
}

void* run_server() {
    // STEP 8 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Couldn't create socket");
        return (void *) 0;  //TODO: FIGURE OUT THESE RETURNS
    }
    int yes = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in server;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(9876);
    if (bind(server_socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        puts("Bind failed");
        return (void *) 0;
    } else {
        puts("Bind worked!");
        listen(server_socket_fd, 2);
        puts("Waiting for incoming connections...");
    }
    struct sockaddr_in client;
    socklen_t size_from_connect;
    int client_socket_fd;
    while ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client, &size_from_connect)) > 0) {
            if ((SERVER->player_sockets[0]) == 0) {
                SERVER->player_sockets[0] = client_socket_fd;
                pthread_create(&SERVER->player_threads[0], NULL, handle_client_connect(0), NULL);
            } else if ((SERVER->player_sockets[1]) == 0){
                SERVER->player_sockets[1] = client_socket_fd;
                pthread_create(&SERVER->player_threads[1], NULL, handle_client_connect(1), NULL);
            } else {
                puts("Game is full!");
            }
    }
    return (void *) 1;  //TODO: FIGURE OUT THESE RETURNS
}

int server_start() {
    // STEP 7 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL
    init_server();
    pthread_create(&SERVER->server_thread, NULL, run_server, NULL);
    return 1;   //TODO: FIGURE OUT THESE RETURNS
}
