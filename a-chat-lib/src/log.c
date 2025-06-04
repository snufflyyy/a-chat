#include "log.h"

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// todo: save log to file!

void a_chat_log_error(const char* message) {
    const char* prefix = "ERROR:";
    const char* suffix = "\n";

    int length = strlen(prefix) + 1 + strlen(message) + strlen(suffix) + 1; // first "+ 1" is for the space, second "+ 1" is for the null terminator
    char* output = malloc(length);
    if (!output) {
        perror("ERROR: Failed to allocate memory for error message!\n");
        return;
    }

    snprintf(output, length, "%s %s%s", prefix, message, suffix);
    fprintf(stderr, "%s", output);
    free(output);
}

void a_chat_log_error_gai_strerror(const char* message, int status) {
    const char* prefix = "ERROR:";
    const char* error_message = gai_strerror(status);
    const char* suffix = "\n";

    // first "+ 1" is for the space, the "+ 2" is for ":" and space, and the second "+ 1" is for the null terminator
    int length = strlen(prefix) + 1 + strlen(message) + 2 + strlen(error_message) + strlen(suffix) + 1;
    char* output = malloc(length);
    if (!output) {
        a_chat_log_error("Failed to allocate memory for error message!");
        return;
    }

    snprintf(output, length, "%s %s: %s%s", prefix, message, error_message, suffix);
    fprintf(stderr, "%s", output);
    free(output);
}

void a_chat_log_error_errno(const char* message) {
    const char* prefix = "ERROR:";

    char error_message[1024];
    int result = strerror_r(errno, error_message, sizeof(error_message));
    if (result != 0) {
        a_chat_log_error("Failed to get error from errno!");
        return;
    }

    const char* suffix = "\n";

    // first "+ 1" is for the space, the "+ 2" is for ":" and space, and the second "+ 1" is for the null terminator
    int length = strlen(prefix) + 1 + strlen(message) + 2 + strlen(error_message) + strlen(suffix) + 1;
    char* output = malloc(length);
    if (!output) {
        a_chat_log_error("Failed to allocate memory for error message!");
        return;
    }

    snprintf(output, length, "%s %s: %s%s", prefix, message, error_message, suffix);
    fprintf(stderr, "%s", output);
    free(output);
}

void a_chat_log_warning_errno(const char* message) {
    const char* prefix = "WARNING:";

    char error_message[1024];
    int result = strerror_r(errno, error_message, sizeof(error_message));
    if (result != 0) {
        a_chat_log_error("Failed to get error from errno!");
        return;
    }

    const char* suffix = "\n";

    // first "+ 1" is for the space, the "+ 2" is for ":" and space, and the second "+ 1" is for the null terminator
    int length = strlen(prefix) + 1 + strlen(message) + 2 + strlen(error_message) + strlen(suffix) + 1;
    char* output = malloc(length);
    if (!output) {
        a_chat_log_error("Failed to allocate memory for error message!");
        return;
    }

    snprintf(output, length, "%s %s: %s%s", prefix, message, error_message, suffix);
    fprintf(stderr, "%s", output);
    free(output);
}

void a_chat_log_info(const char* message) {
    const char* prefix = "INFO:";
    const char* suffix = "\n";

    int length = strlen(prefix) + 1 + strlen(message) + strlen(suffix) + 1; // first "+ 1" is for the space, second "+ 1" is for the null terminator
    char* output = malloc(length);
    if (!output) {
        a_chat_log_error("Failed to allocate memory for info message!");
        return;
    }

    snprintf(output, length, "%s %s%s", prefix, message, suffix);
    fprintf(stderr, "%s", output);
    free(output);
}
