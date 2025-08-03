#ifndef PLUGIN_SDK_H
#define PLUGIN_SDK_H

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>

// Plugin function pointer types
typedef int (*plugin_init_func)(void);
typedef int (*plugin_run_func)(void);
typedef void (*plugin_cleanup_func)(void);

// Plugin structure
typedef struct {
    void *handle;
    char *name;
    plugin_init_func init;
    plugin_run_func run;
    plugin_cleanup_func cleanup;
} plugin_t;

// Plugin system functions
int init_plugin_system(void);
void load_plugins(void);
void cleanup_plugin_system(void);

// Plugin registration macros
#define PLUGIN_INIT(name) int plugin_init(void)
#define PLUGIN_RUN(name) int plugin_run(void)
#define PLUGIN_CLEANUP(name) void plugin_cleanup(void)

#endif // PLUGIN_SDK_H 