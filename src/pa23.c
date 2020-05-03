#include "string.h"
#include <self.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "banking.h"
#include "utils.h"
#include "common.h"
#include <pa2345.h>

void await_orders(Unit *self, FILE *log_file);

void child_main(Unit *self, FILE *events_log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    Message *msg = malloc(sizeof(Message));

    // send started
    timestamp_t time = get_physical_time();
    create_log_text(log_text, log_started_fmt, time, self->lid, getpid(), getppid(), self->balance);
    create_msg(msg, STARTED, log_text, strlen(log_text));

    if (send_multicast(self, msg) != 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

//    char events_log[10];
//    sprintf(events_log, "events_%d.log", self->lid);
    log_msg(events_log_file, log_text);


    // receive all started

    if (receive_all(self, STARTED, events_log_file) != 0) {
        perror("receive_all");
        exit(EXIT_FAILURE);
    }

    await_orders(self, events_log_file);

    // send done
    time = get_physical_time();
    create_log_text(log_text, log_done_fmt, time, self->lid, self->balance);
    create_msg(msg, DONE, log_text, strlen(log_text));

    if (send_multicast(self, msg) < 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

    log_msg(events_log_file, log_text);

    // receive all done
    if (receive_all(self, DONE, events_log_file) < 0) {
        perror("receive_all");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {

    int opt;
    local_id n_processes = 0;
    char *endptr;
    balance_t** balances_pointer;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        if (opt == 'p') {
            n_processes = (int) strtol(optarg, &endptr, 10) + 1;
            if (n_processes < 2) {
                fprintf(stderr, "not enough processes: %d", n_processes);
                exit(EXIT_FAILURE);
            }
            balance_t* balances = malloc((n_processes)*sizeof(balance_t));
            balances_pointer = &balances;
            for (int i = 1; i < n_processes; i++) {
                balances[i] = strtol(argv[optind], &endptr, 10);
                optind++;
            }
        } else {
            abort();
        }
    }

    int ***pipes = alloc_pipes((size_t) n_processes, (size_t) n_processes, 2);

    FILE *events_log_file = fopen(events_log, "w");
    FILE *pipes_log_file = fopen(pipes_log, "w");

    for (local_id from = 0; from < (local_id) n_processes; from++) {
        for (local_id to = 0; to < (local_id) n_processes; to++) {
            if (to == from) continue;
            if (pipe(pipes[from][to]) < 0) {
                perror("pipe");
                exit(-1);
            }
            fprintf(pipes_log_file, "pipe from %d to %d opened; read end: %d, write end: %d\n",
                    from, to, pipes[from][to][0], pipes[from][to][1]);
        }
    }
    fclose(pipes_log_file);

    for (local_id lid = 1; lid < (local_id) n_processes; lid++) {
        Unit self;
        Unit_new(&self, lid, n_processes, pipes, (*balances_pointer)[lid]);

        switch (fork()) {
            case -1: {
                perror("fork");
                exit(1);
            }
            case 0: {
                close_bad_pipes(&self, n_processes, pipes);
                child_main(&self, events_log_file);
            }
            default: {
            }
        }
    }
    Unit self;
    Unit_new(&self, PARENT_ID, n_processes, pipes, 5);
    close_bad_pipes(&self, n_processes, pipes);

    Message stop_msg;
    create_msg(&stop_msg, STOP, "", 0);

    receive_all(&self, STARTED, events_log_file);
    bank_robbery(&self, n_processes - 1);
    send_multicast(&self, &stop_msg);
    receive_all(&self, DONE, events_log_file);

    for (int i = 0; i < n_processes; i++)
        wait(NULL);

    fclose(events_log_file);
    return 0;
}

void await_orders(Unit *self, FILE *log_file) {
    char log_text[MAX_PAYLOAD_LEN];

    Message *in_msg = malloc(sizeof(Message));
    Message *ack_msg = malloc(sizeof(Message));

    create_msg(ack_msg, ACK, "", 0);

    in_msg->s_header.s_type = STARTED;

    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS;

    while (in_msg->s_header.s_type != STOP) {
        switch (receive_any(self, in_msg)) {
            case 0:
                timeout_ns = TIMEOUT_NS;
                if (in_msg->s_header.s_type == TRANSFER) {
                    TransferOrder *transfer_order = (TransferOrder *) in_msg->s_payload;
                    if (transfer_order->s_src == self->lid) {
                        if (send(self, transfer_order->s_dst, in_msg) < 0) {
                            perror("send");
                        }
                        timestamp_t time = get_physical_time();
                        create_log_text(log_text, log_transfer_out_fmt, time, transfer_order->s_src,
                                transfer_order->s_amount, transfer_order->s_dst);
                        log_msg(log_file, log_text);
                    } else if (transfer_order->s_dst == self->lid) {
                        if (send(self, PARENT_ID, ack_msg) < 0) {
                            perror("send");
                        }
                        timestamp_t time = get_physical_time();
                        create_log_text(log_text, log_transfer_in_fmt, time, transfer_order->s_dst,
                                        transfer_order->s_amount, transfer_order->s_src);
                        log_msg(log_file, log_text);
                    } else {
                        fprintf(stderr, "Unit %d received unexpected message\n", self->lid);
                    }
                }
                Unit_clear_mask(self);
                break;

            case -1:
                if (errno == EAGAIN) {
                    nanosleep(&tw, &tr);
                    timeout_ns -= WAIT_TIME_NS / 2;
                    if (timeout_ns <= 0) {
                        fprintf(stderr, "%d: wait TRANSFER timeout exceeded\n", self->lid);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    perror("receive_any");
                    exit(EXIT_FAILURE);
                }
        }
    }

    free(in_msg);
    free(ack_msg);
}

void transfer(void *parent_data, local_id src, local_id dst,
              balance_t amount) {
    Unit *self = parent_data;
    Message *transfer_message = malloc(sizeof(Message));
    Message *ack_message = malloc(sizeof(Message));

    TransferOrder *transfer_order = malloc(sizeof(TransferOrder));
    transfer_order->s_amount = amount;
    transfer_order->s_dst = dst;
    transfer_order->s_src = src;

    create_msg(transfer_message, TRANSFER, transfer_order, sizeof(TransferOrder));

    send(self, src, transfer_message);

    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS * 1.5;
    while (1) {
        if (receive(self, dst, ack_message) < 0) {
            if (errno != EAGAIN) {
                perror("receive");
                exit(EXIT_FAILURE);
            } else {
                nanosleep(&tw, &tr);
                timeout_ns -= WAIT_TIME_NS;
                if (timeout_ns <= 0) {
                    fprintf(stderr, "wait ACK: timeout exceeded\n");
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            printf("ACK: %d from %d to %d\n", amount, src, dst);
            break;
        }
    }

    free(ack_message);
    free(transfer_message);
}
