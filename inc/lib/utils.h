//
// Created by almdudler on 15.04.2020.
//

#ifndef PA1_UTILS_H
#define PA1_UTILS_H

#include "self.h"
#include <stdio.h>

#define EVENTS_FILE_NAME "events.log"

// write syscall utility
int write_nonblock(int fd, char* msg, unsigned long n_bytes);

// extra receive operation
int receive_all(Unit* self, MessageType type, FILE* events_log_file);

// message constructor
void create_msg(Message* msg, MessageType type, char* payload);

// log utils
void create_log_text(char* msg_text, const char* format, ...);

void log_msg(FILE* log_file, char* msg);

void close_bad_pipes(Unit* self, int n_processes, int** const* pipes);


#endif //PA1_UTILS_H
