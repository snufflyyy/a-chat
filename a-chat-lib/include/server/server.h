#pragma once

#include <pthread.h>

#define MAXIMUM_CLIENTS 100

typedef struct ClientHandler {
    int index;
    int socket;
} ClientHandler;

typedef struct Server {
    int listening_socket;

    ClientHandler clientHandlers[MAXIMUM_CLIENTS];
    int number_of_clients;

    pthread_mutex_t lock;
} Server;

typedef struct ClientHandlerThreadArguments {
    Server* server;
    int client_handler_index;
} ClientHandlerThreadArguments;

Server* a_chat_server_create(const char* port);
void a_chat_server_accept(Server* server);
void a_chat_server_close(Server* server);
