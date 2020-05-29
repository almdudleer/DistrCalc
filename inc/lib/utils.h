//
// Created by almdudler on 15.04.2020.
//

#ifndef PA1_UTILS_H
#define PA1_UTILS_H

#include "entity.h"
#include <stdio.h>

#define WAIT_TIME_NS (200000000 / 2)

// write syscall utility
int write_nonblock(int fd, char* msg, unsigned long n_bytes);

// log utils
void create_log_text(char* msg_text, const char* format, ...);

void log_msg(FILE* log_file, char* msg);

void close_bad_pipes(Unit* self, int n_processes, int** const* pipes);


#endif //PA1_UTILS_H
