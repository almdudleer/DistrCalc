//
// Created by almdudler on 17.04.2020.
//

#include "ipc.h"
#include "entity.h"
#include "banking.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

// ---------------------------------------------------- MESSAGE ----------------------------------------------------- //

Message* Message_new(MessageType type, void* payload, size_t payload_len) {
    Message* self = malloc(sizeof(Message));

    // Create header
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_local_time = get_lamport_time();
    header.s_payload_len = (uint16_t) payload_len;
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

Unit* Unit_new(int lid, int n_nodes, int*** pipes) {
    Unit* self = malloc(sizeof(Unit));
    self->pipes = pipes;
    self->que = queue_new();
    self->n_nodes = n_nodes;
    self->lid = lid;
    return self;
}

void Unit_free (Unit *self) {
    queue_free(self->que);
    free(self);
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
