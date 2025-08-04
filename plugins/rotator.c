#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>

// rotate the string one char right 
static const char* plugin_transform(const char* input) {
    if (input) {
        size_t len = strlen(input);
        if (len > 0) {
            char* out = malloc(len + 1);
            if (out) {
                out[0] = input[len - 1]; // last char first
                for (size_t i = 1; i < len; i++) {
                    out[i] = input[i - 1]; // shift rest
                }
                out[len] = '\0';
                return out;
            }
        }
    }
    return NULL;
}

const char* plugin_get_name(void) {
    return "rotator";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}