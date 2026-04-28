#include "protocol.h"

int sock_fd;
char my_username[32];

void *receive_handler(void *arg) {
    payload_t p;
    while (recv(sock_fd, &p, sizeof(payload_t), 0) > 0) {
        if (p.type == MSG_ERROR) {
            printf("\n[System] Identity already synchronized in The Wired.\n");
            exit(0);
        } else if (p.type == MSG_CHAT) {
            printf("\n[%s]: %s\n> ", p.username, p.text);
            fflush(stdout);
        } else if (p.type == MSG_ADMIN) {
            printf("\n[Admin Response]: %s\n> ", p.text);
            fflush(stdout);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed. The Wired is offline.\n");
        return -1;
    }

    printf("Enter your name: ");
    fgets(my_username, 32, stdin);
    my_username[strcspn(my_username, "\n")] = 0;

    payload_t p;
    p.type = MSG_LOGIN;
    strcpy(p.username, my_username);
    send(sock_fd, &p, sizeof(payload_t), 0);

    // Initial Auth Check
    recv(sock_fd, &p, sizeof(payload_t), 0);
    if (p.type == MSG_ERROR) {
        printf("[System] The identity '%s' is already synchronized in The Wired.\n", my_username);
        return 0;
    }

    printf("--- Welcome to The Wired, %s ---\n", my_username);
    
    if (strcmp(my_username, "The Knights") == 0) {
        printf("Enter Password: ");
        char pwd[32];
        fgets(pwd, 32, stdin);
        pwd[strcspn(pwd, "\n")] = 0;
        if (strcmp(pwd, "protocol7") != 0) { printf("Access Denied.\n"); return 0; }
        printf("[System] Authentication Successful. Granted Admin privileges.\n");
    }

    pthread_t tid;
    pthread_create(&tid, NULL, receive_handler, NULL);

    while (1) {
        printf("> ");
        fgets(p.text, BUFFER_SIZE, stdin);
        p.text[strcspn(p.text, "\n")] = 0;

        if (strcmp(p.text, "/exit") == 0) {
            printf("[System] Disconnecting from The Wired...\n");
            break;
        }

        if (strcmp(my_username, "The Knights") == 0) {
            p.type = MSG_ADMIN;
            if (strcmp(p.text, "1") == 0) strcpy(p.text, "RPC_GET_USERS");
            else if (strcmp(p.text, "3") == 0) strcpy(p.text, "RPC_SHUTDOWN");
        } else {
            p.type = MSG_CHAT;
        }
        
        strcpy(p.username, my_username);
        send(sock_fd, &p, sizeof(payload_t), 0);
    }

    close(sock_fd);
    return 0;
}