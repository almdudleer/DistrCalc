//
// Created by almdudleer on 28.05.2020.
//
#include "queue.h"
#include <stddef.h>
#include <malloc.h>

Queue* Queue_new() {
    Queue* que = malloc(sizeof(Queue));
    if (!que) {
        perror("Queue_new: malloc");
        return NULL;
    }
    que->head = malloc(sizeof(node));
    que->head->next = NULL;
    que->head->value = NULL;
    return que;
}

void Queue_free(Queue* que) {
    free(que);
}

CsRequest* dequeue(Queue* que) {
    if (que->head->next == NULL) {
        fprintf(stderr, "queue empty\n");
        return NULL;
    }
    node* last_first = que->head->next;
    CsRequest* res = last_first->value;
    que->head->next = last_first->next;
    free(last_first);
    return res;
}

int enqueue(Queue* que, CsRequest* element) {
    node* cur = que->head->next;
    node* prev = que->head;
    while (cur != NULL) {
        if ((cur->value->time > element->time) ||
            ((cur->value->time == element->time) && (cur->value->lid > element->lid))) break;
        prev = cur;
        cur = cur->next;
    }
    node* new_node = malloc(sizeof(node));
    if (!new_node) {
        perror("enqueue: malloc");
        return -1;
    }
    new_node->value = element;
    new_node->next = cur;
    prev->next = new_node;
    return 0;
}

CsRequest* peek(Queue* que) {
    if (que->head->next != NULL)
        return que->head->next->value;
    else
        return NULL;
}

CsRequest* cut(Queue* que, local_id lid) {
    node* prev = que->head;
    node* cur = que->head->next;
    while (cur != NULL) {
        if (cur->value->lid == lid) {
            prev->next = cur->next;
            return cur->value;
        }
        prev = cur;
        cur = cur->next;
    }
    return NULL;
}

void queue_print(Queue* que) {
    node* cur = que->head->next;
    while (cur != NULL) {
        printf("(%d,%d); ", cur->value->lid, cur->value->time);
        cur = cur->next;
    }
    printf("\n");
}
