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


int receive_or_die(Unit* self, local_id from, Message* msg) {
    // set timeout
    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS * 1.5;

    // wait for data
    while (1) {
        if (receive(self, from, msg) < 0) {
            if (errno != EAGAIN) {
                return -1;
            } else {
                nanosleep(&tw, &tr);
                timeout_ns -= WAIT_TIME_NS;
                if (timeout_ns <= 0) {
                    errno = EBUSY;
                    return -1;
                }
            }
        } else return 0;
    }
}

int receive_all(Unit* self, MessageType type, FILE* events_log_file) {
    Message* incoming_msg = malloc(sizeof(Message));

    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS;

    int to_receive = self->n_nodes - 2;
    while (to_receive != 0) {
        if (receive_any(self, incoming_msg) < 0) {
            if (errno == EAGAIN) {
                nanosleep(&tw, &tr);
                timeout_ns -= WAIT_TIME_NS;
                if (timeout_ns <= 0) {
                    fprintf(stderr, "receive_all: timeout exceeded\n");
                    exit(EXIT_FAILURE);
                }
                continue;
            } else {
                perror("receive_any");
                return -1;
            }
        } else {
            to_receive--;
            on_receive_one(self, type, incoming_msg, events_log_file);
        }
    }

    on_receive_all(self, type, events_log_file);

    Unit_clear_mask(self);
    free(incoming_msg);

    return 0;
}

void on_receive_all(Unit* self, MessageType type, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    timestamp_t time = get_physical_time();
    switch (type) {
        case DONE:
            create_log_text(log_text, log_received_all_done_fmt, time, self->lid);
            log_msg(log_file, log_text);
            break;
        case STARTED:
            create_log_text(log_text, log_received_all_started_fmt, time, self->lid);
            log_msg(log_file, log_text);
            break;
        case BALANCE_HISTORY:
            printf("Got all BALANCE_HISTORY");
        default:
            break;
    }
}

void on_receive_one(Unit* self, MessageType type, Message* msg, FILE* log_file) {
//    if (msg->s_header.s_type != type) {
//         printf("Unexpected message type %d\n", msg->s_header.s_type);
//    }
    if (msg->s_header.s_type != type) {
        printf("%d: bad type %d expected %d\n", self->lid, msg->s_header.s_type, type);
    }

    switch (type) {
        case BALANCE_HISTORY: {
            BalanceHistory bh = *((BalanceHistory*) msg->s_payload);
            printf("BALANCE HISTORY: %d, LEN: %d\n", bh.s_id, bh.s_history_len);
            for (int i = 0; i < bh.s_history_len; i++) {
                printf("%d:%d:%d\n", bh.s_id, bh.s_history[i].s_time, bh.s_history[i].s_balance);
            }
            break;
        }
        default:
            break;
    }
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

void create_msg(Message* msg, MessageType type, void* payload, size_t payload_len) {
    timestamp_t current_time = get_physical_time();

    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_local_time = current_time;
    header.s_payload_len = payload_len;
    header.s_type = type;
    msg->s_header = header;
    memcpy(msg->s_payload, payload, payload_len);
//    sprintf(msg->s_payload, "%s", payload);

}

void create_log_text(char* msg_text, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(msg_text, format, args);
    va_end(args);
}

void log_msg(FILE* log_file, char* msg) {

//    log_file = fopen(events_log, "a");

    write_nonblock(STDOUT_FILENO, msg, strlen(msg));

//    write_nonblock(log_file->_fileno, msg, strlen(msg));

    fflush(log_file);
    if (fprintf(log_file, "%s", msg) < 0) {
        perror("fprintf");
    }
    fflush(log_file);

}

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