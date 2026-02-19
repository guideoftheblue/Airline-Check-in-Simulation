#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include "queue.h"
#include "customer.h"

/* global variables */
pthread_cond_t customers_available;
pthread_mutex_t acs_mutex;

int sim_status = 0; //1 if all customers served, 0 otherwise

Queue business_queue;
Queue econ_queue;

struct timeval start_time; //initial relative machine time, captured in main.


double get_elapsed_time(){
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - start_time.tv_sec) + 
           (now.tv_usec - start_time.tv_usec) / 1e6;
}


void *customer_entry(void *arg){
    Customer *customer = (Customer *)arg;

    /*determine how long its been since start of program*/
    double elapsed_time = get_elapsed_time()

    /*convert arrival_time to seconds*/
    double arrival_time = customer->arrival_time * 0.1;

    /*calculate remaining time before arrival*/
    double sleep_time = arrival_time - elapsed_time;

    /*sleep for remaining time*/
    if (sleep_time > 0){
        usleep((useconds_t)sleep_time * 1e6);
        printf("A customer arrives: customer ID %2d. \n", customer->id);
    } else {
        printf("Customer %2d is late! Better hurry!\n", customer->id);
    }

    pthread_mutex_lock(&acs_mutex);

    queue_entry_time = get_elapsed_time()
    customer->wait_time = queue_entry_time;

    if (customer->class_type == 1){
        enqueue(&business_queue, customer);
        printf("A customer enters a queue: queue ID 1, and length of the queue %2d. \n", business_queue.length);
    } else {
        enqueue(&econ_queue, customer);
        printf("A customer enters a queue: queue ID 0, and length of the queue %2d. \n", econ_queue.length);
    }
    
    pthread_cond_broadcast(&customers_available);

    pthread_mutex_unlock(&acs_mutex);

    return NULL;
}


void *clerk_entry(void *arg){
    int clerkID = *(int *)arg;

    while(1){
        pthread_mutex_lock(&acs_mutex);

        while (business_queue.length == 0 && econ_queue.length == 0 && sim_status == 0){ //while queues empty and customers left to serve
            pthread_cond_wait(&customers_available, &acs_mutex);
        }

        if (business_queue.length == 0 && econ_queue.length == 0 && sim_status == 1){ //all customers served, exit thread
            pthread_mutex_unlock(&acs_mutex);
            break;
        }

        Customer *customer = NULL;
        if (business_queue.length != 0){
            customer = dequeue(&business_queue);
        } else if (econ_queue.length != 0){
            customer = dequeue(&econ_queue);
        }

        pthread_mutex_unlock(&acs_mutex);

        double elapsed_time = get_elapsed_time()

        // print start
        printf("A clerk starts serving a customer: start time %.2f seconds, the customer ID %2d, the clerk ID %1d. \n", elapsed_time, customer->id, clerkID);

        double wait_time = elapsed_time - customer->wait_time; //customer->wait_time holds queue entry time, finding total time spent in queue
        customer->wait_time = wait_time;

        usleep((useconds_t)customer->service_time * 1e5); //serving customer (1/10 of second -> microsecond)

        //print end
        double end_time = get_elapsed_time()
        printf("A clerk finishes serving a customer: end time %.2f seconds, the customer ID %2d, the clerk ID %d. \n", end_time, customer->id, clerkID);
        
    }

    return NULL;  
}


int main(int argc, char *argv[]) {

    if (argc != 2){
        printf("Usage: ACS <input_file>\n");
        exit(1);
    }

    /************* GLOBAL INITIALIZATION *************/

    pthread_cond_init(&customers_available, NULL);
    pthread_mutex_init(&acs_mutex, NULL);
    queue_init(&business_queue);
    queue_init(&econ_queue);
    gettimeofday(&start_time, NULL);

    /************* FILE READING *************/

    char *file = argv[1];
	
    FILE *fp = fopen(file, "r");
    if (!fp){
        printf("File not found, ensure file is in local directory");
        exit(1);
    }

    char line[64];

    if (!fgets(line, sizeof(line), fp)) {
        printf("Failed to read customer count\n");
        exit(1);
    }       

    int total_customers = atoi(line);
    if (total_customers <= 0) {
        printf("Customer count %d invalid, must be greater than 0\n", total_customers);
        exit(1);
    }

    Customer **customer_info = malloc(sizeof(Customer *) * total_customers);
    if (!customer_info){
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < total_customers; i++){
        if (!fgets(line, sizeof(line), fp)) {
            printf("Unexpected end of file at line %d.\n", i + 2);
            break;
        }

        int id, class_type, arrival_time, service_time;

        if (sscanf(line, "%d:%d,%d,%d", &id, &class_type, &arrival_time, &service_time) != 4) //Parsing line
        {
            printf("Line #%d in file formatted incorrectly: %s\n", i+2, line);
            exit(1);
        } 

        /* FILE ERROR HANDLING */

        if (id < 0){
            printf("Customer ID on line %d must be non-negative.\n", i+2);
            exit(1);
        }

        if (class_type != 0 && class_type != 1){
            printf("Class on line %d must be 0 (Economy) or 1 (Business), defaulting to economy.\n", i+2);
            class_type =  0;
        }

        if (arrival_time < 0){
            printf("Arrival time on line %d must be non-negative.\n", i+2);
            exit(1);
        }

        if (service_time <= 0){
            printf("Service time on line %d must be greater than 0.\n", i+2);
            exit(1);
        }

        customer_info[i] = create_customer(id, class_type, arrival_time, service_time); //Adding customer to array of customers
        if(!customer_info[i]){
            printf("Customer creation failed");
            exit(1);
        }
    }

    fclose(fp);

    /************* THREAD CREATION *************/

	/*creating five clerk threads*/
    pthread_t clerks[5];
    int clerk_info[5] = {1,2,3,4,5};

	for(int i = 0; i < 5; i++){
		if (pthread_create(&clerks[i], NULL, clerk_entry, (void *)&clerk_info[i]) != 0){
            perror("clerk pthread_create");
            exit(1);
        } 
	}
	
	/*creating n customer threads*/
    pthread_t *customers = malloc(sizeof(pthread_t) * total_customers);
	for(int i = 0, i < total_customers; i++){ // number of customers
		if (pthread_create(&customers[i], NULL, customer_entry, (void *)customer_info[i]) != 0){
            perror("customer pthread_create");
            exit(1);
        } //customer_info: passing the customer information (e.g., customer ID, arrival time, service time, etc.) to customer thread
	}
    
    /************* THREAD CLEANUP *************/

	/*wait for all customer threads to terminate*/
	for(int i = 0; i < total_customers; i++){
		pthread_join(customers[i], NULL);
	}

    pthread_mutex_lock(&acs_mutex); //mutex so there's no race with clerks for sim_status

    sim_status= 1; //update simulation status to complete, allowing clerks to exit
    pthread_cond_broadcast(&customers_available);

    pthread_mutex_unlock(&acs_mutex);

    for(int i = 0; i < 5; i++){
		pthread_join(clerks[i], NULL);
	}

    pthread_mutex_destroy(&acs_mutex);
    pthread_cond_destroy(&customers_available);
	free(customers);

    /************* WAIT TIME CALCULATION *************/
	
	// calculate the average waiting times

    for(int i = 0; i < total_customers; i++){
		free(customer_info[i]);
	}
    free(customer_info);
	return 0;
}







