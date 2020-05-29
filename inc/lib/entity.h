//
// Created by almdudler on 17.04.2020.
//

#ifndef PA1_SELF_H
#define PA1_SELF_H

#include <stdio.h>
#include "ipc.h"
#include "queue.h"

typedef struct Unit Unit;
typedef struct UnitLimits UnitLimits;

struct Unit {
    int*** pipes;
    Queue* que;
    int n_nodes;
    local_id lid;
    int done;
    int is_parent;

    local_id last_msg_from;
    CsRequest* last_request;
    int* replies_mask;
    int* deferred_replies;
    UnitLimits* limits;

    FILE* log_file;
};

struct UnitLimits {
    int started_left;
    int done_left;
    int iters_left;
    int iters_total;
    int replies_left;
    int replies_total;
};

Unit* Unit_new(int lid, int n_nodes, int*** pipes, FILE* log_file);

void Unit_free(Unit* self);

Message* Message_new(MessageType type, void* payload, size_t payload_len);

Message* Message_empty();

void Message_free(Message* self);

// 3d array operations
void print_pipes(int*** pipes, int n_nodes);

void free_pipes(int*** data, size_t xlen, size_t ylen);

int*** alloc_pipes(size_t xlen, size_t ylen, size_t zlen);

#endif //PA1_SELF_H
