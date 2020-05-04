#include "string.h"
#include <entity.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "banking.h"
#include "utils.h"
#include "common.h"
#include "ipc_utils.h"
#include <pa2345.h>
#include <fcntl.h>

int started_left = -1, done_left = -1, history_left = -1;

void await_orders(Unit* self, FILE* log_file) {

    Message* in_msg = Message_empty();
    Message* out_msg = Message_new(ACK, "", 0);

    while (in_msg->s_header.s_type != STOP) {
        if (receive_any_or_die(self, in_msg) >= 0) {
            if (in_msg->s_header.s_type == TRANSFER) {
                TransferOrder* transfer_order = (TransferOrder*) in_msg->s_payload;

                local_id dst;
                Message* msg;
                balance_t balance_change;
                timestamp_t time = get_physical_time();
                char log_text[MAX_PAYLOAD_LEN];

                if (transfer_order->s_src == self->lid) {
                    dst = transfer_order->s_dst;
                    msg = in_msg;
                    balance_change = -transfer_order->s_amount;
                    create_log_text(log_text, log_transfer_out_fmt, time, transfer_order->s_src,
                                    transfer_order->s_amount, transfer_order->s_dst);
                } else if (transfer_order->s_dst == self->lid) {
                    dst = PARENT_ID;
                    msg = out_msg;
                    balance_change = transfer_order->s_amount;
                    create_log_text(log_text, log_transfer_in_fmt, time, transfer_order->s_dst,
                                    transfer_order->s_amount, transfer_order->s_src);
                } else {
                    fprintf(stderr, "Unit %d received unexpected message\n", self->lid);
                    exit(EXIT_FAILURE);
                }

//                printf("%d sending %d to %d\n", self->lid, msg->s_header.s_type, dst);
                if (send(self, dst, msg) < 0) {
                    perror("send");
                    exit(EXIT_FAILURE);
                }
                Unit_set_balance(self, self->balance + balance_change);
                log_msg(log_file, log_text);
            } else if (in_msg->s_header.s_type != STOP) {
                fprintf(stderr, "Process %d: bad message type %d, expected TRANSFER(%d)\n",
                        self->lid, in_msg->s_header.s_type, TRANSFER);
            }
        } else {
            perror("await_orders: receive_any_or_die");
            exit(EXIT_FAILURE);
        }
    }
    Unit_set_balance(self, self->balance);

    // Clean up
    Message_free(in_msg);
    Message_free(out_msg);
}

void send_started(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];

    timestamp_t time = get_physical_time();
    create_log_text(log_text, log_started_fmt, time, self->lid, getpid(), getppid(), self->balance);

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

    timestamp_t time = get_physical_time();
    create_log_text(log_text, log_done_fmt, time, self->lid, self->balance);

    Message* done_msg = Message_new(DONE, log_text, strlen(log_text));
//    printf("%d sending DONE: %d\n", self->lid, DONE);
    if (send_multicast(self, done_msg) < 0) {
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }

    log_msg(log_file, log_text);
    Message_free(done_msg);
}

void send_history(Unit* self) {
    size_t payload_len =
            sizeof(BalanceHistory) - ((MAX_T + 1 - self->balance_history->s_history_len)*sizeof(BalanceState));
    Message* hist_msg = Message_new(BALANCE_HISTORY, self->balance_history, payload_len);

    if (send(self, PARENT_ID, hist_msg) < 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }

//    printf("%d sending BALANCE_HISTORY", self->lid);
    Message_free(hist_msg);
}

void* receive_all(Unit* self, MessageType type) {
    Message in_msg;
    while (1) {
        if (receive_any_or_die(self, &in_msg) == 0) {
            switch (in_msg.s_header.s_type) {
                case STARTED:
                    started_left--;
                    if (type == STARTED && started_left == 0) {
                        started_left = -1;
                        return NULL;
                    }
                    break;
                case DONE:
                    done_left--;
                    if (type == DONE && done_left == 0) {
                        done_left = -1;
                        return NULL;
                    }
                    break;
                case BALANCE_HISTORY:
                    history_left--;
                    if (type == BALANCE_HISTORY && done_left == 0) {
                        history_left = -1;
                        return NULL;
                    }
                    break;
                default:
                    fprintf(stderr,"Process %d: unexpected message type %d\n",
                            self->lid, in_msg.s_header.s_type);
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
    history_left = -1;

    send_started(self, log_file);
    receive_all(self, STARTED);
    create_log_text(log_text, log_received_all_started_fmt, get_physical_time(), self->lid);
    log_msg(log_file, log_text);

    await_orders(self, log_file);

    send_done(self, log_file);
    receive_all(self, DONE);
    create_log_text(log_text, log_received_all_done_fmt, get_physical_time(), self->lid);
    log_msg(log_file, log_text);

    send_history(self);

    exit(EXIT_SUCCESS);
}

void parent_main(Unit* self, FILE* log_file) {
    char log_text[MAX_PAYLOAD_LEN];

    started_left = done_left = history_left = self->n_nodes - 1;

    // Create messages
    Message* stop_msg = Message_new(STOP, NULL, 0);
    Message* ack_msg = Message_new(ACK, NULL, 0);

    // Run main job
    receive_all(self, STARTED);
    create_log_text(log_text, log_received_all_started_fmt, get_physical_time(), self->lid);
    log_msg(log_file, log_text);

    bank_robbery(self, self->n_nodes - 1);
//    printf("%d sending STOP: %d\n", self->lid, STOP);
    send_multicast(self, stop_msg);
    receive_all(self, DONE);
//    receive_all(self, BALANCE_HISTORY);

    // Clean up
    Message_free(stop_msg);
    Message_free(ack_msg);
}

int main(int argc, char** argv) {

    // Open log files
    FILE* events_log_file = fopen(events_log, "w");
    FILE* pipes_log_file = fopen(pipes_log, "w");

    // Define variables to be set from opts
    local_id n_processes;
    balance_t** balances_pointer;

    // Read opts
    int opt;
    char* endptr;
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
            unsigned int flags;
            int fd = pipes[from][to][0];
            if ((flags = fcntl(fd, F_GETFL)) < 0) return -1;
            if (fcntl(fd, F_SETFL, flags | ((unsigned int) O_NONBLOCK)) < 0) return -1;
            fprintf(pipes_log_file, "pipe from %d to %d opened; read end: %d, write end: %d\n",
                    from, to, pipes[from][to][0], pipes[from][to][1]);
        }
    }
    fclose(pipes_log_file);

    // Fork child processes
    for (local_id lid = 1; lid < (local_id) n_processes; lid++) {
        Unit* self = Unit_new(lid, n_processes, pipes, (*balances_pointer)[lid]);
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
    Unit* parent_unit = Unit_new(PARENT_ID, n_processes, pipes, 5);
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
