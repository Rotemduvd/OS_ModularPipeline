#include "consumer_producer.h"
#include <semaphore.h>
#include <sys/time.h>

// Global variables
static cp_config_t g_config;
static item_t *g_buffer;
static int g_buffer_head = 0;
static int g_buffer_tail = 0;
static int g_buffer_count = 0;
static int g_running = 0;
static cp_stats_t g_stats = {0};

// Synchronization primitives
static pthread_mutex_t g_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_buffer_not_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t g_buffer_not_empty = PTHREAD_COND_INITIALIZER;
static sem_t g_empty_slots;
static sem_t g_filled_slots;

// Thread arrays
static plugin_thread_t *g_producer_threads = NULL;
static plugin_thread_t *g_consumer_threads = NULL;

// Producer thread function
void* producer_thread(void *arg) {
    int producer_id = *(int*)arg;
    int items_produced = 0;
    struct timeval start_time, end_time;
    
    plugin_log("Producer %d started", producer_id);
    
    while (g_running && items_produced < g_config.items_per_producer) {
        item_t item;
        item.id = producer_id * g_config.items_per_producer + items_produced;
        snprintf(item.data, sizeof(item.data), "Data from producer %d", producer_id);
        item.timestamp = time(NULL);
        
        gettimeofday(&start_time, NULL);
        
        if (g_config.use_semaphores) {
            // Using semaphores
            sem_wait(&g_empty_slots);
            pthread_mutex_lock(&g_buffer_mutex);
        } else {
            // Using condition variables
            pthread_mutex_lock(&g_buffer_mutex);
            while (g_buffer_count >= g_config.buffer_size && g_running) {
                g_stats.buffer_full_count++;
                pthread_cond_wait(&g_buffer_not_full, &g_buffer_mutex);
            }
        }
        
        if (!g_running) {
            pthread_mutex_unlock(&g_buffer_mutex);
            break;
        }
        
        // Add item to buffer
        g_buffer[g_buffer_tail] = item;
        g_buffer_tail = (g_buffer_tail + 1) % g_config.buffer_size;
        g_buffer_count++;
        g_stats.items_produced++;
        
        gettimeofday(&end_time, NULL);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                        (end_time.tv_usec - start_time.tv_usec) / 1000.0;
        g_stats.avg_production_time = (g_stats.avg_production_time * (g_stats.items_produced - 1) + elapsed) / g_stats.items_produced;
        
        if (g_config.use_semaphores) {
            pthread_mutex_unlock(&g_buffer_mutex);
            sem_post(&g_filled_slots);
        } else {
            pthread_cond_signal(&g_buffer_not_empty);
            pthread_mutex_unlock(&g_buffer_mutex);
        }
        
        items_produced++;
        plugin_sleep_ms(10); // Simulate work
    }
    
    plugin_log("Producer %d finished, produced %d items", producer_id, items_produced);
    return NULL;
}

// Consumer thread function
void* consumer_thread(void *arg) {
    int consumer_id = *(int*)arg;
    int items_consumed = 0;
    struct timeval start_time, end_time;
    
    plugin_log("Consumer %d started", consumer_id);
    
    while (g_running) {
        gettimeofday(&start_time, NULL);
        
        if (g_config.use_semaphores) {
            // Using semaphores
            sem_wait(&g_filled_slots);
            pthread_mutex_lock(&g_buffer_mutex);
        } else {
            // Using condition variables
            pthread_mutex_lock(&g_buffer_mutex);
            while (g_buffer_count == 0 && g_running) {
                g_stats.buffer_empty_count++;
                pthread_cond_wait(&g_buffer_not_empty, &g_buffer_mutex);
            }
        }
        
        if (!g_running && g_buffer_count == 0) {
            pthread_mutex_unlock(&g_buffer_mutex);
            break;
        }
        
        // Remove item from buffer
        item_t item = g_buffer[g_buffer_head];
        g_buffer_head = (g_buffer_head + 1) % g_config.buffer_size;
        g_buffer_count--;
        g_stats.items_consumed++;
        
        gettimeofday(&end_time, NULL);
        double elapsed = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                        (end_time.tv_usec - start_time.tv_usec) / 1000.0;
        g_stats.avg_consumption_time = (g_stats.avg_consumption_time * (g_stats.items_consumed - 1) + elapsed) / g_stats.items_consumed;
        
        if (g_config.use_semaphores) {
            pthread_mutex_unlock(&g_buffer_mutex);
            sem_post(&g_empty_slots);
        } else {
            pthread_cond_signal(&g_buffer_not_full);
            pthread_mutex_unlock(&g_buffer_mutex);
        }
        
        items_consumed++;
        plugin_log("Consumer %d consumed item %d: %s", consumer_id, item.id, item.data);
        plugin_sleep_ms(20); // Simulate work
    }
    
    plugin_log("Consumer %d finished, consumed %d items", consumer_id, items_consumed);
    return NULL;
}

int cp_init(cp_config_t *config) {
    if (config == NULL) {
        return PLUGIN_INVALID_PARAM;
    }
    
    g_config = *config;
    
    // Allocate buffer
    g_buffer = malloc(g_config.buffer_size * sizeof(item_t));
    if (g_buffer == NULL) {
        return PLUGIN_ERROR;
    }
    
    // Initialize synchronization primitives
    if (g_config.use_semaphores) {
        sem_init(&g_empty_slots, 0, g_config.buffer_size);
        sem_init(&g_filled_slots, 0, 0);
    }
    
    // Allocate thread arrays
    g_producer_threads = malloc(g_config.num_producers * sizeof(plugin_thread_t));
    g_consumer_threads = malloc(g_config.num_consumers * sizeof(plugin_thread_t));
    
    if (g_producer_threads == NULL || g_consumer_threads == NULL) {
        cp_cleanup();
        return PLUGIN_ERROR;
    }
    
    // Initialize statistics
    memset(&g_stats, 0, sizeof(g_stats));
    
    plugin_log("Producer-Consumer plugin initialized with %d producers, %d consumers, buffer size %d", 
              g_config.num_producers, g_config.num_consumers, g_config.buffer_size);
    
    return PLUGIN_SUCCESS;
}

int cp_start(void) {
    if (g_running) {
        return PLUGIN_ERROR;
    }
    
    g_running = 1;
    
    // Start producer threads
    for (int i = 0; i < g_config.num_producers; i++) {
        int *producer_id = malloc(sizeof(int));
        *producer_id = i;
        if (create_plugin_thread(&g_producer_threads[i], producer_thread, producer_id) != PLUGIN_SUCCESS) {
            cp_stop();
            return PLUGIN_ERROR;
        }
    }
    
    // Start consumer threads
    for (int i = 0; i < g_config.num_consumers; i++) {
        int *consumer_id = malloc(sizeof(int));
        *consumer_id = i;
        if (create_plugin_thread(&g_consumer_threads[i], consumer_thread, consumer_id) != PLUGIN_SUCCESS) {
            cp_stop();
            return PLUGIN_ERROR;
        }
    }
    
    plugin_log("Producer-Consumer plugin started");
    return PLUGIN_SUCCESS;
}

int cp_stop(void) {
    if (!g_running) {
        return PLUGIN_ERROR;
    }
    
    g_running = 0;
    
    // Stop all threads
    for (int i = 0; i < g_config.num_producers; i++) {
        stop_plugin_thread(&g_producer_threads[i]);
    }
    
    for (int i = 0; i < g_config.num_consumers; i++) {
        stop_plugin_thread(&g_consumer_threads[i]);
    }
    
    plugin_log("Producer-Consumer plugin stopped");
    return PLUGIN_SUCCESS;
}

void cp_cleanup(void) {
    if (g_running) {
        cp_stop();
    }
    
    // Cleanup synchronization primitives
    if (g_config.use_semaphores) {
        sem_destroy(&g_empty_slots);
        sem_destroy(&g_filled_slots);
    }
    
    // Free memory
    if (g_buffer) {
        free(g_buffer);
        g_buffer = NULL;
    }
    
    if (g_producer_threads) {
        free(g_producer_threads);
        g_producer_threads = NULL;
    }
    
    if (g_consumer_threads) {
        free(g_consumer_threads);
        g_consumer_threads = NULL;
    }
    
    plugin_log("Producer-Consumer plugin cleaned up");
}

cp_stats_t* cp_get_stats(void) {
    return &g_stats;
}

void cp_print_stats(void) {
    plugin_log("=== Producer-Consumer Statistics ===");
    plugin_log("Items produced: %d", g_stats.items_produced);
    plugin_log("Items consumed: %d", g_stats.items_consumed);
    plugin_log("Buffer full count: %d", g_stats.buffer_full_count);
    plugin_log("Buffer empty count: %d", g_stats.buffer_empty_count);
    plugin_log("Average production time: %.2f ms", g_stats.avg_production_time);
    plugin_log("Average consumption time: %.2f ms", g_stats.avg_consumption_time);
    plugin_log("================================");
} 