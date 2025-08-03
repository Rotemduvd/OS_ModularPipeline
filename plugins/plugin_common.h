#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Common plugin error codes
#define PLUGIN_SUCCESS 0
#define PLUGIN_ERROR -1
#define PLUGIN_INVALID_PARAM -2

// Common plugin utilities
void plugin_log(const char *format, ...);
int plugin_validate_params(void *params, size_t size);
void plugin_sleep_ms(int milliseconds);

// Thread utilities
typedef struct {
    pthread_t thread;
    int running;
    void *(*func)(void *);
    void *arg;
} plugin_thread_t;

int create_plugin_thread(plugin_thread_t *thread, void *(*func)(void *), void *arg);
void stop_plugin_thread(plugin_thread_t *thread);

#endif // PLUGIN_COMMON_H 