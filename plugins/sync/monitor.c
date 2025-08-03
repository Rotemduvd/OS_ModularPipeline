#include "monitor.h"
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

static monitor_config_t g_config;
static plugin_thread_t g_monitor_thread;
static monitor_data_t g_latest_data;
static monitor_callback_t g_callback = NULL;
static int g_running = 0;

// Monitor thread function
static void* monitor_thread_func(void *arg) {
    (void)arg; // Unused parameter
    
    while (g_running) {
        // Collect system metrics
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            g_latest_data.cpu_usage = (double)usage.ru_utime.tv_sec + 
                                     (double)usage.ru_utime.tv_usec / 1000000.0;
        }
        
        // Get memory usage (simplified)
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            g_latest_data.memory_usage = (double)(info.totalram - info.freeram) / 
                                        (double)info.totalram * 100.0;
        }
        
        g_latest_data.active_threads = 1; // Simplified
        g_latest_data.timestamp = time(NULL);
        
        // Call callback if set
        if (g_callback != NULL) {
            g_callback(&g_latest_data);
        }
        
        plugin_log("Monitor: CPU=%.2f%%, Memory=%.2f%%, Threads=%d", 
                  g_latest_data.cpu_usage, g_latest_data.memory_usage, 
                  g_latest_data.active_threads);
        
        plugin_sleep_ms(g_config.interval_ms);
    }
    
    return NULL;
}

int monitor_init(monitor_config_t *config) {
    if (config == NULL) {
        return PLUGIN_INVALID_PARAM;
    }
    
    g_config = *config;
    g_running = 0;
    
    plugin_log("Monitor plugin initialized with interval=%dms", config->interval_ms);
    return PLUGIN_SUCCESS;
}

int monitor_start(void) {
    if (g_running) {
        return PLUGIN_ERROR;
    }
    
    g_running = 1;
    if (create_plugin_thread(&g_monitor_thread, monitor_thread_func, NULL) != PLUGIN_SUCCESS) {
        g_running = 0;
        return PLUGIN_ERROR;
    }
    
    plugin_log("Monitor plugin started");
    return PLUGIN_SUCCESS;
}

int monitor_stop(void) {
    if (!g_running) {
        return PLUGIN_ERROR;
    }
    
    g_running = 0;
    stop_plugin_thread(&g_monitor_thread);
    
    plugin_log("Monitor plugin stopped");
    return PLUGIN_SUCCESS;
}

void monitor_cleanup(void) {
    if (g_running) {
        monitor_stop();
    }
    plugin_log("Monitor plugin cleaned up");
}

monitor_data_t* monitor_get_latest_data(void) {
    return &g_latest_data;
}

int monitor_get_data_history(monitor_data_t *buffer, int max_count) {
    if (buffer == NULL || max_count <= 0) {
        return PLUGIN_INVALID_PARAM;
    }
    
    // Simplified - just return current data
    buffer[0] = g_latest_data;
    return 1;
}

void monitor_set_callback(monitor_callback_t callback) {
    g_callback = callback;
}
