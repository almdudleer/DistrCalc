//
// Created by almdudler on 15.04.2020.
//

#ifndef PA1_UTILS_H
#define PA1_UTILS_H

#include "self.h"
#include <stdio.h>

#define WAIT_TIME_NS 200000000
#define TIMEOUT_NS 2000000000

int receive_or_die(Unit* self, local_id from, Message* msg);

void on_receive_one(Unit *self, MessageType type, Message *msg, FILE *log_file);

void on_receive_all(Unit *self, MessageType type, FILE *log_file);

// write syscall utility
int write_nonblock(int fd, char* msg, unsigned long n_bytes);

// extra receive operation
int receive_all(Unit* self, MessageType type, FILE* events_log_file);

// message constructor
void create_msg(Message *msg, MessageType type, void *payload, size_t payload_len);

// log utils
void create_log_text(char* msg_text, const char* format, ...);

void log_msg(FILE* log_file, char* msg);

void close_bad_pipes(Unit* self, int n_processes, int** const* pipes);


#endif //PA1_UTILS_H
