//
// Created by almdudler on 17.04.2020.
//

#ifndef PA1_SELF_H
#define PA1_SELF_H

#include <stdio.h>
#include "ipc.h"
#include "queue.h"

typedef struct Unit Unit;

struct Unit {
    int*** pipes;
    queue* que;
    int n_nodes;
    local_id lid;
};

Message* Message_new(MessageType type, void* payload, size_t payload_len);

Message* Message_empty();

void Message_free(Message* self);

Unit* Unit_new(int lid, int n_nodes, int*** pipes);

void Unit_free(Unit* self);

// 3d array operations
void print_pipes(int*** pipes, int n_nodes);

void free_pipes(int*** data, size_t xlen, size_t ylen);

int*** alloc_pipes(size_t xlen, size_t ylen, size_t zlen);

#endif //PA1_SELF_H
