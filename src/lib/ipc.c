#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <unistd.h>
#include <errno.h>
#include <asm/errno.h>
#include "ipc.h"
#include "entity.h"
#include <time.h>
#include "lamp_time.h"
#include "utils.h"

int send(void* self, local_id dst, const Message* msg) {
    Unit* me = self;
    if (write(me->pipes[me->lid][dst][1], msg, msg->s_header.s_payload_len + sizeof(MessageHeader)) <= 0) {
        return -1;
    }
//    if (dst == PARENT_ID && msg->s_header.s_type == 0) {
//        printf("%d sending STARTED to %d\n", me->lid, PARENT_ID);
//    }
    return 0;
}


int send_multicast(void* self, const Message* msg) {
    Unit* me = self;
    for (int to = 0; to < me->n_nodes; to++) {
        if (to == me->lid) continue;
        if (send(me, (local_id) to, msg) < 0) {
            return -1;
        }
    }
    return 0;
}


int receive(void* self, local_id from, Message* msg) {
    Unit* me = self;
    int fd = me->pipes[from][me->lid][0];

    if (read(fd, &msg->s_header, sizeof(MessageHeader)) <= 0) {
        return -1;
    } else if (msg->s_header.s_payload_len != 0) {
        // have to wait for the payload if we got the header
        struct timespec tw = {0, WAIT_TIME_NS};
        struct timespec tr;
        unsigned long long timeout_ns = TIMEOUT_NS;
        while (1) {
            if (read(fd, &msg->s_payload, msg->s_header.s_payload_len) <= 0) {
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
            } else break;
        }
    }

//    if (me->lid == PARENT_ID && msg->s_header.s_type == 0) {
//        printf("%d received STARTED from %d\n", me->lid, from);
//    }
    compare_and_inc_time(msg->s_header.s_local_time);
    return 0;
}


int receive_any(void* self, Message* msg) {
    Unit* me = self;
    for (int from = 0; from < me->n_nodes; from++) {
        if (from == me->lid) {
            continue;
        }
        if (receive(me, (local_id) from, msg) < 0) {
            if (errno == EAGAIN) continue;
            else return -1;
        } else {
            me->last_message_lid = from;
            return 0;
        }
    }
    return -1;
}
