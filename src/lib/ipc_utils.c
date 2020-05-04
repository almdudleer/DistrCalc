//
// Created by almdudleer on 04.05.2020.
//
#include "ipc_utils.h"
#include "pa2345.h"
#include <time.h>
#include <errno.h>


int receive_or_die(Unit* self, local_id from, Message* msg) {
    // set timeout
    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS;

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

int receive_any_or_die(Unit* self, Message* msg) {
    // set timeout
    struct timespec tw = {0, WAIT_TIME_NS};
    struct timespec tr;
    unsigned long long timeout_ns = TIMEOUT_NS;

    // wait for data
    while (1) {
        if (receive_any(self, msg) < 0) {
            if (errno == EAGAIN) {
                nanosleep(&tw, &tr);
                timeout_ns -= WAIT_TIME_NS;
                if (timeout_ns <= 0) {
                    errno = EBUSY;
                    return -1;
                }
            } else {
                return -1;
            }
        } else return 0;
    }
}

void on_receive_one(Unit* self, MessageType type, Message* msg) {
//    if (msg->s_header.s_type != type) {
//         printf("Unexpected message type %d\n", msg->s_header.s_type);
//    }
    if (msg->s_header.s_type != type) {
        printf("%d: bad message type %d expected %d\n", self->lid, msg->s_header.s_type, type);
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

