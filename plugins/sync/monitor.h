#ifndef MONITOR_H
#define MONITOR_H

#include "plugin_common.h"

// Monitor plugin configuration
typedef struct {
    int interval_ms;
    int max_samples;
    char *log_file;
} monitor_config_t;

// Monitor data structure
typedef struct {
    double cpu_usage;
    double memory_usage;
    int active_threads;
    long timestamp;
} monitor_data_t;

// Monitor plugin functions
int monitor_init(monitor_config_t *config);
int monitor_start(void);
int monitor_stop(void);
void monitor_cleanup(void);

// Monitor data access
monitor_data_t* monitor_get_latest_data(void);
int monitor_get_data_history(monitor_data_t *buffer, int max_count);

// Monitor callbacks
typedef void (*monitor_callback_t)(monitor_data_t *data);
void monitor_set_callback(monitor_callback_t callback);

#endif // MONITOR_H
