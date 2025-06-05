#pragma once

#include <stdbool.h>
#include <pthread.h>

typedef struct AChatClient {
    bool running;

    int socket;
    const char* username;

    pthread_t receive_thread_id;
} AChatClient;

AChatClient* a_chat_client_create(const char* ip_address, const char* port, const char* username);
void a_chat_client_send(AChatClient* client, const char* message);
void a_chat_client_close(AChatClient* client);
