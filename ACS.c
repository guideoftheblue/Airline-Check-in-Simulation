#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "queue.h"
#include "customer.h"

/**** GLOBAL VARIABLES ****/
pthread_cond_t customers_available;
pthread_mutex_t acs_mutex;

int sim_status = 0; //1 if all customers served, 0 otherwise

Queue business_queue;
Queue econ_queue;

struct timeval start_time; //Initial relative machine time, captured in main.


/**
* get_elapsed_time: Determines the time since the beginning of the check-in simulation.
* 
* @return Seconds since the beginning of simulation.
**/ 
double get_elapsed_time(){
    struct timeval now; // Get current time
    gettimeofday(&now, NULL);

    //Seconds + (Microseconds -> Second)
    return (now.tv_sec - start_time.tv_sec) + 
           (now.tv_usec - start_time.tv_usec) / 1e6; 
}


/**
* print_event: Determines the time since the beginning of the check-in simulation.
* 
* @customer: Customer involved with the event, used to get customer id and class.
* 
* @event: Number representing the type of event to be printed. 1 for arrival, 2 for Queue entry,
*         3 for Clerk starting to serve customer, and 4 for Clerk finished serving customer.
* 
* @clerk_id: Number representing the clerk handling the event. -1 if event not handled by a clerk
* 
* @return Seconds since last the beginning of simulation.
**/ 
void print_event(Customer *customer, int event, int clerk_id){
    int cust_class = customer->class_type;
    int cust_id = customer->id;
    double now = get_elapsed_time();

    switch (event) {
    case 1: //Arrival event
        printf("%-8.2f %-7d %-12s %-8s %-6s %-5s\n", now, cust_id, "Arrival", "-", "-", "-");
        break;
    case 2: //Queue entry event
        printf("%-8.2f %-7d %-12s %-8s %-6d %-5d\n", customer->queue_entry_time, cust_id,
        "QueueEntry", "-", cust_class, cust_class == 0? econ_queue.length : business_queue.length);
        break;
    case 3: //Clerk service starts
        printf("%-8.2f %-7d %-12s %-8d %-6d %-5d\n", now, cust_id, "Start", clerk_id, cust_class, 
        cust_class == 0? econ_queue.length : business_queue.length);
        break;
    case 4: //Clerk service finishes
        printf("%-8.2f %-7d %-12s %-8d %-6d %-5d\n", now, cust_id, "Finish", clerk_id, cust_class, 
        cust_class == 0? econ_queue.length : business_queue.length);
        break;
    }
}


/**
* customer_entry: Function used to manage customer threads. First sleeps until designated arrival
*                 time, then enters appropriate queue and alerts clerks to its presence
* 
* @arg:
* 
* @return NULL to satisfy void* type
**/ 
void *customer_entry(void *arg){
    Customer *customer = (Customer *)arg;

    //Determine how long its been since start of program//
    double elapsed_time = get_elapsed_time();

    //Convert arrival_time to seconds//
    double arrival_time = customer->arrival_time * 0.1;

    //Calculate remaining time before arrival//
    double sleep_time = arrival_time - elapsed_time;

    if (sleep_time > 0){ //Ensuring non-negative sleep
        usleep((useconds_t)(sleep_time * 1e6)); //Sleep for remaining time in microseconds
    }
    
    print_event(customer, 1, -1);

    //Locking to protect queue access//
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
    
    //Alerting clerks to presence of customer in a queue//
    pthread_cond_broadcast(&customers_available);

    pthread_mutex_unlock(&acs_mutex);

    return NULL; 
}


/**
* clerk_entry: Function used to manage clerk threads. First finds its' ID. Then if there is still 
*              customers left to serve, waits until a customer is available in queue to serve. Then
*              prioritizes business customers over economy customers when choosing a customer to 
*              dequeue. Once customer is dequeued, sleeps for the customer's designated service time.
* 
* @arg:
* 
* @return NULL to satisfy void* type
**/ 
void *clerk_entry(void *arg){
    int clerk_id = *(int *)arg;

    while(1){
        pthread_mutex_lock(&acs_mutex);

        //While queues empty and customers left to serve
        while (business_queue.length == 0 && econ_queue.length == 0 && sim_status == 0){
            pthread_cond_wait(&customers_available, &acs_mutex);
        }

        //All customers served, simulation over//
        if (business_queue.length == 0 && econ_queue.length == 0 && sim_status == 1){ 
            pthread_mutex_unlock(&acs_mutex);
            break; //Exit thread
        }

        Customer *customer = NULL;
        if (business_queue.length != 0){
            customer = dequeue(&business_queue);
        } else if (econ_queue.length != 0){
            customer = dequeue(&econ_queue);
        }

        //Print start of service
        print_event(customer, 3, clerk_id);

        pthread_mutex_unlock(&acs_mutex);

        double start_time = get_elapsed_time();

        //Finding total time spent in queue
        double wait_time = start_time - customer->queue_entry_time;  
        customer->wait_time = wait_time;

        //Serving customer (1/10 of second -> microsecond)
        usleep((useconds_t)(customer->service_time * 1e5)); 

        //Print end of service, locking to make sure queue, lengths are accurate
        pthread_mutex_lock(&acs_mutex);
        print_event(customer, 4, clerk_id);
        pthread_mutex_unlock(&acs_mutex);
    }

    return NULL;  
}


/**
* print_wait_times: Calculates and prints average wait times for all customers, business customers,
*                   and economy customers. 
* 
* @customer_info: Array of customer structs which contain their wait times.
* 
* @total_customers: The total number of customers being served in the simulation
* 
* @return None
**/ 
void print_wait_times(Customer **customer_info, int total_customers){
    double total_wait_time = 0;
    double business_wait_time = 0;
    int total_business = 0;
    double econ_wait_time = 0;
    int total_econ = 0;


	for(int i = 0; i < total_customers; i++){
        Customer *customer = customer_info[i];

        total_wait_time += customer->wait_time;

        if (customer->class_type == 0){ //Economy total
            total_econ++;
            econ_wait_time += customer->wait_time;
        } else if (customer->class_type == 1) { //Business total
            total_business++;
            business_wait_time += customer->wait_time;
        }
    }

    printf("\n----------------------\n");
    printf("Average Waiting Times\n");
    printf("----------------------\n");
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


/**
* read_customers: Reads customers from given file and creates an array of customer structs
*                 according to the specifications in the file.
* 
* @filename: Pointer to string containing name of file to read customers from.
* 
* @total_customers: Pointer to int that holds total number of customers served by airline.
* 
* @return Array of newly created customer structs.
**/ 
Customer **read_customers(char *filename, int *total_customers){
    FILE *fp = fopen(filename, "r");
    if (!fp){
        fprintf(stderr, "File not found, ensure file is in local directory\n");
        exit(1);
    }

    char line[64];

    if (!fgets(line, sizeof(line), fp)) {
        fprintf(stderr, "Failed to read customer count\n");
        exit(1);
    }       

    //Get total number of customers from first line//
    *total_customers = atoi(line); 
    if (*total_customers <= 0) {
        fprintf(stderr, "Customer count %d invalid, must be greater than 0\n", *total_customers);
        exit(1);
    }

    //Allocating array to hold customers//
    Customer **customer_info = malloc(sizeof(Customer *) * *total_customers);
    if (!customer_info){
        fprintf(stderr, "malloc failed");
        exit(1);
    }

    //for each line in file
    for (int i = 0; i < *total_customers; i++){
        if (!fgets(line, sizeof(line), fp)) { //File contains too few customers
            fprintf(stderr, "Unexpected end of file at line %d.", i + 2);
            exit(1);
        }

        int id, class_type, arrival_time, service_time;

        //Parsing line//
        if (sscanf(line, "%d:%d,%d,%d", &id, &class_type, &arrival_time, &service_time) != 4) 
        {
            fprintf(stderr, "Line #%d in file formatted incorrectly: %s\n", i+2, line);
            exit(1);
        } 

        /*** FILE ERROR HANDLING ***/

        //Negative customer ID//
        if (id < 0){
            fprintf(stderr, "Customer ID on line %d must be non-negative.\n", i+2);
            exit(1);
        }

        //Invalid customer class type//
        if (class_type != 0 && class_type != 1){
            fprintf(stderr,
                    "Class on line %d must be 0 (Economy) or 1 (Business), defaulting to economy.\n",
                    i+2);
            class_type =  0;
        }

        //Negative customer arrival time//
        if (arrival_time < 0){
            fprintf(stderr, "Arrival time on line %d must be non-negative.\n", i+2);
            exit(1);
        }

        //Negative or 0 service time//
        if (service_time <= 0){
            fprintf(stderr, "Service time on line %d must be greater than 0.\n", i+2);
            exit(1);
        }

        //Adding customer to array of customers//
        customer_info[i] = create_customer(id, class_type, arrival_time, service_time); 
        if(!customer_info[i]){
            fprintf(stderr, "Customer creation failed");
            exit(1);
        }
    }

    fclose(fp);

    return customer_info;
}


int main(int argc, char *argv[]) {

    if (argc != 2){
        fprintf(stderr, "Usage: ACS <input_file>\n");
        exit(1);
    }

    /******************* GLOBAL INITIALIZATION  *******************/ 

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

    /******************* FILE READING *******************/

    char *file = argv[1];
    int total_customers = 0;

    //Reads customer file and loads into an array of customers//
    Customer **customer_info = read_customers(file, &total_customers);

    /******************* THREAD CREATION *******************/
    
    //Legend for table headers and event terminology//
    printf("\nCustID = ID of given customer\n");
    printf("Start = Clerk begins to serve customer\n");
    printf("Finish = Clerk finishes serving customer\n");

    //Printing header for output table//
    printf("\n----------------------------------------------------\n");
    printf("%-8s %-7s %-12s %-8s %-6s %-5s\n",
    "Time", "CustID", "Event", "ClerkID", "Queue", "Length"); 
    printf("----------------------------------------------------\n");


	//Creating five clerk threads//
    pthread_t clerks[5];
    int clerk_info[5] = {1,2,3,4,5};

	for(int i = 0; i < 5; i++){
		if (pthread_create(&clerks[i], NULL, clerk_entry, (void *)&clerk_info[i]) != 0){
            fprintf(stderr, "clerk pthread_create error\n");
            exit(1);
        } 
	}
	
	//Creating n customer threads//
    pthread_t *customers = malloc(sizeof(pthread_t) * total_customers);
	for(int i = 0; i < total_customers; i++){ // number of customers
		if (pthread_create(&customers[i], NULL, customer_entry, (void *)customer_info[i]) != 0){
            fprintf(stderr, "customer pthread_create error\n");
            free(customers);
            exit(1);
        } 
	}
    
    /******************* THREAD CLEANUP *******************/

	//Wait for all customer threads to terminate//
	for(int i = 0; i < total_customers; i++){
		pthread_join(customers[i], NULL);
	}

    //Locking mutex so there's no race with clerks for sim_status//
    pthread_mutex_lock(&acs_mutex); 

    sim_status= 1; //Update simulation status to complete, allowing clerks to exit

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
    
    /******************* WAIT TIME CALCULATION *******************/

    print_wait_times(customer_info, total_customers); 

    for(int i = 0; i < total_customers; i++){
		free(customer_info[i]);
	}
    free(customer_info);

	return 0;
}







