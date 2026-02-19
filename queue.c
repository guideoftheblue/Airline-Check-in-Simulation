#include <stdlib.h>
#include "queue.h"
#include "customer.h"

void queue_init(Queue *queue){
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;

}

void enqueue(Queue *queue, Customer *customer){
    customer->next = NULL;

    if (queue->head == NULL) {
        queue->head = customer;
        queue->tail = customer;
    } else {
        queue->tail->next = customer;
        queue->tail = customer;
    }
    
    queue->length++;
}

Customer* dequeue(Queue *queue){
    if (queue->head == NULL){
        return NULL;
    }

    Customer *removed = queue->head;
    queue->head = removed->next;

    if (queue->head == NULL){
        queue->tail = NULL;
    }

    queue->length--;
    return removed;
}
