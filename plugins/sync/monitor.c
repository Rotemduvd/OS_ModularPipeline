#include "monitor.h"
#include <stdlib.h>

int monitor_init(monitor_t* monitor) {
    
    // Check for null pointer
    if (!monitor){
        return -1; 
    }

    // Mutex initialization failed
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0){
        return -1; 
    }

    // Condition variable initialization failed
    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        return -1;
    }
    
    monitor->signaled = 0;
    return 0; // on success
}

void monitor_destroy(monitor_t* monitor) {
    pthread_mutex_destroy(&monitor->mutex);// destroy mutex
    pthread_cond_destroy(&monitor->condition); // destroy condition variable
}

void monitor_signal(monitor_t* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 1;
    pthread_cond_broadcast(&monitor->condition);  // waking up waiting threads
    pthread_mutex_unlock(&monitor->mutex);
}

void monitor_reset(monitor_t* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 0;
    pthread_mutex_unlock(&monitor->mutex);
}

int monitor_wait(monitor_t* monitor) {
    pthread_mutex_lock(&monitor->mutex);

    // loops until signaled
    while (!monitor->signaled) {              
        pthread_cond_wait(&monitor->condition, &monitor->mutex);
    }
    
    pthread_mutex_unlock(&monitor->mutex);
    return 0;
}