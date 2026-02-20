#include <stdlib.h>
#include "queue.h"
#include "customer.h"

/**
 * queue_init: initializes instance of queue structure with default values
 * 
 * @queue: queue to be initalized
 * 
 * @return None
**/ 
void queue_init(Queue *queue){
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
}

/**
 * enqueue: Adds a given customer to the end of a given queue (FIFO).
 * 
 * @queue: Queue to be initalized.
 * 
 * @customer: Customer instance to be added to queue.
 * 
 * @return None
**/ 
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

/**
 * dequeue: Removes the front customer from a given queue (FIFO).
 * 
 * @queue: Queue to be extract customer from.
 * 
 * @return Front of queue if non-empty, NULL otherwise.
**/ 
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
