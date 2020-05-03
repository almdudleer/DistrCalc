#include <pa2345.h>
#include <bits/types/FILE.h>
#include <inc/lib/self.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "ipc.h"
#include "utils.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>

int receive_all(Unit *self, MessageType type, FILE *events_log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    const char *log_fmt;
    timestamp_t time = get_physical_time();
    switch (type) {
        case DONE:
            create_log_text(log_text, log_received_all_done_fmt, time, self->lid);
            break;
        case STARTED:
            create_log_text(log_text, log_received_all_started_fmt, time, self->lid);
            break;
        default:
            break;
    }

    Message *incoming_msg = malloc(sizeof(Message));

    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS;

    int n_received = self->n_nodes - 2;
    while (n_received != 0) {
//        if (timeout <= 0) return -1;
        if (receive_any(self, incoming_msg) < 0) {
            if (errno == EAGAIN) {
                nanosleep(&tw, &tr);
                timeout_ns -= WAIT_TIME_NS;
                if (timeout_ns <= 0) {
                    fprintf(stderr, "timeout exceeded\n");
                    exit(EXIT_FAILURE);
                }
                continue;
            } else {
                perror("receive_any");
                return -1;
            }
        } else {
            n_received--;
            if (incoming_msg->s_header.s_type != type) {
//                printf("Unexpected message type %d\n", incoming_msg->s_header.s_type);
            }
        }
    }

    log_msg(events_log_file, log_text);

    Unit_clear_mask(self);
    free(incoming_msg);

    return 0;
}

int write_nonblock(int fd, char *msg, unsigned long n_bytes) {
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

void create_msg(Message *msg, MessageType type, void *payload, size_t payload_len) {
    timestamp_t current_time = (timestamp_t) time(NULL);

    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_local_time = current_time;
    header.s_payload_len = payload_len;
    header.s_type = type;
    msg->s_header = header;
    sprintf(msg->s_payload, "%s", payload);

}

void create_log_text(char *msg_text, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(msg_text, format, args);
    va_end(args);
}

void log_msg(FILE *log_file, char *msg) {

//    log_file = fopen(events_log, "a");

    write_nonblock(STDOUT_FILENO, msg, strlen(msg));

//    write_nonblock(log_file->_fileno, msg, strlen(msg));

    fflush(log_file);
    if (fprintf(log_file, "%s", msg) < 0) {
        perror("fprintf");
    }
    fflush(log_file);

}

void close_bad_pipes(Unit *self, int n_processes, int **const *pipes) {
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