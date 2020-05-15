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
                if (TIMEOUTS) {
                    timeout_ns -= WAIT_TIME_NS;
                    if (timeout_ns <= 0) {
                        errno = EBUSY;
                        return -1;
                    }
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
                if (TIMEOUTS) {
                    timeout_ns -= WAIT_TIME_NS;
                    if (timeout_ns <= 0) {
                        errno = EBUSY;
                        return -1;
                    }
                }
            } else {
                return -1;
            }
        } else return 0;
    }
}

