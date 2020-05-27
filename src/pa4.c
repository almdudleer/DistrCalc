#include <string.h>
#include "entity.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "banking.h"
#include "utils.h"
#include "common.h"
#include "ipc_utils.h"
#include "lamp_time.h"
#include "pa2345.h"
#include <fcntl.h>
#include <getopt.h>

int started_left = -1, done_left = -1;

void send_started(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];

    timestamp_t time = inc_lamport_time();
    create_log_text(log_text, log_started_fmt, time, self->lid, getpid(), getppid());

    Message* started_msg = Message_new(STARTED, log_text, strlen(log_text));
//    printf("%d sending STARTED: %d\n", self->lid, STARTED);
    if (send_multicast(self, started_msg) != 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

    log_msg(log_file, log_text);
    Message_free(started_msg);
}

void send_done(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];

    timestamp_t time = inc_lamport_time();
    create_log_text(log_text, log_done_fmt, time, self->lid);

    Message* done_msg = Message_new(DONE, log_text, strlen(log_text));
//    printf("%d sending DONE: %d\n", self->lid, DONE);
    if (send_multicast(self, done_msg) < 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

    log_msg(log_file, log_text);
    Message_free(done_msg);
}

void receive_all(Unit* self, MessageType type) {
    Message* in_msg;
    while (1) {
        in_msg = Message_empty();
        if (receive_any_or_die(self, in_msg) == 0) {
            switch (in_msg->s_header.s_type) {
                case STARTED:
                    started_left--;
                    if (type == STARTED && started_left == 0) {
                        started_left = -1;
                        return;
                    }
                    break;
                case DONE:
                    done_left--;
                    if (type == DONE && done_left == 0) {
                        done_left = -1;
                        return;
                    }
                    break;
                default:
                    fprintf(stderr, "Process %d: bad message type %d, expected %d\n",
                            self->lid, in_msg->s_header.s_type, type);
                    exit(EXIT_FAILURE);
            }
        } else {
            perror("receive_all: receive_any_or_die");
            exit(EXIT_FAILURE);
        }
    }
}

void child_main(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];

    started_left = done_left = self->n_nodes - 2;

    send_started(self, log_file);
    receive_all(self, STARTED);
    create_log_text(log_text, log_received_all_started_fmt, get_lamport_time(), self->lid);
    log_msg(log_file, log_text);

    send_done(self, log_file);
    receive_all(self, DONE);
    create_log_text(log_text, log_received_all_done_fmt, get_lamport_time(), self->lid);
    log_msg(log_file, log_text);

    exit(EXIT_SUCCESS);
}

void parent_main(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    started_left = done_left = self->n_nodes - 1;

    // Run main job
    receive_all(self, STARTED);
    create_log_text(log_text, log_received_all_started_fmt, get_lamport_time(), self->lid);
    log_msg(log_file, log_text);

    receive_all(self, DONE);
    create_log_text(log_text, log_received_all_done_fmt, get_lamport_time(), self->lid);
    log_msg(log_file, log_text);
}

int main(int argc, char** argv) {

    // Open log files
    FILE* events_log_file = fopen(events_log, "w");
    FILE* pipes_log_file = fopen(pipes_log, "w");

    // Define variables to be set from opts
    local_id n_processes;
    static int mutex_flag = 0;

    // Store opts
    char* endptr;
    int opt;
    int option_index = 0;
    static struct option long_options[] =
            {
                    {"mutexl", no_argument,       &mutex_flag, 1},
                    {"nproc",  required_argument, 0,           'p'},
            };

    // Read opts
    while ((opt = getopt_long(argc, argv, "p:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                n_processes = (local_id) ((int) strtol(optarg, &endptr, 10) + 1);
                if (n_processes < 2) {
                    fprintf(stderr, "not enough processes: %d", n_processes);
                    exit(EXIT_FAILURE);
                }
                break;
            case 0:
                break;
            default:
                abort();
        }
    }

    // Create pipes
    int*** pipes = alloc_pipes((size_t) n_processes, (size_t) n_processes, 2);
    for (local_id from = 0; from < (local_id) n_processes; from++) {
        for (local_id to = 0; to < (local_id) n_processes; to++) {
            if (to == from) continue;
            if (pipe(pipes[from][to]) < 0) {
                perror("pipe");
                exit(-1);
            }

            // Set read pipes to nonblock mode
            int flags1;
            int flags2;
            int fd1 = pipes[from][to][0];
            int fd2 = pipes[from][to][1];
            if ((flags1 = fcntl(fd1, F_GETFL)) < 0) return -1;
            if ((flags2 = fcntl(fd2, F_GETFL)) < 0) return -1;
            if (fcntl(fd1, F_SETFL, (unsigned int) flags1 | ((unsigned int) O_NONBLOCK)) < 0) return -1;
            if (fcntl(fd2, F_SETFL, (unsigned int) flags2 | ((unsigned int) O_NONBLOCK)) < 0) return -1;
            fprintf(pipes_log_file, "pipe from %d to %d opened; read end: %d, write end: %d\n",
                    from, to, pipes[from][to][0], pipes[from][to][1]);
        }
    }
    fclose(pipes_log_file);

    // Fork child processes
    for (local_id lid = 1; lid < (local_id) n_processes; lid++) {
        Unit* self = Unit_new(lid, n_processes, pipes);
        switch (fork()) {
            case -1: {
                perror("fork");
                exit(1);
            }
            case 0: {
                close_bad_pipes(self, n_processes, pipes);
                child_main(self, events_log_file);
                Unit_free(self);
            }
        }
    }

    // Run parent process
    Unit* parent_unit = Unit_new(PARENT_ID, n_processes, pipes);
    close_bad_pipes(parent_unit, n_processes, pipes);
    parent_main(parent_unit, events_log_file);
    Unit_free(parent_unit);

    // Wait for children
    for (int i = 0; i < n_processes; i++)
        wait(NULL);

    // Clean up
    fclose(events_log_file);

    return 0;
}
