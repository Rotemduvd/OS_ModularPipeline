#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H

#include "../plugin_common.h"

// Buffer configuration
#define BUFFER_SIZE 10
#define MAX_ITEMS 100

// Item structure
typedef struct {
    int id;
    char data[64];
    long timestamp;
} item_t;

// Producer-Consumer configuration
typedef struct {
    int num_producers;
    int num_consumers;
    int items_per_producer;
    int buffer_size;
    int use_semaphores;
    int use_condition_variables;
} cp_config_t;

// Producer-Consumer statistics
typedef struct {
    int items_produced;
    int items_consumed;
    int buffer_full_count;
    int buffer_empty_count;
    double avg_production_time;
    double avg_consumption_time;
} cp_stats_t;

// Main functions
int cp_init(cp_config_t *config);
int cp_start(void);
int cp_stop(void);
void cp_cleanup(void);

// Statistics
cp_stats_t* cp_get_stats(void);
void cp_print_stats(void);

// Thread functions (for internal use)
void* producer_thread(void *arg);
void* consumer_thread(void *arg);

#endif // CONSUMER_PRODUCER_H 