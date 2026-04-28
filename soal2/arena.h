#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <pthread.h>
#include <termios.h>
#include <time.h>
#include <signal.h>

#define SHM_KEY 0x1234
#define MSG_KEY 0x5678
#define MAX_USERS 10

#define RESET "\033[0m"
#define RED   "\033[1;31m"
#define GREEN "\033[1;32m"
#define YEL   "\033[1;33m"
#define CYN   "\033[1;36m"
#define BOLD  "\033[1m"

typedef struct {
    char username[50];
    char password[50];
    int health;
    int money;
    int is_logged_in;
    int state; // 0: Idle, 1: Searching, 2: Battle
    int last_opponent_idx; 
} User;

typedef struct {
    User users[MAX_USERS];
} SharedData;

struct msg_buffer {
    long msg_type;
    User user_data;
};

#endif