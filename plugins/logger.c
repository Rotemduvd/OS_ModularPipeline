#include "plugin_common.h"
#include <stdio.h>

// log the input string to stdout
static const char* plugin_transform(const char* input) {
    if (input) {
        printf("[logger] %s\n", input); // print the string
        fflush(stdout);
    }
    return input; // pass unchanged
}

/* get plugin name */
const char* plugin_get_name(void) {
    return "logger";
}

/* init plugin */
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "logger", queue_size);
}