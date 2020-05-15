#include "entity.h"
#include "ipc.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


void close_bad_pipes(Unit* self, int n_processes, int** const* pipes) {
    for (local_id from = 0; from < (local_id) n_processes; from++) {
        for (local_id to = 0; to < (local_id) n_processes; to++) {
            if (from != to) {
                if ((*self).lid == to) {
                    close(pipes[from][to][1]);
                } else if ((*self).lid == from) {
                    close(pipes[from][to][0]);
                } else {
                    close(pipes[from][to][0]);
                    close(pipes[from][to][1]);
                }
            }
        }
    }
}

void create_log_text(char* msg_text, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(msg_text, format, args);
    va_end(args);
}

void log_msg(FILE* log_file, char* msg) {
    write_nonblock(STDOUT_FILENO, msg, strlen(msg));
    fflush(log_file);
    if (fprintf(log_file, "%s", msg) < 0) {
        perror("fprintf");
    }
    fflush(log_file);

}

int write_nonblock(int fd, char* msg, unsigned long n_bytes) {
//    int timeout = 1000;
    while (1) {
//        if (timeout <= 0) return -1;
        if (write(fd, msg, n_bytes) < 0) {
            if (errno == EAGAIN) {
//                timeout--;
                sleep(1);
                continue;
            } else {
                perror("write_nonblock");
                return -1;
            }
        }
        return 0;
    }
}
