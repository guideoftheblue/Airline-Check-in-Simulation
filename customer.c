#include <stdlib.h>
#include "customer.h"

Customer *create_customer(int id, int class_type, int arrival_time, int service_time){
    Customer *customer = malloc(sizeof(Customer));
    if (!customer) return NULL;

    customer->id = id;
    customer->class_type = class_type;
    customer->arrival_time = arrival_time;
    customer->service_time = service_time;
    customer->wait_time = 0;
    customer->next = NULL;

    return customer
}