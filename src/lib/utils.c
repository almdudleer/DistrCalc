#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include "ipc.h"
#include "utils.h"
#include "self.h"
#include "pa1.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>


int receive_all(Self* self, MessageType type, FILE* events_log_file) {
    char log_text[MAX_PAYLOAD_LEN];
    const char* log_fmt;
    switch (type) {
        case DONE:
            log_fmt = log_received_all_done_fmt;
            break;
        case STARTED:
            log_fmt = log_received_all_started_fmt;
            break;
        default:
            log_fmt = "Event of type %d";

    }
    create_log_text(log_text, log_fmt, self->lid);

    Message* incoming_msg = malloc(sizeof(Message));


    int n_received = self->n_nodes - 2;
//    int timeout = 1000;
    while (n_received != 0) {
//        if (timeout <= 0) return -1;
        if (receive_any(self, incoming_msg) < 0) {
            if (errno == EAGAIN) {
                sleep(1);
//                timeout--;
//                printf("sleep %d\n", self->lid);
                continue;
            } else {
                perror("receive_any");
                return -1;
            }
        } else {
            n_received--;
            if (incoming_msg->s_header.s_type != type) {
//                printf("Unexpected message type %d\n", incoming_msg->s_header.s_type);
            }
        }
    }

    log_msg(events_log_file, log_text);

    Self_clear_mask(self);
    free(incoming_msg);

    return 0;
}

int write_nonblock(int fd, char* msg, unsigned long n_bytes) {
//    int timeout = 1000;
    while (1) {
//        if (timeout <= 0) return -1;
        if (write(fd, msg, n_bytes) < 0) {
            if (errno == EAGAIN) {
//                timeout--;
                sleep(1);
                continue;
            } else {
                perror("write_nonblock");
                return -1;
            }
        }
        return 0;
    }
}

void create_msg(Message* msg, MessageType type, char* payload) {
    timestamp_t current_time = (timestamp_t) time(NULL);

    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_local_time = current_time;
    header.s_payload_len = (uint16_t) strlen(payload);
    header.s_type = type;
    msg->s_header = header;
    sprintf(msg->s_payload, "%s", payload);

}

void create_log_text(char* msg_text, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsprintf(msg_text, format, args);
    va_end(args);
}

void log_msg(FILE* log_file, char* msg) {

//    log_file = fopen(events_log, "a");

    write_nonblock(STDOUT_FILENO, msg, strlen(msg));

//    write_nonblock(log_file->_fileno, msg, strlen(msg));

    fflush(log_file);
    if (fprintf(log_file, "%s", msg) < 0) {
        perror("fprintf");
    }
    fflush(log_file);

}
