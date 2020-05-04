//
// Created by almdudler on 17.04.2020.
//

#include "ipc.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <entity.h>

// ---------------------------------------------------- MESSAGE ----------------------------------------------------- //

Message* Message_new(MessageType type, void* payload, size_t payload_len) {
    Message* self = malloc(sizeof(Message));

    // Create header
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_local_time = get_physical_time();
    header.s_payload_len = payload_len;
    header.s_type = type;
    self->s_header = header;

    // Copy payload
    if (payload_len != 0) memcpy(self->s_payload, payload, payload_len);

    return self;
}

Message* Message_empty() {
    Message* self = malloc(sizeof(Message));
    return self;
}


void Message_free(Message* self) {
    free(self);
}

// ----------------------------------------------------- UNIT ------------------------------------------------------- //

Unit* Unit_new(int lid, int n_nodes, int*** pipes, balance_t balance) {
    Unit* self = malloc(sizeof(Unit));
    self->pipes = pipes;
    self->n_nodes = n_nodes;
    self->lid = lid;
    malloc(self->n_nodes*sizeof(char));
    self->balance = balance;
    self->balance_history = malloc(sizeof(BalanceHistory));
    BalanceHistory_new(self->balance_history, self->lid, self->balance);
    return self;
}

void Unit_free (Unit *self) {
    BalanceHistory_free(self->balance_history);
    free(self);
}

void Unit_set_balance(Unit *self, balance_t new_balance) {
    int hist_len = self->balance_history->s_history_len;
    timestamp_t time = get_physical_time();
//    printf("%d: %d hist_len before: %d, last_balance: %d at %d\n", time, self->lid, hist_len,
//            self->balance_history->s_history[hist_len-1].s_balance, self->balance_history->s_history[hist_len-1].s_time);
    for (; hist_len - 1 < time; hist_len++) {
        BalanceState bs;
        BalanceState_new(&bs, self->balance, hist_len - 1);
        self->balance_history->s_history[hist_len - 1] = bs;
//        printf("%d: %d hist_len in: %d, last_balance: %d at %d\n", time, self->lid, hist_len,
//               self->balance_history->s_history[hist_len-1].s_balance, self->balance_history->s_history[hist_len-1].s_time);
    }
    BalanceState bs;
    BalanceState_new(&bs, new_balance, time);
    self->balance_history->s_history[time] = bs;
    self->balance = new_balance;
    self->balance_history->s_history_len = hist_len;
//    printf("%d: %d hist_len after: %d, last_balance: %d at %d\n", time, self->lid, hist_len,
//           self->balance_history->s_history[hist_len-1].s_balance, self->balance_history->s_history[hist_len-1].s_time);
}

// ---------------------------------------------- BALANCE HISTORY --------------------------------------------------- //

void BalanceHistory_new(BalanceHistory *self, local_id lid, balance_t init_balance) {
    self->s_id = lid;
    self->s_history_len = 1;
    BalanceState balance_state;
    BalanceState_new(&balance_state, init_balance, get_physical_time());
    // TODO: check for errors (memcpy?)
    self->s_history[0] = balance_state;
}

void BalanceHistory_free(BalanceHistory* history) {
    free(history);
}

// ----------------------------------------------- BALANCE STATE ---------------------------------------------------- //

void BalanceState_new(BalanceState *self, balance_t balance, timestamp_t time) {
    self->s_balance = balance;
    self->s_time = time;
    self->s_balance_pending_in = 0;
}

int ***alloc_pipes(size_t xlen, size_t ylen, size_t zlen) {
    int ***p;
    size_t i, j;

    if ((p = malloc(xlen * sizeof *p)) == NULL) {
        perror("malloc 1");
        return NULL;
    }

    for (i = 0; i < xlen; ++i)
        p[i] = NULL;

    for (i = 0; i < xlen; ++i)
        if ((p[i] = malloc(ylen * sizeof *p[i])) == NULL) {
            perror("malloc 2");
            free_pipes(p, xlen, ylen);
            return NULL;
        }

    for (i = 0; i < xlen; ++i)
        for (j = 0; j < ylen; ++j)
            p[i][j] = NULL;

    for (i = 0; i < xlen; ++i)
        for (j = 0; j < ylen; ++j)
            if ((p[i][j] = malloc(zlen * sizeof *p[i][j])) == NULL) {
                perror("malloc 3");
                free_pipes(p, xlen, ylen);
                return NULL;
            }

    return p;
}

void free_pipes(int ***data, size_t xlen, size_t ylen) {
    size_t i, j;

    for (i = 0; i < xlen; ++i) {
        if (data[i] != NULL) {
            for (j = 0; j < ylen; ++j)
                free(data[i][j]);
            free(data[i]);
        }
    }
    free(data);
}
