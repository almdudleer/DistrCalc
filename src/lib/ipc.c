#include <unistd.h>
#include <errno.h>
#include <asm/errno.h>
#include "ipc.h"
#include "self.h"
#include <fcntl.h>


int send(void* self, local_id dst, const Message* msg) {
    Unit* me = self;
    if (write(me->pipes[me->lid][dst][1], msg, msg->s_header.s_payload_len + sizeof(MessageHeader)) <= 0) {
        return -1;
    }

//    char console_text[255];
//    sprintf(console_text, "%d: %d send to %d\n", msg->s_header.s_type, me->lid, dst);
//    write_nonblock(STDOUT_FILENO, console_text, strlen(console_text));

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

    int flags;
    int fd = me->pipes[from][me->lid][0];
    if ((flags = fcntl(fd, F_GETFL)) < 0) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;

    if (read(fd, &msg->s_header, sizeof(MessageHeader)) <= 0) {
        return -1;
    }

    if (read(fd, &msg->s_payload, msg->s_header.s_payload_len) <= 0) {
        return -1;
    }

    return 0;
}


int receive_any(void* self, Message* msg) {
    Unit* me = self;
    for (int from = 0; from < me->n_nodes; from++) {
        if ((from == me->lid) || (me->read_mask[from] == 0)) {
            continue;
        }

        if (receive(me, (local_id) from, msg) < 0) {
            if (errno == EAGAIN) continue;
            else return -1;
        } else {
            me->read_mask[from] = 0;
            return 0;
        }

    }
    return -1;
}
