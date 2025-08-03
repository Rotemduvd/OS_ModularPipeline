#include "plugin_common.h"
#include <stdarg.h>
#include <time.h>

void plugin_log(const char *format, ...) {
    time_t now;
    struct tm *tm_info;
    char time_str[26];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] ", time_str);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

int plugin_validate_params(void *params, size_t size) {
    if (params == NULL || size == 0) {
        return PLUGIN_INVALID_PARAM;
    }
    return PLUGIN_SUCCESS;
}

void plugin_sleep_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

int create_plugin_thread(plugin_thread_t *thread, void *(*func)(void *), void *arg) {
    if (thread == NULL || func == NULL) {
        return PLUGIN_ERROR;
    }
    
    thread->func = func;
    thread->arg = arg;
    thread->running = 1;
    
    if (pthread_create(&thread->thread, NULL, func, arg) != 0) {
        return PLUGIN_ERROR;
    }
    
    return PLUGIN_SUCCESS;
}

void stop_plugin_thread(plugin_thread_t *thread) {
    if (thread != NULL) {
        thread->running = 0;
        pthread_join(thread->thread, NULL);
    }
}
