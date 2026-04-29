// REPLACE TOTAL isi arena.h dengan ini:
#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>

#define SHM_KEY 0x9988 
#define MSG_KEY 0x8899 
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
    int xp;
    int level;
    int has_weapon[6]; 
    int is_logged_in;
    int state; 
    int last_opponent_idx; 
    time_t queue_start;
    char history_log[10][100]; 
} User;

typedef struct {
    User users[MAX_USERS];
    sem_t mutex;
} SharedData;

struct msg_buffer {
    long msg_type;
    User user_data;
};

// Data Armory (ID 1-5 sesuai gambar)
static const char *arm_names[] = {"None", "Wooden Sword", "Iron Sword", "Steel Axe", "Demon Blade", "God Slayer"};
static const int arm_price[] = {0, 100, 300, 600, 1500, 5000};
static const int arm_dmg[] = {0, 5, 15, 30, 60, 150};

#endif