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
    

    pthread_mutex_lock(&acs_mutex); 

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

void *clerk_entry(void *arg){
    int clerkID = *(int *)arg;

    while(1){

        
    }
}

void *customer_entry(void *arg){
    Customer *customer = (Customer *)arg;

    struct timeval now_time;
    gettimeofday(&now_time, NULL);

    /*determine how long its been since start of program*/
    double elapsed_time = (now_time.tv_sec - start_time.tv_sec) +
                          (now_time.tv_usec - start_time.tv_usec)/1e6; //seconds + (microseconds -> seconds)

    /*convert arrrival_time to seconds*/
    double arrival_time = customer->arrival_time * 0.1;

    /*calculate remaining time before arrival*/
    double remaining_time = arrival_time - elapsed_time;

    /*sleep for remaining time*/
    if (remaining_time > 0){
        usleep((useconds_t)remaining_time * 1e6);
        printf("A customer arrives: customer ID %2d. \n", customer->id);
    } else {
        printf("Customer %2d is late! Better hurry!\n", customer->id);
    }

    pthread_mutex_lock(&acs_mutex);

    double queue_entry_time = elapsed_time + (remaining_time > 0? remaining_time : 0); //if customer is late, just uses elapsed time
    customer->wait_time = queue_entry_time;

    if (customer->class_type == 1){
        enqueue(&business_queue, customer);
    } else {
        enqueue(&econ_queue, customer);
    }
    
    pthread_cond_broadcast(&customers_available);

    pthread_mutex_unlock(&acs_mutex);

    return NULL;
}




