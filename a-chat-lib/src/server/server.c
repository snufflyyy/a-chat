#include "server/server.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

Server* a_chat_server_create(const char* port) {
    // create the server on the heap to avoid thread race conditions
    Server* server = malloc(sizeof(Server));
    if (!server) {
        fprintf(stderr, "ERROR: Failed to allocate memory for server\n");
        return NULL;
    }

    // get all the ip address related infomation for us
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // allows for either IPv4 or IPv6, it doesnt matter to us
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_PASSIVE; // get the correct IP for us

    struct addrinfo* address_info;
    int status; // used for error checking
    if ((status = getaddrinfo(NULL, port, &hints, &address_info)) != 0) {
        fprintf(stderr, "ERROR: Failed to get address infomation: %s\n", gai_strerror(status));

        free(server);
        return NULL;
    }

    // create the listening socket with the correct ip infomation
    server->listening_socket = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (server->listening_socket == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));

        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to create listen socket: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        close(server->listening_socket);
        free(server);
        return NULL;
    }

    // bind the socket to the port
    int bind_result = bind(server->listening_socket, address_info->ai_addr, address_info->ai_addrlen);
    if (bind_result == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));

        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to bind port: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        close(server->listening_socket);
        free(server);
        return NULL;
    }

    // free the address infomation as it is no longer needed
    freeaddrinfo(address_info);
    address_info = NULL;

    // begin listening
    int listen_result = listen(server->listening_socket, 10);
    if (listen_result == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));

        if (result == 0) {
            fprintf(stderr, "ERROR: Failed to begin listening: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        close(server->listening_socket);
        free(server);
        return NULL;
    }

    // create the mutex for thread safety
    if (pthread_mutex_init(&server->lock, NULL) != 0) {
        fprintf(stderr, "ERROR: Failed to create thread mutex!\n");
        free(server);
        return NULL;
    }

    printf("INFO: Created server at port: %s\n", port);

    return server;
}

static void a_chat_client_handler_destroy(ClientHandlerThreadArguments* thread_arguments) {
    // close the client handler's socket
    close(thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].socket);

    // lock the server for thread safety
    pthread_mutex_lock(&thread_arguments->server->lock);

    // shift all the client handlers down starting at the client handler being destroyed
    for (int i = thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].index; i < thread_arguments->server->number_of_clients - 1; i++) {
        thread_arguments->server->clientHandlers[i] = thread_arguments->server->clientHandlers[i + 1];
        thread_arguments->server->clientHandlers[i].index = i;
    }
    thread_arguments->server->number_of_clients--;

    // unlock as the server struct is no longer being modified
    pthread_mutex_unlock(&thread_arguments->server->lock);

    // finally free the thread arguments pointer and set it to NULL
    free(thread_arguments);
    thread_arguments = NULL;
}

static void* a_chat_client_handler_thread(void* arguments) {
    ClientHandlerThreadArguments* thread_arguments = (ClientHandlerThreadArguments*) arguments;

    while (true) {
        char buffer[1024];
        int bytes_received = recv(thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].socket, &buffer, sizeof(buffer), 0);
        if (bytes_received == 0) {
            printf("INFO: Client Disconnected!\n");
            break;
        } else if (bytes_received == -1) {
            char error_buffer[1024];
            int result = strerror_r(errno, error_buffer, sizeof(error_buffer));

            if (result == 0) {
                fprintf(stderr, "ERROR: Connection with client failed: %s\n", error_buffer);
            } else {
                fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
            }
        }

        // this is just temporary
        buffer[bytes_received - 1] = '\0';
        printf("%s\n", buffer);
    }

    // destory client handler onces the client disconnects or an error occurs
    a_chat_client_handler_destroy(arguments);

    return NULL;
}

static void a_chat_client_handler_create(Server* server) {
    struct sockaddr_storage their_address;
    socklen_t address_size = sizeof(struct sockaddr_storage);

    int new_socket = accept(server->listening_socket, (struct sockaddr*) &their_address, &address_size);
    if (new_socket == -1) {
        char error_buffer[1024];
        int result = strerror_r(errno, error_buffer, sizeof(error_buffer));
        if (result == 0) {
            fprintf(stderr, "ERROR: Failed accept new client: %s\n", error_buffer);
        } else {
            fprintf(stderr, "ERROR: Failed to get error with code: %d\n", result);
        }

        return;
    }

    pthread_mutex_lock(&server->lock);

    if (server->number_of_clients >= MAXIMUM_CLIENTS) {
        fprintf(stderr, "ERROR: Maximum clients reached: %d\n", MAXIMUM_CLIENTS);
        close(new_socket);
        pthread_mutex_unlock(&server->lock);
        return;
    }

    server->clientHandlers[server->number_of_clients].socket = new_socket;
    server->clientHandlers[server->number_of_clients].index = server->number_of_clients;

    ClientHandlerThreadArguments* arguments = malloc(sizeof(ClientHandlerThreadArguments));
    if (!arguments) {
        fprintf(stderr, "ERROR: Failed to allocate memory for client handler thread arguments!\n");
        pthread_mutex_unlock(&server->lock);
        return;
    }

    arguments->client_handler_index = server->clientHandlers[server->number_of_clients].index;
    arguments->server = server;

    // do some sort of handshake to get public key from client

    pthread_t client_handler_thread;
    pthread_create(&client_handler_thread, NULL, a_chat_client_handler_thread, arguments);
    pthread_detach(client_handler_thread);

    server->number_of_clients++;

    pthread_mutex_unlock(&server->lock);

    printf("INFO: Connected to client!\n");
}

void a_chat_server_accept(Server* server) {
    while (true) {
        a_chat_client_handler_create(server);
    }
}

void a_chat_server_close(Server* server) {
    close(server->listening_socket);
    pthread_mutex_destroy(&server->lock);
    free(server);
}
