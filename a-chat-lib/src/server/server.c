#include "server/server.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "log.h"

AChatServer* a_chat_server_create(const char* port) {
    // create the server on the heap to avoid thread race conditions
    AChatServer* server = malloc(sizeof(AChatServer));
    if (!server) {
        a_chat_log_error("Failed to allocate memory for server");
        return NULL;
    }

    // get all the ip address related infomation for us
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC; // allows for either IPv4 or IPv6, it doesnt matter to us
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_PASSIVE; // get the correct IP for us

    struct addrinfo* address_info;
    int status; // used for error checking
    if ((status = getaddrinfo(NULL, port, &hints, &address_info)) != 0) {
        a_chat_log_error_gai_strerror("Failed to get address infomation", status);

        free(server);
        return NULL;
    }

    // create the listening socket with the correct ip infomation
    server->listening_socket = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (server->listening_socket == -1) {
        a_chat_log_error_errno("Failed to create listening socket");

        free(server);
        return NULL;
    }

    // bind the socket to the port
    int bind_result = bind(server->listening_socket, address_info->ai_addr, address_info->ai_addrlen);
    if (bind_result == -1) {
        a_chat_log_error_errno("Failed to bind port");

        close(server->listening_socket);
        free(server);
        return NULL;
    }

    // free the address infomation as it is no longer needed
    freeaddrinfo(address_info);

    // begin listening
    int listen_result = listen(server->listening_socket, 10);
    if (listen_result == -1) {
        a_chat_log_error_errno("Failed to begin listening");

        close(server->listening_socket);
        free(server);
        return NULL;
    }

    // create the mutex for thread safety
    if (pthread_mutex_init(&server->lock, NULL) != 0) {
        a_chat_log_error("Failed to create thread mutex");

        close(server->listening_socket);
        free(server);
        return NULL;
    }

    server->running = true;

    // log that the server was created successfully and which port the server is using
    char message[128];
    snprintf(message, sizeof(message), "Created server at port: %s", port);
    a_chat_log_info(message);

    return server;
}

static void a_chat_client_handler_destroy(AChatClientHandlerThreadArguments* thread_arguments) {
    // close the client handler's socket
    if (close(thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].socket) != 0) {
        a_chat_log_error("Failed to close client handler socket while destroying client handler");
    }

    // lock the server for thread safety
    if (pthread_mutex_lock(&thread_arguments->server->lock) != 0) {
        a_chat_log_error("Failed to lock thread mutex while destroying client handler");
        return;
    }

    // shift all the client handlers down starting at the client handler being destroyed
    for (int i = thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].index; i < thread_arguments->server->number_of_clients - 1; i++) {
        thread_arguments->server->clientHandlers[i] = thread_arguments->server->clientHandlers[i + 1];
        thread_arguments->server->clientHandlers[i].index = i;
    }
    thread_arguments->server->number_of_clients--;

    // unlock as the server struct is no longer being modified
    if (pthread_mutex_unlock(&thread_arguments->server->lock) != 0) {
        a_chat_log_error("Failed to unlock thread mutex while destroying client handler");
        return;
    }

    // finally free the thread arguments pointer and set it to NULL
    free(thread_arguments);
    thread_arguments = NULL;
}

static void* a_chat_client_handler_thread(void* arguments) {
    // check if the arguments are valid
    if (!arguments) {
        a_chat_log_error("Failed to get client handler arguments!");
        return NULL;
    }

    // create a pointer of type "AChatClientHandlerThreadArguments" so it is easier to use the arguments passed through
    AChatClientHandlerThreadArguments* thread_arguments = (AChatClientHandlerThreadArguments*) arguments;

    while (true) {
        // check if the server is still running
        if (pthread_mutex_lock(&thread_arguments->server->lock) != 0) {
            a_chat_log_error("Failed to lock server's mutex in client handler main loop");
            return NULL;
        }
        bool running = thread_arguments->server->running;
        if (pthread_mutex_unlock(&thread_arguments->server->lock) != 0) {
            a_chat_log_error("Failed to unlock server's mutex in client handler main loop");
            return NULL;
        }
        if (!running) { break; }

        // wait till the client sends something, then receive the infomation and put it into the buffer
        char buffer[1024];
        int bytes_received = recv(thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].socket, &buffer, sizeof(buffer), 0);
        if (bytes_received == 0) { // if recv() returns 0, the client associated with the client handler has disconnected
            char message[640];
            snprintf(message, sizeof(message), "%s has disconnected", thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].username);
            a_chat_log_info(message);
            a_chat_server_broadcast(thread_arguments->server, message);

            break;
        } else if (bytes_received == -1) { // revc() return -1 if any errors occur and sets errno with the error message
            char message[640];
            snprintf(message, sizeof(message), "Connection with client %s has failed", thread_arguments->server->clientHandlers[thread_arguments->client_handler_index].username);
            a_chat_log_error_errno(message);

            break;
        }

        // this is just temporary
        buffer[bytes_received - 1] = '\0'; // - 1 because of new line
        a_chat_server_broadcast(thread_arguments->server, buffer);
        printf("%s\n", buffer);
    }

    // destory client handler onces the client disconnects or an error occurs
    a_chat_client_handler_destroy(arguments);

    return NULL;
}

static bool a_chat_handshake(AChatServer* server) {
    // set a timeout time of 5 seconds
    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(server->clientHandlers[server->number_of_clients].socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // the client will send a "handshake" message which looks like this "a-chat [username]"
    char buffer[1024];
    int bytes_received = recv(server->clientHandlers[server->number_of_clients].socket, &buffer, sizeof(buffer), 0);
    if (bytes_received == 0) {
        a_chat_log_error_errno("Client disconnect before handshake message was received");

        return false;
    } else if (bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            a_chat_log_error_errno("Handshake from client timed out");

            return false;
        }
        a_chat_log_error_errno("Failed to get handshake from client");

        return false;
    }

    // reset the socket's timeout
    timeout = (struct timeval) { .tv_sec = 0, .tv_usec = 0 };
    setsockopt(server->clientHandlers[server->number_of_clients].socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // make sure the handshake is at least long enough to contain "a-chat "
    if (strlen(buffer) < 7) {
        a_chat_log_error("Handshake with client was too short");

        return false;
    }

    // make sure the identifier matches with the expected result
    // the identifier is just the first "token" of the handshake, which is just "a-chat"
    if (strncmp(buffer, "a-chat", 6) != 0) {
        a_chat_log_error("Handshake with client had invalid identifier");

        return false;
    }

    // make sure the client's username is in the length
    const char* username_start = buffer + 7;
    int username_length = strlen(username_start);
    if (username_length == 0 || username_length >= 512) {
        a_chat_log_error("Client's username is invalid");

        return false;
    }

    // set the associated client handler's username to the client's username
    strncpy(server->clientHandlers[server->number_of_clients].username, username_start, 511);
    server->clientHandlers[server->number_of_clients].username[511] = '\0';

    return true;
}

static void a_chat_client_handler_create(AChatServer* server) {
    struct sockaddr_storage their_address;
    socklen_t address_size = sizeof(struct sockaddr_storage);

    // wait for a client connect and then accept the new connect
    int new_socket = accept(server->listening_socket, (struct sockaddr*) &their_address, &address_size);
    if (new_socket == -1) {
        a_chat_log_error_errno("Failed accept new client");

        return;
    }

    if (pthread_mutex_lock(&server->lock) != 0) {
        a_chat_log_error_errno("Failed to lock server's mutex while creating new client handler");

        close(new_socket);
        return;
    }

    // check if the maximum number of clients have connected
    if (server->number_of_clients >= MAXIMUM_CLIENTS) {
        a_chat_log_error("Maximum number of connected clients reached");

        close(new_socket);
        if (pthread_mutex_unlock(&server->lock) != 0) {
            a_chat_log_error_errno("Failed to unlock server's mutex while creating new client handler");
        }
        return;
    }

    // set the next client handler to the new socket and assign the correct index
    server->clientHandlers[server->number_of_clients].socket = new_socket;
    server->clientHandlers[server->number_of_clients].index = server->number_of_clients;

    // create the arguments for the new client handler's thread
    AChatClientHandlerThreadArguments* arguments = malloc(sizeof(AChatClientHandlerThreadArguments));
    if (!arguments) {
        a_chat_log_error("Failed to allocate memory for client handler thread arguments");

        close(server->clientHandlers[server->number_of_clients].socket);
        if (pthread_mutex_unlock(&server->lock) != 0) {
            a_chat_log_error_errno("Failed to unlock server's mutex while creating new client handler");
        }
        return;
    }

    arguments->client_handler_index = server->clientHandlers[server->number_of_clients].index;
    arguments->server = server;

    // get the handshake from the client
    if (!a_chat_handshake(server)) {
        // the correct error message will be printed inside the a_chat_handshake function

        close(server->clientHandlers[server->number_of_clients].socket);
        free(arguments);
        if (pthread_mutex_unlock(&server->lock) != 0) {
            a_chat_log_error_errno("Failed to unlock server's mutex while retrieving handshake from new client");
        }

        return;
    }

    // create the client handler thread with the arguments created
    if (pthread_create(&server->clientHandlers[server->number_of_clients].thread_id, NULL, a_chat_client_handler_thread, arguments) != 0) {
        a_chat_log_error_errno("Failed to create thread for new client handler");

        close(server->clientHandlers[server->number_of_clients].socket);
        free(arguments);
        if (pthread_mutex_unlock(&server->lock) != 0) {
            a_chat_log_error_errno("Failed to unlock server's mutex while creating thread for client handler");
        }
        return;
    }

    // used for logging and broadcasting that a new client has connected
    char message[640];
    snprintf(message, sizeof(message), "%s has connected", server->clientHandlers[server->number_of_clients].username);
    a_chat_log_info(message);

    server->number_of_clients++;

    if (pthread_mutex_unlock(&server->lock) != 0) {
        a_chat_log_error_errno("Failed to unlock server's mutex while creating new client handler");
        return;
    }

    // log and broadcast that a new client has connected to the server
    a_chat_server_broadcast(server, message); // this is at the end of the function because a_chat_server_broadcast uses the mutex
}

void a_chat_server_accept(AChatServer* server) {
    while (true) {
        // check if the server is still running
        if (pthread_mutex_lock(&server->lock) != 0) {
            a_chat_log_error("Failed to lock server's mutex while accepting new clients");
            break;
        }
        bool running = server->running;
        if (pthread_mutex_unlock(&server->lock) != 0) {
            a_chat_log_error("Failed to unlock server's mutex while accepting new clients");
            break;
        }

        if (!running) { break; }

        a_chat_client_handler_create(server);
    }
}

void a_chat_server_broadcast(AChatServer* server, const char* message) {
    if (pthread_mutex_lock(&server->lock) != 0) {
        a_chat_log_error("Failed to lock server's mutex while broadcasting message");
        return;
    }

    // send the message to every client
    for (int i = 0; i < server->number_of_clients; i++) {
        if (send(server->clientHandlers[i].socket, message, strlen(message), MSG_NOSIGNAL) == -1) {
            a_chat_log_warning_errno("Failed broadcast message to a client");
        }
    }

    if (pthread_mutex_unlock(&server->lock) != 0) {
        a_chat_log_error("Failed to unlock server's mutex while broadcasting message");
    }
}

void a_chat_server_close(AChatServer* server) {
    if (pthread_mutex_lock(&server->lock) != 0) {
        a_chat_log_error("Failed to lock server's mutex while closing server");
    }
    server->running = false;

    // join all the threads so the server and all it's threads close gracefully
    for (int i = 0; i < server->number_of_clients; i++) {
        pthread_join(server->clientHandlers[i].thread_id, NULL);
    }

    if (pthread_mutex_unlock(&server->lock) != 0) {
        a_chat_log_error("Failed to unlock server's mutex while closing server");
    }

    // shutdown the server's listening socket, the "SHUT_RDWR" is to stop allowing sending and receiving new messages
    shutdown(server->listening_socket, SHUT_RDWR);
    close(server->listening_socket);
    pthread_mutex_destroy(&server->lock);
    free(server);
}
