#include "client/client.h"

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "log.h"

static bool a_chat_client_send_handshake(AChatClient* client) {
    char handshake_message[1024] = "a-chat ";
    strcat(handshake_message, client->username);

    int bytes_sent = send(client->socket, handshake_message, strlen(handshake_message), 0);
    if (bytes_sent == -1) {
        a_chat_log_error_errno("Failed to send handshake to server");

        return false;
    }

    return true;
}

static void* a_chat_client_receive_thread(void* arguments) {
    AChatClient* client = (AChatClient*) arguments;

    // this is just temporary
    while (client->running) {
        char buffer[1024];
        int bytes_received = recv(client->socket, &buffer, sizeof(buffer), 0);
        if (bytes_received == 0 || bytes_received == -1) {
            client->running = false;
        }

        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }

    return NULL;
}

AChatClient* a_chat_client_create(const char* ip_address, const char* port, const char* username) {
    AChatClient* client = malloc(sizeof(AChatClient));
    if (!client) {
        a_chat_log_error("Failed to allocate memory for client");
        return NULL;
    }

    // get all the ip address related infomation for us
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC; // allows for either IPv4 or IPv6, it doesnt matter to us
    hints.ai_socktype = SOCK_STREAM; // use TCP

    struct addrinfo* address_info;
    int status; // used for error checking
    if ((status = getaddrinfo(ip_address, port, &hints, &address_info)) != 0) {
        a_chat_log_error_gai_strerror("Failed to get address infomation", status);

        free(client);
        return NULL;
    }

    // create the client socket with the correct ip infomation
    client->socket = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (client->socket == -1) {
        a_chat_log_error_errno("Failed to create client socket");

        free(client);
        return NULL;
    }

    // connect to the server using the client socket
    if (connect(client->socket, address_info->ai_addr, address_info->ai_addrlen) == -1) {
        a_chat_log_error_errno("Failed to connect to server");

        close(client->socket);
        free(client);
        return NULL;
    }

    // free the address infomation as it is no longer needed
    freeaddrinfo(address_info);

    // make sure the username is the correct length
    int username_length = strlen(username);
    if (username_length < 0 || username_length > 512) {
        a_chat_log_error("Username is invalid!");

        close(client->socket);
        free(client);
        return NULL;
    }

    client->username = username;

    if (!a_chat_client_send_handshake(client)) {
        // a_chat_client_send_handshake logs the correct error already

        close(client->socket);
        free(client);
        return NULL;
    }

    client->running = true;

    // create the receiving thread
    pthread_create(&client->receive_thread_id, NULL, a_chat_client_receive_thread, client);

    return client;
}

void a_chat_client_send(AChatClient* client, const char* message) {
    // this needs encryption!

    if (send(client->socket, message, strlen(message), 0) == -1) {
        a_chat_log_error_errno("Failed to send message to server");

        close(client->socket);
        free(client);
    }
}

void a_chat_client_close(AChatClient* client) {
    shutdown(client->socket, SHUT_RDWR);

    client->running = false;

    pthread_join(client->receive_thread_id, NULL);

    close(client->socket);
    free(client);
}
