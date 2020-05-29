//
// Created by almdudleer on 30.05.2020.
//

#include "queue.h"
#include "pa2345.h"
#include "utils.h"
#include "lamp_time.h"
#include "banking.h"
#include "entity.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "message_processing.h"

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