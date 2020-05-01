//
// Created by almdudler on 17.04.2020.
//

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <self.h>

void Self_clear_mask(Self *self) {
    for (int i = 0; i < self->n_nodes; i++) {
        self->read_mask[i] = 1;
    }
}

void Self_new(Self *self, int lid, int n_nodes, int ***pipes) {
    self->pipes = pipes;
    self->n_nodes = n_nodes;
    self->lid = lid;
    self->read_mask = malloc(self->n_nodes * sizeof(char));
    Self_clear_mask(self);
}

void Self_destruct(Self *self) {
    free(self->read_mask);
}

void print_pipes(int ***pipes, int n_nodes) {
    for (int from = 1; from < n_nodes; from++) {
        for (int to = 0; to < n_nodes; to++) {
            for (int is_write = 0; is_write < 2; is_write++) {
                if (to == from) continue;
                printf("from: %d, to: %d, write end? %d: %d\n", from, to, is_write, pipes[from][to][is_write]);
            }
        }
    }
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
