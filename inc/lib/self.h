//
// Created by almdudler on 17.04.2020.
//

#ifndef PA1_SELF_H
#define PA1_SELF_H

#include "ipc.h"
#include "banking.h"

typedef struct Unit Unit;

typedef enum UNIT_TYPE {
    K,
    C
} UNIT_TYPE;

struct Unit {
    int*** pipes;
    int n_nodes;
    local_id lid;
    char* read_mask;
    balance_t balance;

    BalanceHistory* balance_history;
    UNIT_TYPE unit_type;
};

typedef struct Client {
    int*** pipes;
    local_id lid;
} Client;

void Unit_clear_mask(Unit* self);

void Unit_new(Unit *self, int lid, int n_nodes, int ***pipes, balance_t balance);

// 3d array operations
void print_pipes(int*** pipes, int n_nodes);

void free_pipes(int*** data, size_t xlen, size_t ylen);

int*** alloc_pipes(size_t xlen, size_t ylen, size_t zlen);

#endif //PA1_SELF_H
