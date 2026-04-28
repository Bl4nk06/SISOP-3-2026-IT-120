#include "protocol.h"

int client_sockets[MAX_CLIENTS];
char client_names[MAX_CLIENTS][32];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
time_t start_time;

void broadcast(payload_t *p, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1 && client_sockets[i] != sender_fd) {
            send(client_sockets[i], p, sizeof(payload_t), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int fd = *((int *)arg);
    free(arg);
    payload_t p;
    char name[32] = "";

    // Step 1: Authentication / Login
    if (recv(fd, &p, sizeof(payload_t), 0) <= 0) { close(fd); return NULL; }
    
    pthread_mutex_lock(&clients_mutex);
    int exists = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != -1 && strcmp(client_names[i], p.username) == 0) {
            exists = 1; break;
        }
    }

    if (exists && strcmp(p.username, "The Knights") != 0) {
        p.type = MSG_ERROR;
        send(fd, &p, sizeof(payload_t), 0);
        pthread_mutex_unlock(&clients_mutex);
        close(fd); return NULL;
    }

    // Assign to slot
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == -1) {
            client_sockets[i] = fd;
            strcpy(client_names[i], p.username);
            strcpy(name, p.username);
            slot = i; break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    p.type = MSG_SUCCESS;
    send(fd, &p, sizeof(payload_t), 0);
    
    char log_buf[256];
    sprintf(log_buf, "User '%s' connected", name);
    log_message("System", "Status", log_buf);

    // Step 2: Main Loop
    while (recv(fd, &p, sizeof(payload_t), 0) > 0) {
        if (p.type == MSG_ADMIN) {
            if (strcmp(p.text, "RPC_GET_USERS") == 0) {
                int count = 0;
                pthread_mutex_lock(&clients_mutex);
                for(int i=0; i<MAX_CLIENTS; i++) if(client_sockets[i] != -1 && strcmp(client_names[i], "The Knights") != 0) count++;
                pthread_mutex_unlock(&clients_mutex);
                sprintf(p.text, "Active Entities: %d", count);
                send(fd, &p, sizeof(payload_t), 0);
                log_message("Admin", "RPC_GET_USERS", "");
            } else if (strcmp(p.text, "RPC_SHUTDOWN") == 0) {
                log_message("Admin", "RPC_SHUTDOWN", "EMERGENCY SHUTDOWN INITIATED");
                exit(0);
            }
        } else {
            char chat_log[BUFFER_SIZE + 50];
            sprintf(chat_log, "[%s]: %s", name, p.text);
            log_message("User", chat_log, "");
            broadcast(&p, fd);
        }
    }

    // Cleanup on disconnect
    pthread_mutex_lock(&clients_mutex);
    client_sockets[slot] = -1;
    pthread_mutex_unlock(&clients_mutex);
    sprintf(log_buf, "User '%s' disconnected", name);
    log_message("System", "Status", log_buf);
    close(fd);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    start_time = time(NULL);

    for (int i = 0; i < MAX_CLIENTS; i++) client_sockets[i] = -1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);
    
    log_message("System", "SERVER ONLINE", "");
    printf("The Wired is active on port %d...\n", PORT);

    while (1) {
        int *new_sock = malloc(sizeof(int));
        *new_sock = accept(server_fd, NULL, NULL);
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_sock);
        pthread_detach(tid);
    }
    return 0;
}