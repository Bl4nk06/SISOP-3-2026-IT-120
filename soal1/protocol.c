#include "protocol.h"

void get_timestamp(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", t);
}

void log_message(const char *role, const char *status, const char *msg) {
    FILE *log_file = fopen("history.log", "a");
    if (!log_file) return;

    char ts[20];
    get_timestamp(ts);
    fprintf(log_file, "[%s] [%s] [%s] %s\n", ts, role, status, msg);
    fclose(log_file);
}