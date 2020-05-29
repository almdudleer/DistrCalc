#include <string.h>
#include "entity.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "banking.h"
#include "utils.h"
#include "common.h"
#include "lamp_time.h"
#include "pa2345.h"
#include <fcntl.h>
#include <getopt.h>
#include <queue.h>
#include <errno.h>
#include <time.h>

static int mutex_flag = 0;

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

void send_done(Unit* self, FILE* log_file, const int* done_left) {
    char log_text[MAX_PAYLOAD_LEN];

    timestamp_t time = inc_lamport_time();
    create_log_text(log_text, log_done_fmt, time, self->lid);

    Message* done_msg = Message_new(DONE, log_text, strlen(log_text));
//    printf("process %d - sending DONE: %d\n", self->lid, DONE);
    if (send_multicast(self, done_msg) < 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

    self->done = 1;
    log_msg(log_file, log_text);
    Message_free(done_msg);

    if (*done_left <= 0) {
        Unit_free(self);
        exit(EXIT_SUCCESS);
    }
}

int request_cs(const void* self) {
    Unit* this = (Unit*) self;
    inc_lamport_time();
    Message* msg = Message_new(CS_REQUEST, "", 0);
    if (send_multicast(this, msg) != 0) {
        perror("send_multicast");
        return -1;
    }
    cs_request* request = malloc(sizeof(cs_request));
    request->time = msg->s_header.s_local_time;
    request->lid = this->lid;
    enqueue(this->que, request);
//    printf("process %d - insert self %d,%d: ", this->lid, request->lid, request->time);
//    queue_print(this->que);
//    printf("process %d - request_cs\n", this->lid);
    return 0;
}

int release_cs(const void* self) {
    Unit* this = (Unit*) self;
//    cs_request* request = peek(this->que);
//    printf("process %d - remove self %d,%d: ", this->lid, request->lid, request->time);
//    queue_print(this->que);
    dequeue(this->que);
    inc_lamport_time();
    Message* msg = Message_new(CS_RELEASE, "", 0);
    if (send_multicast(this, msg) != 0) {
        perror("send_multicast");
        return -1;
    }
//    printf("process %d - release_cs\n", this->lid);
    return 0;
}

int reply_cs(const void* self, cs_request* request) {
    Unit* this = (Unit*) self;
    enqueue(this->que, request);
//    printf("process %d - insert alien %d,%d: ", this->lid, request->lid, request->time);
//    queue_print(this->que);
    inc_lamport_time();
    Message* msg = Message_new(CS_REPLY, "", 0);
    if (send(this, request->lid, msg) != 0) {
        perror("send");
        return -1;
    }
    return 0;
}

void check_cs_enter
        (Unit* self, Message* in_msg, cs_request** last_request, int* replies_left, int iters_total,
         int* iters_left, FILE* log_file, int* done_left, int* replies_mask) {
//    printf("%d checks cs enter\n", self->lid);
    if (*last_request != NULL) {
        char log_text[MAX_PAYLOAD_LEN];

        cs_request* que_top = peek(self->que);
        if ((*replies_left <= 0) && (que_top->lid == (*last_request)->lid)) {
//            printf("process %d - enter cs\n", self->lid);
            create_log_text(log_text, log_loop_operation_fmt, self->lid,
                            iters_total - *iters_left + 1, iters_total);
            print(log_text);
            (*iters_left)--;
            release_cs(self);
//            printf("process %d - check iters left: %d\n", self->lid, *iters_left);

            free(*last_request);
            *last_request = NULL;
            if (*iters_left <= 0)
                send_done(self, log_file, done_left);

        }
    }
}

void proc_main(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    int started_left, done_left;
    int is_parent = (self->lid == PARENT_ID);
    cs_request* last_request = NULL;
    int replies_total = self->n_nodes - 2;
    int replies_left = replies_total;
    int iters_total = self->lid*5;
    int iters_left = iters_total;
    int* replies_mask = calloc(self->n_nodes - 1, sizeof(int));

    Message* in_msg;

    if (is_parent) {
        started_left = done_left = self->n_nodes - 1;
    } else {
        started_left = done_left = self->n_nodes - 2;
    }

    // Timeouts
    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS;

    send_started(self, log_file);
    while (1) {
        in_msg = Message_empty();
        if (receive_any(self, in_msg) == 0) {
            switch (in_msg->s_header.s_type) {
                case STARTED:
//                    printf("process %d - got STARTED from %d\n", self->lid, self->last_message_lid);
                    started_left--;
                    if (started_left == 0) {
                        create_log_text(log_text, log_received_all_started_fmt, get_lamport_time(), self->lid);
                        log_msg(log_file, log_text);
                        if (is_parent) self->done = 1;
                    }
                    break;
                case DONE:
//                    printf("process %d - got DONE from %d\n", self->lid, self->last_message_lid);
                    done_left--;
//                    printf("process %d - done left: %d\n", self->lid, done_left);
                    if (done_left == 0) {
                        create_log_text(log_text, log_received_all_done_fmt, get_lamport_time(), self->lid);
                        log_msg(log_file, log_text);
                        if (self->done == 1) {
//                            printf("process %d - EXIT\n", self->lid);
                            Unit_free(self);
                            exit(EXIT_SUCCESS);
                        }
                    }
                    break;
                case CS_REQUEST:
//                    printf("process %d - got CS_REQUEST from %d\n", self->lid, self->last_message_lid);
                    if (!is_parent) {
                        cs_request* request = malloc(sizeof(cs_request));
                        request->time = in_msg->s_header.s_local_time;
                        request->lid = self->last_message_lid;
                        reply_cs(self, request);
                    }
                    break;
                case CS_RELEASE:
//                    printf("process %d - got CS_RELEASE from %d, ", self->lid, self->last_message_lid);
                    if (!is_parent) {
                        cs_request* request = peek(self->que);
//                        printf("process %d - remove alien %d,%d: ", self->lid, request->lid, request->time);
//                        queue_print(self->que);
                        request = dequeue(self->que);
                        if (request->lid != self->last_message_lid) {
                            fprintf(stderr, "%d sees wrong local id at dequeue: %d\n", self->lid, request->lid);
                            exit(EXIT_FAILURE);
                        }
                    }
                    break;
                case CS_REPLY: {
//                    printf("process %d - got CS_REPLY from %d\n", self->lid, self->last_message_lid);
                    if (!is_parent) {
                        if (!last_request) {
                            fprintf(stderr, "%d got cs reply from %d, which wasn't requested\n", self->lid,
                                    self->last_message_lid);
                            exit(EXIT_FAILURE);
                        } else {
                            if (in_msg->s_header.s_local_time > last_request->time ||
                                (in_msg->s_header.s_local_time == last_request->time
                                 && self->last_message_lid > self->lid)) {
                                if (replies_mask[self->last_message_lid] == 0) {
                                    replies_left--;
                                    replies_mask[self->last_message_lid] = 1;
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    fprintf(stderr, "Process %d: unexpected message type %d\n",
                            self->lid, in_msg->s_header.s_type);
                    exit(EXIT_FAILURE);
            }
//            printf("process %d - last_request: %p\n", self->lid, last_request);
            if (last_request) {
                check_cs_enter(self, in_msg,
                               &last_request, &replies_left,
                               iters_total, &iters_left, log_file, &done_left, replies_mask);
            }
        } else {
            if (errno == EAGAIN) {
                // do job in meantime
//                printf("%d does job in meantime, done_left=%d, iters_left=%d\n", self->lid, done_left, iters_left);
                if (!last_request && !is_parent && (started_left <= 0) && (iters_left > 0)) {
                    request_cs(self);
                    last_request = malloc(sizeof(cs_request));
                    last_request->lid = self->lid;
                    last_request->time = get_lamport_time();
                    replies_left = replies_total;
                    for (int i = 0; i < self->n_nodes; i++) {
                        replies_mask[i] = 0;
                    }
                }

                // wait to lower CPU load
                nanosleep(&tw, &tr);
                if (TIMEOUTS) {
                    timeout_ns -= WAIT_TIME_NS;
                    if (timeout_ns <= 0) {
                        errno = EBUSY;
                        perror("receive_any");
                        exit(EXIT_FAILURE);
                    }
                }
            } else {
                perror("receive_any");
                exit(EXIT_FAILURE);
            }
        }
        Message_free(in_msg);
    }
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
        Unit* self = Unit_new(lid, n_processes, pipes);
        switch (fork()) {
            case -1: {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            case 0: {
                close_bad_pipes(self, n_processes, pipes);
                proc_main(self, events_log_file);
                exit(EXIT_SUCCESS);
            }
        }
    }

    // Run parent process
    Unit* parent_unit = Unit_new(PARENT_ID, n_processes, pipes);
    close_bad_pipes(parent_unit, n_processes, pipes);
    proc_main(parent_unit, events_log_file);

    // Wait for children
    for (int i = 0; i < n_processes; i++)
        wait(NULL);

    // Clean up
    fclose(events_log_file);
    return 0;
}
