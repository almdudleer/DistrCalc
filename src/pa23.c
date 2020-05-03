#include <self.h>
#include <stdlib.h>
#include <pa2345.h>
#include <unistd.h>
#include <sys/wait.h>
#include "banking.h"
#include "utils.h"

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
    // student, please implement me
}

void child_main(Unit* self, FILE* events_log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    Message* msg = malloc(sizeof(Message));

    // send started
    create_log_text(log_text, log_started_fmt, self->lid, getpid(), getppid());
    create_msg(msg, STARTED, log_text);

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

    switch (self->unit_type) {
        case K:
            child_k(self);
            break;
        case C:
            child_c(self);
            break;
        default:
            sleep(1);
    }

    // send done
    create_log_text(log_text, log_done_fmt, self->lid);
    create_msg(msg, DONE, log_text);

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

int main(int argc, char** argv) {

    int opt, n_processes = 0;
    char* endptr;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        if (opt == 'p') {
            n_processes = (int) strtol(optarg, &endptr, 10);
        } else {
            abort();
        }
    }
    n_processes += 1;
    if (n_processes < 2) exit(EXIT_FAILURE);

    int*** pipes = alloc_pipes((size_t) n_processes, (size_t) n_processes, 2);

    FILE* events_log_file = fopen(events_log, "w");
    FILE* pipes_log_file = fopen(pipes_log, "w");

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
        Unit_new(&self, lid, n_processes, pipes);

        switch (fork()) {
            case -1: {
                perror("fork");
                exit(1);
            }
            case 0: {
                close_bad_pipes(&self, n_processes, pipes);
                child_main(&self, events_log_file);
            }
            default: { }
        }
    }
    Unit self;
    Unit_new(&self, PARENT_ID, n_processes, pipes);
    close_bad_pipes(&self, n_processes, pipes);

    receive_all(&self, STARTED, events_log_file);
    receive_all(&self, DONE, events_log_file);

    for (int i = 0; i < n_processes; i++)
        wait(NULL);

    fclose(events_log_file);
    return 0;
}
