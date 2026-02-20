#include <stdlib.h>
#include "customer.h"

/**
 * create_customerr: Creates instance of Customer structure with given attributes
 * 
 * @id: Number that identifies the customer
 * 
 * @class_type: Number identifying business class of the customer. 0 for economy, 1 for business.
 * 
 * @arrival_time: Number representing the designated arrival time of the customer in tenths of seconds.
 * 
 * @service_time: Number representing the service duration of the customer in tenths of seconds.
 * 
 * @return: Customer instance with given attributes
**/ 
Customer *create_customer(int id, int class_type, int arrival_time, int service_time){
    Customer *customer = malloc(sizeof(Customer));
    if (!customer) return NULL;

    customer->id = id;
    customer->class_type = class_type;
    customer->arrival_time = arrival_time;
    customer->service_time = service_time;
    customer->queue_entry_time = 0;
    customer->wait_time = 0;
    customer->next = NULL;

    return customer;
}