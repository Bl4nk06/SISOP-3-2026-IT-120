#include "arena.h"

int current_idx = -1;
SharedData *shm_ptr;
struct termios oldt;

void set_raw_mode(int enable) {
    if (enable) {
        struct termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

void handle_sigint(int sig) {
    set_raw_mode(0);
    if(current_idx != -1) shm_ptr->users[current_idx].is_logged_in = 0;
    printf("\n" YEL "Keluar dari Eterion..." RESET "\n");
    exit(0);
}

void print_banner() {
    system("clear");
    printf(CYN BOLD);
    printf("====================================================\n");
    printf("  ____  _____ _____ ____  ___ ___  _   _ \n");
    printf(" | __ )| ____|_   _|  _ \\|_ _/ _ \\| \\ | |\n");
    printf(" |  _ \\|  _|   | | | |_) || | | | |  \\| |\n");
    printf(" | |_) | |___  | | |  _ < | | |_| | |\\  |\n");
    printf(" |____/|_____| |_| |_| \\_\\___\\___/|_| \\_|\n");
    printf("----------------------------------------------------\n");
    printf("             BATTLE OF ETERION ONLINE               \n");
    printf("====================================================\n");
    printf(RESET "\n");
}

void draw_hp_bar(int cur, int max, char *color) {
    int w = 30; int p = (cur * w) / max;
    if(p < 0) p = 0;
    printf("%s[", color);
    for(int i=0; i<w; i++) printf(i < p ? "=" : " ");
    printf("] %d/%d" RESET "\n", cur, max);
}

void *battle_ui(void *arg) {
    while(shm_ptr->users[current_idx].state == 2) {
        int opp = shm_ptr->users[current_idx].last_opponent_idx;
        system("clear");
        printf(YEL BOLD "==============================================\n");
        printf("              BATTLE FIELD                    \n");
        printf("==============================================\n\n" RESET);
        printf(BOLD "Lawan: " RED "%s\n" RESET, (opp == -2 ? "Wild Monster" : shm_ptr->users[opp].username));
        draw_hp_bar((opp == -2 ? 80 : shm_ptr->users[opp].health), 100, RED);
        printf("\n                     VS                     \n\n");
        printf(BOLD "Prajurit: " GREEN "%s\n" RESET, shm_ptr->users[current_idx].username);
        draw_hp_bar(shm_ptr->users[current_idx].health, 100, GREEN);
        printf("\n" BOLD "[a] Serang  |  [q] Kabur\n> " RESET);
        fflush(stdout);
        usleep(250000);
    }
    return NULL;
}

int main() {
    signal(SIGINT, handle_sigint);
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if(shmid < 0) { printf("Jalankan ./orion dulu!\n"); return 1; }
    shm_ptr = (SharedData *)shmat(shmid, NULL, 0);
    int msgid = msgget(MSG_KEY, 0666);

    while(1) {
        print_banner();
        printf("1. Register\n2. Login\n3. Exit\n> ");
        int choice; if(scanf("%d", &choice) != 1) break;

        if(choice == 1) {
            struct msg_buffer m; m.msg_type = 1;
            printf("Username: "); scanf("%s", m.user_data.username);
            printf("Password: "); scanf("%s", m.user_data.password);
            msgsnd(msgid, &m, sizeof(User), 0);
            msgrcv(msgid, &m, sizeof(User), 0, 0);
            printf(m.msg_type == 3 ? GREEN "Success!\n" RESET : RED "Fail!\n" RESET);
            sleep(1);
        } else if(choice == 2) {
            char u[50], p[50]; printf("User: "); scanf("%s", u); printf("Pass: "); scanf("%s", p);
            for(int i=0; i<MAX_USERS; i++) {
                if(strcmp(shm_ptr->users[i].username, u) == 0 && strcmp(shm_ptr->users[i].password, p) == 0) {
                    current_idx = i; shm_ptr->users[i].is_logged_in = 1;
                    while(1) {
                        print_banner();
                        printf(BOLD "Prajurit: %s | Gold: %d\n" RESET "1. Battle\n2. Logout\n> ", u, shm_ptr->users[i].money);
                        int m2; scanf("%d", &m2);
                        if(m2 == 1) {
                            shm_ptr->users[i].state = 1; shm_ptr->users[i].health = 100;
                            printf("Mencari lawan...\n");
                            int t = 0; while(shm_ptr->users[i].state == 1 && t < 70) { usleep(500000); t++; }
                            if(shm_ptr->users[i].state == 1) { shm_ptr->users[i].state = 2; shm_ptr->users[i].last_opponent_idx = -2; }
                            set_raw_mode(1);
                            pthread_t tid; pthread_create(&tid, NULL, battle_ui, NULL);
                            while(shm_ptr->users[i].state == 2) {
                                char in = getchar();
                                if(in == 'a') { int o = shm_ptr->users[i].last_opponent_idx; if(o >= 0) shm_ptr->users[o].health -= 10; }
                                else if(in == 'q') { shm_ptr->users[i].state = 0; break; }
                                if(shm_ptr->users[i].health <= 0 || (shm_ptr->users[i].last_opponent_idx >= 0 && shm_ptr->users[shm_ptr->users[i].last_opponent_idx].health <= 0)) { shm_ptr->users[i].state = 0; break; }
                            }
                            set_raw_mode(0);
                        } else { shm_ptr->users[i].is_logged_in = 0; break; }
                    }
                    break;
                }
            }
        } else break;
    }
    set_raw_mode(0);
    return 0;
}