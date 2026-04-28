#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 50

typedef enum {
    MSG_LOGIN,
    MSG_CHAT,
    MSG_ERROR,
    MSG_SUCCESS,
    MSG_ADMIN,
    MSG_SHUTDOWN
} msg_type_t;

typedef struct {
    msg_type_t type;
    char username[32];
    char text[BUFFER_SIZE];
} payload_t;

void get_timestamp(char *buffer);
void log_message(const char *role, const char *status, const char *msg);

#endif