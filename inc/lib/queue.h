//
// Created by almdudleer on 28.05.2020.
//

#ifndef PAS_QUEUE_H
#define PAS_QUEUE_H
#include <stddef.h>
#include <stdio.h>
#include "ipc.h"

typedef struct Queue Queue;
typedef struct node node;
typedef struct CsRequest CsRequest;

struct CsRequest {
    local_id lid;
    timestamp_t time;
};

struct node {
    CsRequest* value;
    node* next;
};

struct Queue {
    node* head;
};

Queue* Queue_new();

void Queue_free(Queue* que);

CsRequest* dequeue(Queue* que);

int enqueue(Queue* que, CsRequest* element);

CsRequest* peek(Queue* que);

void queue_print(Queue* que);

#endif //PAS_QUEUE_H
