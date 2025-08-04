#include "plugin_common.h"
#include <stdio.h>
#include <unistd.h>

// print the input string character by character with a 100msdelay
static const char* plugin_transform(const char* input) {
    if (input) {
        for (size_t i = 0; input[i] != '\0'; i++) {
            putchar(input[i]);
            fflush(stdout);
            usleep(100000); // delay 100ms
        }
        putchar('\n');
    }
    return input; // pass unchanged
}

const char* plugin_get_name(void) {
    return "typewriter";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "typewriter", queue_size);
}