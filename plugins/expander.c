#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>

// add spaces between characters in the input string
static const char* plugin_transform(const char* input) {
    if (input) {
        size_t len = strlen(input);
        if (len > 0) {
            char* out = malloc(len * 2); // space for chars + spaces
            if (out) {
                size_t idx = 0;
                for (size_t i = 0; i < len; i++) {
                    out[idx++] = input[i];
                    if (i < len - 1) {
                        out[idx++] = ' '; // add space
                    }
                }
                out[idx] = '\0';
                return out;
            }
        }
    }
    return NULL;
}

const char* plugin_get_name(void) {
    return "expander";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "expander", queue_size);
}