//
// Created by almdudleer on 28.05.2020.
//
#include <queue.h>
#include <stddef.h>
#include <malloc.h>

queue* queue_new(size_t size) {

    queue* que = malloc(sizeof(queue));
    if (!que) {
        perror("queue_new: malloc");
        return NULL;
    }

    que->start = calloc(size, sizeof(void*));
    if (!que->start) {
        perror("queue_new: calloc");
        return NULL;
    }

    que->size = size;
    que->end = que->start;
    return que;
}

void queue_free(queue* que) {
    free(que->start);
    free(que);
}

void* dequeue(queue* que) {
    if (que->start == que->end) {
        return NULL;
    }
    void* res = *(que->end);
    que->end--;
    return res;
}

int enqueue(queue* que, void* element) {
    if (que->end - que->start == que->size) {
        return -1;
    }
    que->end++;
    *que->end = element;
    return 0;
}
