#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include "entity.h"
#include "banking.h"
#include "lamp_time.h"
#include "utils.h"
#include "pa2345.h"
#include "queue.h"


static int mutex_flag = 0;

void send_started(Unit* self) {
    char log_text[MAX_PAYLOAD_LEN];

    timestamp_t time = inc_lamport_time();
    create_log_text(log_text, log_started_fmt, time, self->lid, getpid(), getppid(), 0);

    Message* started_msg = Message_new(STARTED, log_text, strlen(log_text));
//    printf("%d sending STARTED: %d\n", self->lid, STARTED);
    if (send_multicast(self, started_msg) != 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

    log_msg(self->log_file, log_text);
    Message_free(started_msg);
}

void send_done(Unit* self) {
    char log_text[MAX_PAYLOAD_LEN];
    timestamp_t time = inc_lamport_time();
    create_log_text(log_text, log_done_fmt, time, self->lid, 0);
    Message* done_msg = Message_new(DONE, log_text, strlen(log_text));
    if (send_multicast(self, done_msg) < 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }
    log_msg(self->log_file, log_text);
    Message_free(done_msg);
}

int request_cs(const void* self) {
    Unit* this = (Unit*) self;
    inc_lamport_time();
    Message* msg = Message_new(CS_REQUEST, "", 0);
    if (send_multicast(this, msg) != 0) {
        perror("send_multicast");
        return -1;
    }
    CsRequest* request = malloc(sizeof(CsRequest));
    request->time = msg->s_header.s_local_time;
    request->lid = this->lid;
    enqueue(this->que, request);
    return 0;
}

int release_cs(const void* self) {
    Unit* this = (Unit*) self;
    dequeue(this->que);
    inc_lamport_time();
    Message* msg = Message_new(CS_RELEASE, "", 0);
    if (send_multicast(this, msg) != 0) {
        perror("send_multicast");
        return -1;
    }
    return 0;
}

int reply_cs(const void* self, CsRequest* request) {
    Unit* this = (Unit*) self;
    enqueue(this->que, request);
    inc_lamport_time();
    Message* msg = Message_new(CS_REPLY, "", 0);
    if (send(this, request->lid, msg) != 0) {
        perror("send");
        return -1;
    }
    return 0;
}

void handle_started(Unit* self) {
    char log_text[MAX_PAYLOAD_LEN];
    self->limits->started_left--;
    if (self->limits->started_left == 0) {
        create_log_text(log_text, log_received_all_started_fmt, get_lamport_time(), self->lid);
        log_msg(self->log_file, log_text);
        if (self->is_parent) self->done = 1;
    }
}

void handle_done(Unit* self) {
    char log_text[MAX_PAYLOAD_LEN];
    self->limits->done_left--;
    if (self->limits->done_left == 0) {
        create_log_text(log_text, log_received_all_done_fmt, get_lamport_time(), self->lid);
        log_msg(self->log_file, log_text);
    }
}

void handle_cs_reply(Unit* self, Message* in_msg) {
    if (in_msg->s_header.s_local_time > self->last_request->time ||
        (in_msg->s_header.s_local_time == self->last_request->time
         && self->last_msg_from > self->lid)) {
        if (self->replies_mask[self->last_msg_from] == 0) {
            self->limits->replies_left--;
            self->replies_mask[self->last_msg_from] = 1;
        }
    }
}

void handle_cs_request(Unit* self, Message* in_msg) {
    CsRequest* request = malloc(sizeof(CsRequest));
    request->time = in_msg->s_header.s_local_time;
    request->lid = self->last_msg_from;
    reply_cs(self, request);
}

void handle_cs_release(Unit* self) {
    if (!cut(self->que, self->last_msg_from)) {
        fprintf(stderr, "process %d - couldn't find %d on the queue\n",
                self->lid, self->last_msg_from);
        exit(EXIT_FAILURE);
    }
}

void critical_section(Unit* self) {
    char log_text[MAX_PAYLOAD_LEN];
    create_log_text(log_text, log_loop_operation_fmt, self->lid,
                    self->limits->iters_total - self->limits->iters_left + 1, self->limits->iters_total);
    print(log_text);
    (self->limits->iters_left)--;
}

int seize_cs(Unit* self) {
    CsRequest* que_top = peek(self->que);
    if ((self->limits->replies_left == 0) && (que_top->lid == self->lid)) {
        return 1;
    }
    return 0;
}

void job(Unit* self) {
    if (mutex_flag) {
        if (self->last_request) {
            if (seize_cs(self)) {
                critical_section(self);
                release_cs(self);
                free(self->last_request);
                self->last_request = NULL;
            }
        } else {

            request_cs(self);
            self->last_request = malloc(sizeof(CsRequest));
            self->last_request->lid = self->lid;
            self->last_request->time = get_lamport_time();
            self->limits->replies_left = self->limits->replies_total;
            for (int i = 0; i < self->n_nodes; i++) {
                self->replies_mask[i] = 0;
            }

        }
    } else {
        critical_section(self);
    }
}

void proc_main(Unit* self) {
    close_bad_pipes(self, self->n_nodes, self->pipes);

    Message* in_msg;

    // Timeouts
    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;

    if (!self->is_parent) send_started(self);

    while ((self->done == 0) || (self->limits->done_left > 0)) {
        in_msg = Message_empty();
        if (receive_any(self, in_msg) == 0) {
            switch (in_msg->s_header.s_type) {
                case STARTED: {
                    handle_started(self);
                    break;
                }
                case DONE: {
                    handle_done(self);
                    break;
                }
                case CS_REQUEST: {
                    if (!self->is_parent && mutex_flag) {
                        handle_cs_request(self, in_msg);
                    }
                    break;
                }
                case CS_RELEASE: {
                    if (!self->is_parent && mutex_flag) {
                        handle_cs_release(self);
                    }
                    break;
                }
                case CS_REPLY: {
                    if (!self->is_parent && mutex_flag) {
                        handle_cs_reply(self, in_msg);
                    }
                    break;
                }
                default: {
                    fprintf(stderr, "Process %d: unexpected message type %d\n",
                            self->lid, in_msg->s_header.s_type);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            if (errno == EAGAIN) {
                // do job in meantime
                if (!self->limits->started_left) {
                    if (self->limits->iters_left != 0) job(self);
                    else if (!self->done) {
                        self->done = 1;
                        if (!self->is_parent) send_done(self);
                    }
                }

                // wait to lower CPU load
                nanosleep(&tw, &tr);
            } else {
                perror("receive_any");
                exit(EXIT_FAILURE);
            }
        }
        Message_free(in_msg);
    }
    Unit_free(self);
}

int main(int argc, char** argv) {
    // Open log files
    FILE* events_log_file = fopen(events_log, "w");
    FILE* pipes_log_file = fopen(pipes_log, "w");

    // Define variables to be set from opts
    local_id n_processes;

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
                exit(EXIT_FAILURE);
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
        Unit* self = Unit_new(lid, n_processes, pipes, events_log_file);
        switch (fork()) {
            case -1: {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            case 0: {
                proc_main(self);
                exit(EXIT_SUCCESS);
            }
        }
    }

    // Run parent process
    Unit* parent_unit = Unit_new(PARENT_ID, n_processes, pipes, events_log_file);
    proc_main(parent_unit);

    // Wait for children
    for (int i = 0; i < n_processes; i++)
        wait(NULL);

    // Clean up
    fclose(events_log_file);
    return 0;
}
