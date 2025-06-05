#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <a-chat.h>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    switch (argc) {
        case 2: // use the localhost as the ip and the default port (1126)
            if (strcmp(argv[1], "server") == 0) {
                AChatServer* server = a_chat_server_create(A_CHAT_DEFAULT_PORT);
                if (!server) {
                    fprintf(stderr, "ERROR: Failed to create server!\n");
                    return -1;
                }

                a_chat_server_accept(server);

                a_chat_server_close(server);
            } else if (strcmp(argv[1], "client") == 0) {
                AChatClient* client = a_chat_client_create("127.0.0.1", A_CHAT_DEFAULT_PORT, "braden");
                if (!client) {
                    fprintf(stderr, "ERROR: Failed to create client!\n");
                    return -1;
                }

                while (client->running) {
                    char message[1024];
                    fgets(message, sizeof(message), stdin);

                    if (strcmp(message, "exit\n") == 0) {
                        client->running = false;
                        break;
                    }

                    send(client->socket, message, strlen(message), 0);
                }

                a_chat_client_close(client);
            }
            break;
        case 3: // use the ip address passed through and use the default port (1126)
            if (strcmp(argv[1], "server") == 0) {
                AChatServer* server = a_chat_server_create(A_CHAT_DEFAULT_PORT);
                if (!server) {
                    fprintf(stderr, "ERROR: Failed to create server!\n");
                    return -1;
                }

                a_chat_server_accept(server);
                a_chat_server_close(server);
            } else if (strcmp(argv[1], "client") == 0) {

            }
            break;
        case 4: // use the ip address and port passed through
            if (strcmp(argv[1], "server") == 0) {
                AChatServer* server = a_chat_server_create(argv[3]);
                if (!server) {
                    fprintf(stderr, "ERROR: Failed to create server!\n");
                    return -1;
                }

                a_chat_server_accept(server);
                a_chat_server_close(server);
            } else if (strcmp(argv[1], "client") == 0) {

            }
            break;
        default:
            printf("a-chat usage: [type] [ip-address] [port]\n");
            printf("\n");
            printf("types:\n");
            printf("\n");
            printf("  server - host a a-chat session\n");
            printf("  client - join a a-chat session\n");
            break;
    }

    return 0;
}
