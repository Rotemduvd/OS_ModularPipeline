#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "plugins/plugin_sdk.h"

#define MAX_PLUGINS 10
#define MAX_LINE 1024

// struct to store plugin handles and functions
typedef struct {
    const char* (*init)(int);
    const char* (*fini)(void);
    const char* (*place_work)(const char*);
    void (*attach)(const char* (*)(const char*));
    const char* (*wait_finished)(void);
    const char* (*get_name)(void);
    void* handle;
} plugin_handle_t;

// print usage help
void print_usage() {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("Arguments:\n");
    printf("    queue_size      Maximum number of items in each plugin's queue\n");
    printf("    plugin1..N      Names of plugins to load (without .so extension)\n");
    printf("Available plugins:\n");
    printf("    logger       - Logs all strings that pass through\n");
    printf("    typewriter   - Simulates typewriter effect with delays\n");
    printf("    uppercaser   - Converts strings to uppercase\n");
    printf("    rotator      - Move every character right; last moves to start\n");
    printf("    flipper      - Reverses order of characters\n");
    printf("    expander     - Expands each character with spaces\n");
    printf("Example:\n");
    printf("    ./analyzer 20 uppercaser rotator logger\n");
}

int main(int argc, char* argv[]) {
    // check args
    if (argc < 3) {
        fprintf(stderr, "error- there are missing arguments\n");
        print_usage();
        return 1;
    }

    // parse queue size
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "error- not a valid queue size\n");
        print_usage();
        return 1;
    }

    int plugin_count = argc - 2;
    plugin_handle_t plugins[MAX_PLUGINS];

    // load plugins
    for (int i = 0; i < plugin_count; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "./output/plugins/%s.so", argv[i + 2]); // build so path

        plugins[i].handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
        if (!plugins[i].handle) {
            fprintf(stderr, "error- failed to load %s: %s\n", filename, dlerror());
            print_usage();
            return 1;
        }

        // resolve functions
        plugins[i].init = dlsym(plugins[i].handle, "plugin_init");
        plugins[i].fini = dlsym(plugins[i].handle, "plugin_fini");
        plugins[i].place_work = dlsym(plugins[i].handle, "plugin_place_work");
        plugins[i].attach = dlsym(plugins[i].handle, "plugin_attach");
        plugins[i].wait_finished = dlsym(plugins[i].handle, "plugin_wait_finished");
        plugins[i].get_name = dlsym(plugins[i].handle, "plugin_get_name");

        if (!plugins[i].init || !plugins[i].fini || !plugins[i].place_work || !plugins[i].attach || !plugins[i].wait_finished || !plugins[i].get_name) {
            fprintf(stderr, "error- missing function in plugin %s\n", filename);
            print_usage();
            return 1;
        }
    }

    // init plugins
    for (int i = 0; i < plugin_count; i++) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "error- failed to init plugin %s: %s\n", plugins[i].get_name(), err);
            // cleanup loaded plugins so far
            for (int j = 0; j <= i; j++) {
                dlclose(plugins[j].handle);
            }
            return 2;
        }
    }

    // attach plugins in pipeline
    for (int i = 0; i < plugin_count - 1; i++) {
        plugins[i].attach(plugins[i + 1].place_work);
    }

    // read input from stdin
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0'; // strip newline

        if (strcmp(line, "<END>") == 0) {
            plugins[0].place_work("<END>");
            break;
        }

        plugins[0].place_work(line); // send to first plugin
    }

    // wait for plugins to finish
    for (int i = 0; i < plugin_count; i++) {
        plugins[i].wait_finished();
    }

    // cleanup and unload
    for (int i = 0; i < plugin_count; i++) {
        plugins[i].fini();
        dlclose(plugins[i].handle);
    }

    printf("Pipeline shutdown complete\n");
    return 0;
}