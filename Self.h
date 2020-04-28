//
// Created by almdudler on 17.04.2020.
//

#ifndef PA1_SELF_H
#define PA1_SELF_H

#include "ipc.h"

typedef struct {
    int*** pipes;
    int n_nodes;
    local_id lid;
    char* read_mask;
} Self;

void Self_clear_mask(Self* self);

void Self_new(Self* self, int lid, int n_nodes, int*** pipes);

// 3d array operations
void print_pipes(int*** pipes, int n_nodes);

void free_pipes(int*** data, size_t xlen, size_t ylen);

int*** alloc_pipes(size_t xlen, size_t ylen, size_t zlen);

#endif //PA1_SELF_H
