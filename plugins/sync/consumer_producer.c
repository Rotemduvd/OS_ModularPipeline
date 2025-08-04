#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>

// init queue
const char* consumer_producer_init(consumer_producer_t* q, int capacity) {
   
    if (!q || capacity <= 0){
        return "args are invalid"; // check for null pointer and non-positive capacity
    } 

    // allocate memory for items and check if allocation was successful
    q->items = (char**)malloc(sizeof(char*) * capacity);
    if (!q->items) return "malloc failed"; 

    // initialize queue properties
    q->capacity = capacity;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    q->is_finished = 0;

    // initialize monitors and mutex
    if (pthread_mutex_init(&q->lock, NULL) != 0) return "mutex init failed";
    if (monitor_init(&q->not_full_monitor) != 0) return "monitor init failed";
    if (monitor_init(&q->not_empty_monitor) != 0) return "monitor init failed";
    if (monitor_init(&q->finished_monitor) != 0) return "monitor init failed";

    return NULL;
}

// destroy queue and free resources
void consumer_producer_destroy(consumer_producer_t* q) {
    if (!q) return; // check for null pointer

    // free remaining items in queue
    for (int i = 0; i < q->count; i++) {
        free(q->items[(q->head + i) % q->capacity]); 
    }

    free(q->items); // free all items in the queue

    // destroy mutex and monitors
    pthread_mutex_destroy(&q->lock);
    monitor_destroy(&q->not_full_monitor);
    monitor_destroy(&q->not_empty_monitor);
    monitor_destroy(&q->finished_monitor);
}

// put item into queue
const char* consumer_producer_put(consumer_producer_t* q, const char* item) {
    if (!q || !item) return "args are invalid"; // check for null pointers

    pthread_mutex_lock(&q->lock); // lock the mutex to protect shared state

    // wait until there is space in the queue or it is finished
    while (q->count == q->capacity && !q->is_finished) {
        pthread_mutex_unlock(&q->lock);
        monitor_wait(&q->not_full_monitor);
        pthread_mutex_lock(&q->lock);
    }

    // if the queue is finished wont accept new items
    if (q->is_finished) {
        pthread_mutex_unlock(&q->lock);
        return "queue finished";
    }

    // allocate memory for the new item and check if allocation successful
    q->items[q->tail] = strdup(item);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    // signal that there is at least one item in the queue
    monitor_signal(&q->not_empty_monitor);
    pthread_mutex_unlock(&q->lock);
    return NULL;
}

// get item from queue
char* consumer_producer_get(consumer_producer_t* q) {
    if (!q) return NULL; // null pointer check
    pthread_mutex_lock(&q->lock);

    // wait until there is an item in the queue or it is finished
    while (q->count == 0 && !q->is_finished) {
        pthread_mutex_unlock(&q->lock);
        monitor_wait(&q->not_empty_monitor);
        pthread_mutex_lock(&q->lock);
    }

    // if the queue is finished and empty, return NULL
    if (q->count == 0 && q->is_finished) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    // get the item from the queue and update the state
    char* item = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    monitor_signal(&q->not_full_monitor); // signal that there is space in the queue
    pthread_mutex_unlock(&q->lock);
    return item;
}
// signal that processing is finished
void consumer_producer_signal_finished(consumer_producer_t* q) {
    if (!q) return; // null pointer check
    
    pthread_mutex_lock(&q->lock);
    q->is_finished = 1;
    monitor_signal(&q->not_empty_monitor);
    monitor_signal(&q->not_full_monitor);
    monitor_signal(&q->finished_monitor);
    pthread_mutex_unlock(&q->lock);
}

int consumer_producer_wait_finished(consumer_producer_t* q) {
    return monitor_wait(&q->finished_monitor);
}