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

void print_event(Customer *customer, int event, int clerkID){
    int cust_class = customer->class_type;
    int custID = customer->id;
    double now = get_elapsed_time();

    

    switch (event) {
    case 1: //arrival
        printf("%-8.2f %-7d %-12s %-8s %-6s %-5s\n", now, custID, "Arrival", "-", "-", "-");
        break;
    case 2: //queue_entry
        printf("%-8.2f %-7d %-12s %-8s %-6d %-5d\n", customer->queue_entry_time, custID, "QueueEntry", "-", cust_class, 
        cust_class == 0? econ_queue.length : business_queue.length);
        break;
    case 3: //service start
        printf("%-8.2f %-7d %-12s %-8d %-6d %-5d\n", now, custID, "Start", clerkID, cust_class, 
        cust_class == 0? econ_queue.length : business_queue.length);
        break;
    case 4: //service finish
        printf("%-8.2f %-7d %-12s %-8d %-6d %-5d\n", now, custID, "Finish", clerkID, cust_class, 
        cust_class == 0? econ_queue.length : business_queue.length);
        break;
    }

}


void *customer_entry(void *arg){
    Customer *customer = (Customer *)arg;

    /*determine how long its been since start of program*/
    double elapsed_time = get_elapsed_time();

    /*convert arrival_time to seconds*/
    double arrival_time = customer->arrival_time * 0.1;

    /*calculate remaining time before arrival*/
    double sleep_time = arrival_time - elapsed_time;

    /*sleep for remaining time*/
    usleep((useconds_t)(sleep_time * 1e6));
    print_event(customer, 1, -1);

    pthread_mutex_lock(&acs_mutex);

    double queue_entry_time = get_elapsed_time();
    customer->queue_entry_time = queue_entry_time;

    if (customer->class_type == 1){
        enqueue(&business_queue, customer);
        print_event(customer, 2, -1);
        
    } else {
        enqueue(&econ_queue, customer);
        print_event(customer, 2, -1);
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

        //print start of service
        print_event(customer, 3, clerkID);

        pthread_mutex_unlock(&acs_mutex);

        double start_time = get_elapsed_time();

        double wait_time = start_time - customer->queue_entry_time; //finding total time spent in queue
        customer->wait_time = wait_time;

        usleep((useconds_t)(customer->service_time * 1e5)); //serving customer (1/10 of second -> microsecond)

        //print end of service, locking to make sure queue, lengths are accurate
        pthread_mutex_lock(&acs_mutex);
        print_event(customer, 4, clerkID);
        pthread_mutex_unlock(&acs_mutex);
    }

    return NULL;  
}

void print_wait_times(Customer **customer_info, int total_customers){
    double total_wait_time = 0;
    double business_wait_time = 0;
    int total_business = 0;
    double econ_wait_time = 0;
    int total_econ = 0;


	for(int i = 0; i < total_customers; i++){
        Customer *customer = customer_info[i];

        total_wait_time += customer->wait_time;

        if (customer->class_type == 0){ //economy
            total_econ++;
            econ_wait_time += customer->wait_time;
        } else if (customer->class_type == 1) {
            total_business++;
            business_wait_time += customer->wait_time;
        }
    }
    printf("\n----------------------\n")
    printf("Average Waiting Times\n")
    printf("----------------------\n")
    printf("All customers: %.2f seconds. \n", total_wait_time/total_customers);
    if (total_business > 0){
        printf("Business-class: %.2f seconds. \n", business_wait_time/total_business);
    } else {
        printf("No business-class customers were served. \n");
    }
    if (total_econ > 0){
        printf("Economy class: %.2f seconds. \n\n", econ_wait_time/total_econ);
    } else {
        printf("No economy-class customers were served. \n\n");
    }
}


int main(int argc, char *argv[]) {

    if (argc != 2){
        fprintf(stderr, "Usage: ACS <input_file>\n");
        exit(1);
    }

    /************* GLOBAL INITIALIZATION *************/

    if (pthread_cond_init(&customers_available, NULL)){
        fprintf(stderr, "convar initialization error\n");
        exit(1);
    }
    if (pthread_mutex_init(&acs_mutex, NULL)){
        fprintf(stderr, "mutex initialization error\n");
        exit(1);
    }
    queue_init(&business_queue);
    queue_init(&econ_queue);
    gettimeofday(&start_time, NULL);

    /************* FILE READING *************/

    char *file = argv[1];
	
    FILE *fp = fopen(file, "r");
    if (!fp){
        fprintf(stderr, "File not found, ensure file is in local directory\n");
        exit(1);
    }

    char line[64];

    if (!fgets(line, sizeof(line), fp)) {
        fprintf(stderr, "Failed to read customer count\n");
        exit(1);
    }       

    int total_customers = atoi(line);
    if (total_customers <= 0) {
        fprintf(stderr, "Customer count %d invalid, must be greater than 0\n", total_customers);
        exit(1);
    }

    Customer **customer_info = malloc(sizeof(Customer *) * total_customers);
    if (!customer_info){
        fprintf(stderr, "malloc failed");
        exit(1);
    }

    for (int i = 0; i < total_customers; i++){
        if (!fgets(line, sizeof(line), fp)) {
            fprintf(stderr, "Unexpected end of file at line %d.\n", i + 2);
            exit(1);
        }

        int id, class_type, arrival_time, service_time;

        if (sscanf(line, "%d:%d,%d,%d", &id, &class_type, &arrival_time, &service_time) != 4) //Parsing line
        {
            fprintf(stderr, "Line #%d in file formatted incorrectly: %s\n", i+2, line);
            exit(1);
        } 

        /* FILE ERROR HANDLING */

        if (id < 0){
            fprintf(stderr, "Customer ID on line %d must be non-negative.\n", i+2);
            exit(1);
        }

        if (class_type != 0 && class_type != 1){
            fprintf(stderr, "Class on line %d must be 0 (Economy) or 1 (Business), defaulting to economy.\n", i+2);
            class_type =  0;
        }

        if (arrival_time < 0){
            fprintf(stderr, "Arrival time on line %d must be non-negative.\n", i+2);
            exit(1);
        }

        if (service_time <= 0){
            fprintf(stderr, "Service time on line %d must be greater than 0.\n", i+2);
            exit(1);
        }

        customer_info[i] = create_customer(id, class_type, arrival_time, service_time); //Adding customer to array of customers
        if(!customer_info[i]){
            fprintf(stderr, "Customer creation failed");
            exit(1);
        }
    }

    fclose(fp);

    /************* THREAD CREATION *************/
    printf("\nCustID = ID of given customer\n");
    printf("Start = Clerk begins to serve customer\n");
    printf("Finish = Clerk finishes serving customer\n");

    printf("\n----------------------------------------------------\n")
    printf("%-8s %-7s %-12s %-8s %-6s %-5s\n",
    "Time", "CustID", "Event", "ClerkID", "Queue", "Length"); //header for output table
    printf("----------------------------------------------------\n")


	/*creating five clerk threads*/
    pthread_t clerks[5];
    int clerk_info[5] = {1,2,3,4,5};

	for(int i = 0; i < 5; i++){
		if (pthread_create(&clerks[i], NULL, clerk_entry, (void *)&clerk_info[i]) != 0){
            fprintf(stderr, "clerk pthread_create error\n");
            exit(1);
        } 
	}
	
	/*creating n customer threads*/
    pthread_t *customers = malloc(sizeof(pthread_t) * total_customers);
	for(int i = 0; i < total_customers; i++){ // number of customers
		if (pthread_create(&customers[i], NULL, customer_entry, (void *)customer_info[i]) != 0){
            fprintf(stderr, "customer pthread_create error\n");
            free(customers);
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

    if (pthread_mutex_destroy(&acs_mutex)){
        fprintf(stderr, "mutex destroy failed\n");
    }
    if (pthread_cond_destroy(&customers_available)){
        fprintf(stderr, "convar destroy failed\n");
    }

	free(customers);
    
    print_wait_times(customer_info, total_customers); /**** WAIT TIME CALCULATION ****/

    for(int i = 0; i < total_customers; i++){
		free(customer_info[i]);
	}
    free(customer_info);
	return 0;
}







