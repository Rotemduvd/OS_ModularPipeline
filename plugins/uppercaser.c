#include "plugin_common.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// convert input string to uppercase
static const char* plugin_transform(const char* input) {
    // check if input is valid
    if (input) {
        size_t len = strlen(input); // get length of input
        char* out = malloc(len + 1); // allocate memory for output

        if (out) { 
            // convert each character to uppercase
            for (size_t i = 0; i < len; i++) {
                out[i] = toupper((unsigned char)input[i]); // uppercase each char
            }
            out[len] = '\0'; // null terminator
            return out;
        }
    }
    return NULL;
}

const char* plugin_get_name(void) {
    return "uppercaser";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "uppercaser", queue_size);
}