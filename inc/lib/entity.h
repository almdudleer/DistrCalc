//
// Created by almdudler on 17.04.2020.
//

#ifndef PA1_SELF_H
#define PA1_SELF_H

#include <stdio.h>
#include "ipc.h"
#include "banking.h"

typedef struct Unit Unit;

struct Unit {
    int*** pipes;
    int n_nodes;
    local_id lid;
    balance_t balance;

    BalanceHistory* balance_history;
};

Message* Message_new(MessageType type, void* payload, size_t payload_len);

Message* Message_empty();

void Message_free(Message* self);

Unit* Unit_new(int lid, int n_nodes, int*** pipes, balance_t balance);

void Unit_free(Unit* self);

void Unit_set_balance(Unit *self, balance_t new_balance);

void BalanceHistory_new(BalanceHistory *self, local_id lid, balance_t init_balance);

void BalanceHistory_free(BalanceHistory* history);

void BalanceState_new(BalanceState *self, balance_t balance, timestamp_t time);

// 3d array operations
void print_pipes(int*** pipes, int n_nodes);

void free_pipes(int*** data, size_t xlen, size_t ylen);

int*** alloc_pipes(size_t xlen, size_t ylen, size_t zlen);

#endif //PA1_SELF_H
