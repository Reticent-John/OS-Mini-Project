#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define MAX_ITEMS 20 // Maximum no. of items a producer can make
#define POISON_PILL -1

// Defining the Circular Buffer Object 
typedef struct {
    int* buffer;
    struct timespec* enq_time; // Array to store enqueue timestamps
    struct timespec* deq_time; // Array to store dequeue timestamps
    int size;
    int in; // Index for the next add
    int out; // Index for the next remove 
} Circular_Buffer;

Circular_Buffer cb;
sem_t empty; // Counts Empty Slots
sem_t full; // Counts Full Slots

pthread_mutex_t mutex; // Protects buffer and ensures mutual exclusion

// Initializes Circular Buffer to the size
void buffer_init(int size) { 
    cb.buffer = (int *)malloc(size * sizeof(int)); // Allocates Memory
    cb.size = size;
    cb.in = 0;
    cb.out = 0;
    
    // Error Check
    if (cb.buffer == NULL) { 
        printf("Error: Failed to allocate memory!\n");
        exit(1);
    }

    // Allocating Memory for the Arrays
    cb.enq_time = malloc(size * sizeof(struct timespec));
    cb.deq_time = malloc(size * sizeof(struct timespec));
    
    // Error Check
    if (!cb.enq_time || !cb.deq_time) {
        printf("Error: Failed to allocate timestamp arrays\n");
        exit(1);
    }
}

// Inserting item
int insert_item(int item) { 
    sem_wait(&empty); // Waits until empty slot available in the buffer
    pthread_mutex_lock(&mutex); // Locks buffer to enter critical section    

    // Inserting item
    int index = cb.in;
    cb.buffer[index] = item;
    clock_gettime(CLOCK_MONOTONIC, &cb.enq_time[index]); // Recording enqueue timestamp
    cb.in = (cb.in + 1) % cb.size;

    pthread_mutex_unlock(&mutex); // Unlocks buffer to exit critical section
    sem_post(&full); // Signals that there is a new item

    return index;
}

// Defining object used to return result of remove_item()
typedef struct {
    int item;
    int index;
} Removal_Result;

// Removes item from buffer
Removal_Result remove_item() {
    sem_wait(&full); // Waits until atleast one item is present in buffer
    pthread_mutex_lock(&mutex); // Locks the buffer to enter critical section

    // Sets return values
    Removal_Result result;
    result.item = cb.buffer[cb.out];
    result.index = cb.out;
    
    // Removes item from buffer by letting out index be overwritten
    cb.out = (cb.out + 1) % cb.size; // Ensures Circular Implementation

    pthread_mutex_unlock(&mutex); // Unlocks buffer to exit critical section
    sem_post(&empty); // Signals that there is an empty slot

    clock_gettime(CLOCK_MONOTONIC, &cb.deq_time[result.index]); // Recording dequeue timestamp

    return result;
}

int total_prod = 0;
int total_consumed = 0;
pthread_mutex_t stats_mutex;

// Latency and Throughput Measurement
double total_latency = 0.0;
long total_latency_count = 0;

struct timespec start_time;
struct timespec end_time;

// Producer Thread Function
void *producer(void *arg) {
    int id = (int)(long)arg;
    int seed = time(NULL) + id; // Seed set for producer

    for (int i =0; i < MAX_ITEMS; i++) { // Produces maximum no. of items per producer
        int item = (rand_r(&seed) % 100) + 1; // Generates random item

        int index = insert_item(item);
        printf("[Producer - %d] Produced the Item %d at index %d\n", id, item, index);

        // Updates the stats variable
        pthread_mutex_lock(&stats_mutex);
        total_prod++;
        pthread_mutex_unlock(&stats_mutex);
    }

    printf("[Producer - %d] Finished producing %d items\n", id, MAX_ITEMS);
    return NULL;
}

// Consumer Thread Function
void *consumer(void *arg) {
    int id = (int)(long)arg;
    int items_consumed = 0;

    while(1) {
        Removal_Result result = remove_item(); // Item gets removed

        if(result.item == POISON_PILL) { // If item is poison pill then consumer is killed
            printf("[Consumer - %d] Received the poison pill. Exiting\n", id);
            break;
        }

        printf("[Consumer - %d] Consumed the item %d at index %d\n", id, result.item, result.index);
        items_consumed++;
        
        // Calculates Latency
        struct timespec enq = cb.enq_time[result.index];
        struct timespec deq = cb.deq_time[result.index];

        long sec = deq.tv_sec - enq.tv_sec;
        long nsec = deq.tv_nsec - enq.tv_nsec;
        double ms = sec * 1000.0 + nsec / 1e6;

        // Updates stats and Latency variables
        pthread_mutex_lock(&stats_mutex);
        total_consumed++;
        
        total_latency += ms;
        total_latency_count++;
        
        pthread_mutex_unlock(&stats_mutex);
    }

    printf("[Consumer - %d] Finished consuming %d items.\n", id, items_consumed);
    return NULL;
}


// Validating arguments
int validate_args(int arg_count, char *arg_vec[], int *num_producers, int *num_consumers, int *buffer_size) {
    if (arg_count != 4) {
        printf("Error: Arguments are %s <num_producers> <num_consumers> <buffer_size>\n", arg_vec[0]);
        return 0;
    }

    // Converting to int
    *num_producers = atoi(arg_vec[1]);
    *num_consumers = atoi(arg_vec[2]);
    *buffer_size = atoi(arg_vec[3]);


    // Validating each argument
    if ((*num_producers <= 0) || (*num_producers > 100)) {
        printf("Error: Number of producers has to be between 1 and 100\n");
        return 0;
    }
    
    if ((*num_consumers <= 0) || (*num_consumers > 100)) {
        printf("Error: Number of consumers has to be between 1 and 100\n");
        return 0;
    }
    
    if ((*buffer_size <= 0) || (*buffer_size > 1000)) {
        printf("Error: Buffer size has to be between 1 and 1000\n");
        return 0;
    }
    
    return 1;
}

int main(int arg_count, char *arg_vec[]) {
    int num_producers, num_consumers, buffer_size;
    
    // Validating input
    if (!validate_args(arg_count, arg_vec, &num_producers, &num_consumers, &buffer_size)) {
        return 1;
    }
    
    printf("\n--- Producer-Consumer Simulation ---\n");
    printf("Producers: %d, Consumers: %d, Buffer Size: %d\n", num_producers, num_consumers, buffer_size);
    printf("Each producer will generate %d items\n", MAX_ITEMS);
    printf("-----------------------------------------------------\n\n");
    
    buffer_init(buffer_size); // Initializing Buffer

    // Record Simulation Start Time
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    
    // Initializing Semaphores
    if (sem_init(&empty, 0, buffer_size) != 0) { 
        printf("Error: Failed to initialize empty semaphore\n");
        return 1;
    }
    if (sem_init(&full, 0, 0) != 0) {
        printf("Error: Failed to initialize full semaphore\n");
        return 1;
    }
    
    // Initializing the mutexes
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Error: Failed to initialize buffer mutex\n");
        return 1;
    }
    if (pthread_mutex_init(&stats_mutex, NULL) != 0) {
        printf("Error: Failed to initialize stats mutex\n");
        return 1;
    }
    
    // Creating Producer Threads
    pthread_t *producers = (pthread_t *)malloc(num_producers * sizeof(pthread_t));
    for (int i = 0; i < num_producers; i++) {
        if (pthread_create(&producers[i], NULL, producer, (void *)(long)(i + 1)) != 0) {
            printf("Error: Failed to create producer thread %d\n", i + 1);
            return 1;
        }
    }
    
    // Creating Consumer Threads
    pthread_t *consumers = (pthread_t *)malloc(num_consumers * sizeof(pthread_t));
    for (int i = 0; i < num_consumers; i++) {  
        if (pthread_create(&consumers[i], NULL, consumer, (void *)(long)(i + 1)) != 0) {
            printf("Error: Failed to create consumer thread %d\n", i + 1);
            return 1;
        }
    }
    
    // Waiting for all producers to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producers[i], NULL);
    }
    
    printf("\n--- All producers finished ---\nInserting the %d poison pills...\n\n", num_consumers);
    
    // Inserts poison pill for all the consumers
    for (int i = 0; i < num_consumers; i++) {
        insert_item(POISON_PILL);
    }
    
    // Waiting for all consumers to die
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    // Recording the simulation end time
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    printf("\n--- Simulation Complete ---\n");
    printf("Total items produced: %d\n", total_prod);
    printf("Total items consumed: %d\n", total_consumed);
    printf("Expected items: %d\n", num_producers * MAX_ITEMS);
    double average_latency = total_latency / total_latency_count;
    printf("\n--- Latency Statistics ---\n");
    printf("Average latency: %.3f ms\n", average_latency);
    double duration_sec = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    double throughput = total_consumed / duration_sec;
    printf("Throughput: %.2f items/sec\n", throughput);
    
    free(producers);
    free(consumers);
    free(cb.buffer);
    free(cb.enq_time);
    free(cb.deq_time);
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&stats_mutex);
    
    return 0;
}