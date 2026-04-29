#include "arena.h"

SharedData *shm_ptr;

void save_db() {
    FILE *f = fopen("players.db", "wb");
    if (f) {
        fwrite(shm_ptr, sizeof(SharedData), 1, f);
        fclose(f);
    }
}

void load_db() {
    FILE *f = fopen("players.db", "rb");
    if (f) {
        fread(shm_ptr, sizeof(SharedData), 1, f);
        fclose(f);
        // Re-inisialisasi semaphore agar fresh setiap server nyala
        sem_init(&shm_ptr->mutex, 1, 1);
        // Reset status login dan state untuk semua user
        for (int i = 0; i < MAX_USERS; i++) {
            shm_ptr->users[i].is_logged_in = 0;
            shm_ptr->users[i].state = 0;
        }
        printf(YEL "[SYSTEM] Database dimuat dan status di-reset.\n" RESET);
    } else {
        memset(shm_ptr, 0, sizeof(SharedData));
        sem_init(&shm_ptr->mutex, 1, 1);
        printf(YEL "[SYSTEM] Database baru dibuat.\n" RESET);
    }
}

int main() {
    // Inisialisasi Shared Memory
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    shm_ptr = (SharedData *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) { perror("shmat"); exit(1); }

    // Inisialisasi Message Queue
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);

    load_db();

    printf(GREEN "[SERVER] Orion Arena berjalan...\n" RESET);

    while (1) {
        struct msg_buffer msg;
        
        // 1. Logika Registrasi (Non-blocking rcv)
        if (msgrcv(msgid, &msg, sizeof(User), 1, IPC_NOWAIT) != -1) {
            sem_wait(&shm_ptr->mutex);
            int slot = -1, exists = 0;
            for (int i = 0; i < MAX_USERS; i++) {
                if (strlen(shm_ptr->users[i].username) > 0 && 
                    strcmp(shm_ptr->users[i].username, msg.user_data.username) == 0) {
                    exists = 1;
                    break;
                }
                if (slot == -1 && strlen(shm_ptr->users[i].username) == 0) slot = i;
            }

            if (!exists && slot != -1) {
                strcpy(shm_ptr->users[slot].username, msg.user_data.username);
                strcpy(shm_ptr->users[slot].password, msg.user_data.password);
                shm_ptr->users[slot].money = 150; // Modal awal
                shm_ptr->users[slot].level = 1;
                shm_ptr->users[slot].xp = 0;
                shm_ptr->users[slot].health = 100;
                shm_ptr->users[slot].is_logged_in = 0;
                shm_ptr->users[slot].state = 0;
                
                save_db();
                msg.msg_type = msg.user_data.health; // Menggunakan field health sebagai tempat kirim PID
                msg.user_data.health = 100; // Penanda sukses
                printf(GREEN "[REGIS] User '%s' berhasil terdaftar.\n" RESET, msg.user_data.username);
            } else {
                msg.msg_type = msg.user_data.health; 
                msg.user_data.health = -1; // Penanda gagal
                printf(RED "[REGIS] Pendaftaran '%s' gagal (Penuh/Duplikat).\n" RESET, msg.user_data.username);
            }
            sem_post(&shm_ptr->mutex);
            msgsnd(msgid, &msg, sizeof(User), 0);
        }

        // 2. Logika Matchmaking (PVP & PVE Bot)
        time_t now = time(NULL);
        for (int i = 0; i < MAX_USERS; i++) {
            if (shm_ptr->users[i].state == 1) { // User sedang antre
                int found_opponent = 0;

                // Cek Lawan Manusia
                for (int j = i + 1; j < MAX_USERS; j++) {
                    if (shm_ptr->users[j].state == 1) {
                        sem_wait(&shm_ptr->mutex);
                        shm_ptr->users[i].state = 2;
                        shm_ptr->users[j].state = 2;
                        shm_ptr->users[i].last_opponent_idx = j;
                        shm_ptr->users[j].last_opponent_idx = i;
                        sem_post(&shm_ptr->mutex);
                        
                        printf(GREEN "[BATTLE] %s VS %s DIMULAI!\n" RESET, 
                               shm_ptr->users[i].username, shm_ptr->users[j].username);
                        found_opponent = 1;
                        break;
                    }
                }

                // Cek Timer untuk VS Bot (35 detik)
                if (!found_opponent && (now - shm_ptr->users[i].queue_start >= 35)) {
                    sem_wait(&shm_ptr->mutex);
                    shm_ptr->users[i].state = 2;
                    shm_ptr->users[i].last_opponent_idx = -2; // Kode VS Bot
                    sem_post(&shm_ptr->mutex);
                    
                    printf(YEL "[PVE] %s VS Wild Beast dimulai (Timeout 35s)!\n" RESET, 
                           shm_ptr->users[i].username);
                }
            }
        }
        
        // Auto-save berkala setiap loop
        save_db();
        usleep(500000); // 0.5 detik loop rate untuk hemat CPU
    }

    shmdt(shm_ptr);
    return 0;
}