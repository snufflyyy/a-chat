#pragma once

typedef struct AChatClient {
    int socket;
    const char* username;
} AChatClient;

AChatClient* a_chat_client_create(const char* ip_address, const char* port, const char* username);
void a_chat_client_send(AChatClient* client, const char* message);
void a_chat_client_close(AChatClient* client);
