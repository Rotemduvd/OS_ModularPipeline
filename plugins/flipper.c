#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>

//reverse tjhe input string
static const char* plugin_transform(const char* input) {

    // check if input is valid allocating memory for output
    if (input) {
        size_t len = strlen(input);
        char* out = malloc(len + 1);
        if (out) {

            // reverse the string
            for (size_t i = 0; i < len; i++) {
                out[i] = input[len - i - 1]; 
            }
            out[len] = '\0'; // adding null terminator
            return out;
        }
    }
    return NULL;
}

const char* plugin_get_name(void) {
    return "flipper";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "flipper", queue_size);
}