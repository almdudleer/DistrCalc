//
// Created by almdudleer on 28.05.2020.
//

#ifndef PAS_QUEUE_H
#define PAS_QUEUE_H
#include <stddef.h>
#include <stdio.h>
#include "ipc.h"

typedef struct queue queue;
typedef struct node node;
typedef struct cs_request cs_request;

struct cs_request {
    local_id lid;
    timestamp_t time;
};

struct node {
    cs_request* value;
    node* next;
};

struct queue {
    node* head;
};

queue* queue_new();

void queue_free(queue* que);

cs_request* dequeue(queue* que);

int enqueue(queue* que, cs_request* element);

cs_request* peek(queue* que);

void queue_print(queue* que);

#endif //PAS_QUEUE_H
