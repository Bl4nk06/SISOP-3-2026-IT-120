#include "arena.h"

SharedData *shm_ptr;

void save_db() {
    FILE *f = fopen("players.db", "wb");
    if(f) { fwrite(shm_ptr, sizeof(SharedData), 1, f); fclose(f); }
}

void load_db() {
    FILE *f = fopen("players.db", "rb");
    if (f) {
        fread(shm_ptr, sizeof(SharedData), 1, f);
        fclose(f);
        for(int i=0; i<MAX_USERS; i++) {
            shm_ptr->users[i].is_logged_in = 0;
            shm_ptr->users[i].state = 0;
        }
    } else {
        memset(shm_ptr, 0, sizeof(SharedData));
        for(int i=0; i<MAX_USERS; i++) shm_ptr->users[i].username[0] = '\0';
    }
}

int main() {
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    shm_ptr = (SharedData *)shmat(shmid, NULL, 0);
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    
    load_db();
    printf(CYN BOLD "Orion Server Aktif. Menunggu Prajurit...\n" RESET);

    struct msg_buffer msg;
    while (1) {
        if (msgrcv(msgid, &msg, sizeof(User), 1, IPC_NOWAIT) > 0) {
            int exists = 0, slot = -1;
            for(int i=0; i<MAX_USERS; i++) {
                if (strlen(shm_ptr->users[i].username) > 0 && strcmp(shm_ptr->users[i].username, msg.user_data.username) == 0) { exists = 1; break; }
                if (strlen(shm_ptr->users[i].username) == 0 && slot == -1) slot = i;
            }
            if (!exists && slot != -1) {
                shm_ptr->users[slot] = msg.user_data;
                shm_ptr->users[slot].health = 100;
                shm_ptr->users[slot].money = 0;
                save_db();
                msg.msg_type = 3;
            } else msg.msg_type = 4;
            msgsnd(msgid, &msg, sizeof(User), 0);
        }

        for(int i=0; i<MAX_USERS; i++) {
            if (shm_ptr->users[i].state == 1) {
                for(int j=0; j<MAX_USERS; j++) {
                    if (i != j && shm_ptr->users[j].state == 1) {
                        shm_ptr->users[i].state = 2; shm_ptr->users[j].state = 2;
                        shm_ptr->users[i].last_opponent_idx = j;
                        shm_ptr->users[j].last_opponent_idx = i;
                    }
                }
            }
        }
        usleep(100000);
    }
    return 0;
}