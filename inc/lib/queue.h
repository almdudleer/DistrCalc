//
// Created by almdudleer on 28.05.2020.
//

#ifndef PAS_QUEUE_H
#define PAS_QUEUE_H
#include <stddef.h>

typedef struct queue {
    void** start;
    void** end;
    size_t size;
} queue;

queue* queue_new(size_t size);

void queue_free(queue* que);

void* dequeue(queue* que);

int enqueue(queue* que, void* element);

#endif //PAS_QUEUE_H
