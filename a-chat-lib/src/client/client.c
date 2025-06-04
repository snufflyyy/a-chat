#include "client/client.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

static bool a_chat_client_send_handshake(AChatClient* client) {
    char handshake_message[1024] = "a-chat ";
    strcat(handshake_message, client->username);

    int bytes_sent = send(client->socket, handshake_message, strlen(handshake_message), 0);
    if (bytes_sent == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));
        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to send handshake to server: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        return false;
    }

    return true;
}

AChatClient* a_chat_client_create(const char* ip_address, const char* port, const char* username) {
    AChatClient* client = malloc(sizeof(AChatClient));
    if (!client) {
        fprintf(stderr, "ERROR: Failed to allocate memory for client!\n");
        return NULL;
    }

    // get all the ip address related infomation for us
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // allows for either IPv4 or IPv6, it doesnt matter to us
    hints.ai_socktype = SOCK_STREAM; // use TCP

    struct addrinfo* address_info;
    int status; // used for error checking
    if ((status = getaddrinfo(ip_address, port, &hints, &address_info)) != 0) {
        fprintf(stderr, "ERROR: Failed to get address infomation: %s\n", gai_strerror(status));

        free(client);
        return NULL;
    }

    // create the client socket with the correct ip infomation
    client->socket = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (client->socket == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));
        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to create client socket: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        close(client->socket);
        free(client);
        return NULL;
    }

    // connect to the server using the client socket
    if (connect(client->socket, address_info->ai_addr, address_info->ai_addrlen) == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));

        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to connect to server: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        close(client->socket);
        free(client);
        return NULL;
    }

    // make sure the username is the correct length
    int username_length = strlen(username);
    if (username_length < 0) {
        fprintf(stderr, "ERROR: Username is empty!");

        close(client->socket);
        free(client);
        return NULL;
    } else if (username_length > 512) {
        fprintf(stderr, "ERROR: Username is longer than 512 characters!");

        close(client->socket);
        free(client);
        return NULL;
    }

    client->username = username;

    if (!a_chat_client_send_handshake(client)) {
        close(client->socket);
        free(client);
        return NULL;
    }

    while (true) {
        char buffer[1024];
        recv(client->socket, &buffer, sizeof(buffer), 0);
        printf("%s\n", buffer);
    }

    return client;
}

void a_chat_client_send(AChatClient* client, const char* message) {
    // this needs encryption!

    if (send(client->socket, message, strlen(message), 0) == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));

        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to send message: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }
    }
}

void a_chat_client_close(AChatClient* client) {
    close(client->socket);
    free(client);
    client = NULL;
}
