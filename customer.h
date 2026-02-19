#ifndef _CUSTOMER_H_
#define _CUSTOMER_H_

typedef struct Customer{
    int id;
    int class_type;
    int arrival_time;
    int service_time;
    double queue_entry_time;
    double wait_time;
    struct Customer *next; //next customer in respective queue.
}Customer;

Customer *create_customer(int id, int class_type, int arrival_time, int service_time);
#endif