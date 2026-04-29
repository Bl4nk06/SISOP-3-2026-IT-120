#include "arena.h"

// --- GLOBAL VARIABLES ---
int current_idx = -1;
SharedData *shm_ptr;
struct termios oldt;
int is_raw = 0;

// --- FUNCTION PROTOTYPES ---
// Menghindari warning "implicit declaration"
void user_game_menu();
void battle_ui();
void matchmaking_menu();
void armory_menu();
void print_header();
void draw_line(int n);
void set_raw_mode(int enable);
void wait_for_enter();
void handle_sigint(int sig);

// --- UTILITY SYSTEM ---

void draw_line(int n) {
    for(int i=0; i<n; i++) printf("-");
    printf("\n");
}

void set_raw_mode(int enable) {
    if (enable && !is_raw) {
        struct termios newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        is_raw = 1;
    } else if (!enable && is_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        is_raw = 0;
    }
}

void wait_for_enter() {
    printf("\nTekan Enter untuk melanjutkan...");
    fflush(stdout);
    if (is_raw) {
        getchar();
    } else {
        // Buffer clearing untuk mode standar
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
}

void handle_sigint(int sig) {
    set_raw_mode(0);
    if(current_idx != -1) {
        sem_wait(&shm_ptr->mutex);
        shm_ptr->users[current_idx].is_logged_in = 0;
        sem_post(&shm_ptr->mutex);
    }
    printf("\n" YEL "[SYSTEM] Logout aman. Terminal di-reset." RESET "\n");
    exit(0);
}

// --- VISUAL & STATS SYSTEM ---

void print_header() {
    printf(CYN BOLD);
    printf("| __ )  / \\|_   _|_   _| |   | ____|   / _ \\|  ___|\n");
    printf("|  _ \\ / _ \\ | |   | | | |   |  _|    | | | | |_   \n");
    printf("| |_) / ___ \\| |   | | | |___| |___   | |_| |  _|  \n");
    printf("|____/_/   \\_\\_|   |_| |_____|_____|   \\___/|_|    \n");
    printf("| ____|_   _| ____|  _ \\|_ _/ _ \\| \\ | |\n");
    printf("|  _|   | | |  _| | |_) || | | | |  \\| |\n");
    printf("| |___  | | | |___|  _ < | | |_| | |\\  |\n");
    printf("|_____| |_| |_____|_| \\_\\___\\___/|_| \\_|\n" RESET);
}

void draw_health_bar(int current, int max) {
    int bar_len = 20;
    if (max <= 0) max = 1;
    float ratio = (float)current / max;
    int filled = (int)(ratio * bar_len);
    if (filled < 0) filled = 0;
    if (filled > bar_len) filled = bar_len;

    printf("[");
    for(int i=0; i<bar_len; i++) {
        if(i < filled) printf(GREEN "█" RESET);
        else printf(" ");
    }
    printf("] %d/%d HP\n", (current < 0 ? 0 : current), max);
}

int get_max_hp(User *u) { return 100 + (u->xp / 10); }

int get_total_dmg(User *u) {
    int w_dmg = 0;
    // Mencari senjata terkuat yang dimiliki (menggunakan data dari arena.h)
    for(int i=5; i>=1; i--) {
        if(u->has_weapon[i]) {
            w_dmg = arm_dmg[i];
            break;
        }
    }
    return 5 + (u->xp / 50) + w_dmg;
}

void add_history(char *opp_name, char *res, int xp, int gold) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char entry[100];
    sprintf(entry, "%02d:%02d | %-12s | %-7s | +%d XP | +%d G", 
            tm.tm_hour, tm.tm_min, opp_name, res, xp, gold);
    
    sem_wait(&shm_ptr->mutex);
    for(int i=9; i>0; i--) {
        strcpy(shm_ptr->users[current_idx].history_log[i], shm_ptr->users[current_idx].history_log[i-1]);
    }
    strcpy(shm_ptr->users[current_idx].history_log[0], entry);
    sem_post(&shm_ptr->mutex);
}

// --- GAMEPLAY MENUS ---

void armory_menu() {
    while(1) {
        system("clear");
        print_header();
        sem_wait(&shm_ptr->mutex);
        User *me = &shm_ptr->users[current_idx];
        printf("\n=== ARMORY SHOP === (Gold: %d)\n", me->money);
        for(int i=1; i<=5; i++) {
            printf("%d. %-15s | %4dG | +%3d Dmg %s\n", 
                   i, arm_names[i], arm_price[i], arm_dmg[i], 
                   me->has_weapon[i] ? GREEN "(OWNED)" RESET : "");
        }
        sem_post(&shm_ptr->mutex);
        
        printf("0. Kembali\nPilihan: ");
        int c; 
        if (scanf("%d", &c) != 1) { while(getchar() != '\n'); continue; }
        
        if(c == 0) break;
        if(c >= 1 && c <= 5) {
            sem_wait(&shm_ptr->mutex);
            if(me->has_weapon[c]) {
                printf(YEL "\nAnda sudah memiliki item ini!\n" RESET);
            } else if(me->money < arm_price[c]) {
                printf(RED "\nGold tidak mencukupi!\n" RESET);
            } else {
                me->money -= arm_price[c];
                me->has_weapon[c] = 1;
                printf(GREEN "\nBerhasil membeli %s!\n" RESET, arm_names[c]);
            }
            sem_post(&shm_ptr->mutex);
            sleep(1);
        }
    }
}

void battle_ui() {
    set_raw_mode(1);
    char logs[5][100] = {"", "", "", "", "Pertarungan Dimulai!"};
    User *me = &shm_ptr->users[current_idx];
    
    // Bot data
    User bot; 
    strcpy(bot.username, "Wild Beast"); 
    bot.health = 100; bot.level = 1; bot.xp = 0;

    while(me->state == 2) {
        system("clear");
        User *opp = (me->last_opponent_idx == -2) ? &bot : &shm_ptr->users[me->last_opponent_idx];
        
        printf(RED BOLD "%-15s" RESET " LVL:%d\n", opp->username, opp->level);
        draw_health_bar(opp->health, (me->last_opponent_idx == -2) ? 100 : get_max_hp(opp));
        
        printf("\n          " YEL "VS" RESET "          \n\n");
        
        printf(GREEN BOLD "%-15s" RESET " LVL:%d\n", me->username, me->level);
        draw_health_bar(me->health, get_max_hp(me));

        printf("\nCOMBAT LOG:\n");
        for(int i=0; i<5; i++) printf("> %s\n", logs[i]);
        printf("\n" BOLD "[a] Attack | [u] Ultimate (x3) | [q] Kabur" RESET "\nChoice: ");
        
        char c = getchar();
        if(c == 'q') {
            sem_wait(&shm_ptr->mutex);
            me->state = 0;
            sem_post(&shm_ptr->mutex);
            add_history(opp->username, "RUN", 15, 30);
            break;
        }

        if(c == 'a' || c == 'u') {
            sem_wait(&shm_ptr->mutex);
            int d = get_total_dmg(me);
            if(c == 'u') d *= 3;
            opp->health -= d;
            
            for(int i=0; i<4; i++) strcpy(logs[i], logs[i+1]);
            sprintf(logs[4], "Kamu hit %s sebesar %d dmg!", opp->username, d);

            if(opp->health > 0) {
                int od = (me->last_opponent_idx == -2) ? 8 : get_total_dmg(opp);
                me->health -= od;
                for(int i=0; i<4; i++) strcpy(logs[i], logs[i+1]);
                sprintf(logs[4], "%s membalas %d dmg!", opp->username, od);
            }
            sem_post(&shm_ptr->mutex);
        }

        if(opp->health <= 0 || me->health <= 0) {
            set_raw_mode(0);
            system("clear");
            sem_wait(&shm_ptr->mutex);
            int rxp, rgold;
            char *status;
            if(opp->health <= 0) {
                printf(GREEN BOLD "=== VICTORY ===\n" RESET);
                rxp = 50; rgold = 120; status = "WIN";
            } else {
                printf(RED BOLD "=== DEFEAT ===\n" RESET);
                rxp = 15; rgold = 30; status = "LOSS";
            }
            me->xp += rxp; me->money += rgold;
            me->level = 1 + (me->xp / 100);
            me->state = 0;
            me->health = get_max_hp(me);
            sem_post(&shm_ptr->mutex);
            
            add_history(opp->username, status, rxp, rgold);
            printf("Reward: +%d XP | +%d Gold\n", rxp, rgold);
            wait_for_enter();
            break;
        }
        usleep(50000);
    }
    set_raw_mode(0);
}

void matchmaking_menu() {
    sem_wait(&shm_ptr->mutex);
    shm_ptr->users[current_idx].state = 1;
    shm_ptr->users[current_idx].queue_start = time(NULL);
    sem_post(&shm_ptr->mutex);

    set_raw_mode(1);
    while(1) {
        system("clear");
        print_header();
        int elapsed = time(NULL) - shm_ptr->users[current_idx].queue_start;
        
        printf("\n" YEL "Mencari lawan... [%ds / 35s]" RESET "\n", elapsed);
        printf("Tekan [q] untuk membatalkan antrean\n");

        if(shm_ptr->users[current_idx].state == 2) { 
            set_raw_mode(0); 
            battle_ui(); 
            break; 
        }
        
        // Non-blocking key check
        struct timeval tv = {0, 500000}; 
        fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
        if(select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0) {
            if(getchar() == 'q') {
                sem_wait(&shm_ptr->mutex); 
                shm_ptr->users[current_idx].state = 0; 
                sem_post(&shm_ptr->mutex);
                break;
            }
        }
    }
    set_raw_mode(0);
}

void user_game_menu() {
    while(1) {
        system("clear");
        print_header();
        sem_wait(&shm_ptr->mutex);
        User *me = &shm_ptr->users[current_idx];
        printf("\n" BOLD "=== PLAYER PROFILE ===" RESET "\n");
        printf("Username : %-15s | Level : %d\n", me->username, me->level);
        printf("Gold     : %-15d | XP    : %d/100\n", me->money, me->xp % 100);
        printf("HP       : %d/%d\n", me->health, get_max_hp(me));
        sem_post(&shm_ptr->mutex);

        printf("\n1. Battle Arena\n2. Armory Shop\n3. Match History\n4. Logout\nPilihan: ");
        int c; 
        if (scanf("%d", &c) != 1) { while(getchar() != '\n'); continue; }

        if(c == 1) matchmaking_menu();
        else if(c == 2) armory_menu();
        else if(c == 3) {
            system("clear");
            printf(BOLD "=== MATCH HISTORY ===\n" RESET);
            draw_line(55);
            sem_wait(&shm_ptr->mutex);
            for(int i=0; i<10; i++) {
                if(strlen(me->history_log[i]) > 0) printf("%s\n", me->history_log[i]);
            }
            sem_post(&shm_ptr->mutex);
            wait_for_enter();
        }
        else if(c == 4) {
            sem_wait(&shm_ptr->mutex);
            shm_ptr->users[current_idx].is_logged_in = 0;
            sem_post(&shm_ptr->mutex);
            break;
        }
    }
}

// --- MAIN SYSTEM ---

int main() {
    // Shared Memory Setup
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if(shmid == -1) { printf(RED "Error: Jalankan ./orion terlebih dahulu!\n" RESET); exit(1); }
    shm_ptr = (SharedData *)shmat(shmid, NULL, 0);
    int msgid = msgget(MSG_KEY, 0666);

    signal(SIGINT, handle_sigint);

    while(1) {
        system("clear");
        print_header();
        printf("\n1. Register\n2. Login\n3. Exit\nPilihan: ");
        int choice;
        if (scanf("%d", &choice) != 1) { while(getchar() != '\n'); continue; }

        if(choice == 3) break;

        if(choice == 1) {
            struct msg_buffer m;
            m.msg_type = 1;
            m.user_data.health = getpid(); // Identifikasi PID
            printf("Username Baru: "); scanf("%s", m.user_data.username);
            printf("Password Baru: "); scanf("%s", m.user_data.password);
            msgsnd(msgid, &m, sizeof(User), 0);
            
            // Tunggu feedback
            msgrcv(msgid, &m, sizeof(User), getpid(), 0);
            if(m.user_data.health != -1) printf(GREEN "Registrasi Berhasil!\n" RESET);
            else printf(RED "Registrasi Gagal (Username duplikat/Server penuh).\n" RESET);
            sleep(2);
        } else if(choice == 2) {
            char u[50], p[50];
            printf("Username: "); scanf("%s", u);
            printf("Password: "); scanf("%s", p);
            
            int found = -1;
            sem_wait(&shm_ptr->mutex);
            for(int i=0; i<MAX_USERS; i++) {
                if(strlen(shm_ptr->users[i].username) > 0 && 
                   strcmp(shm_ptr->users[i].username, u) == 0 && 
                   strcmp(shm_ptr->users[i].password, p) == 0) {
                    if(shm_ptr->users[i].is_logged_in) found = -2;
                    else {
                        shm_ptr->users[i].is_logged_in = 1;
                        current_idx = i;
                        found = i;
                    }
                    break;
                }
            }
            sem_post(&shm_ptr->mutex);

            if(found >= 0) {
                printf(GREEN "Login Berhasil!\n" RESET);
                sleep(1);
                user_game_menu();
            }
            else if(found == -2) { printf(RED "Akun sudah login!\n" RESET); sleep(2); }
            else { printf(RED "Login Gagal!\n" RESET); sleep(2); }
        }
    }
    shmdt(shm_ptr);
    return 0;
}