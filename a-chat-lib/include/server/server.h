#pragma once

#include <stdbool.h>
#include <pthread.h>

#define MAXIMUM_CLIENTS 100

typedef struct AChatClientHandler {
    pthread_t thread_id;
    int index;
    int socket;
    char username[512];
} AChatClientHandler;

typedef struct AChatServer {
    bool running;

    int listening_socket;

    AChatClientHandler clientHandlers[MAXIMUM_CLIENTS];
    int number_of_clients;

    pthread_mutex_t lock;
} AChatServer;

typedef struct AChatClientHandlerThreadArguments {
    AChatServer* server;
    int client_handler_index;
} AChatClientHandlerThreadArguments;

AChatServer* a_chat_server_create(const char* port);
void a_chat_server_accept(AChatServer* server);
void a_chat_server_broadcast(AChatServer* server, const char* message);
void a_chat_server_close(AChatServer* server);
