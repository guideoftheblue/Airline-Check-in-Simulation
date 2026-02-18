#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef struct Customer Customer;

typedef struct Queue{
    Customer *head;
    Customer *tail;
    int length;
} Queue;

void queue_init(Queue *queue);
void enqueue(Queue *queue, Customer *customer);
Customer* dequeue(Queue *queue);

#endif