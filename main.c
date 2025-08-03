#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "plugins/plugin_common.h"
#include "plugins/plugin_sdk.h"

int main(int argc, char *argv[]) {
    printf("Operating Systems Final Project\n");
    printf("Main application starting...\n");
    
    // Initialize plugin system
    if (init_plugin_system() != 0) {
        fprintf(stderr, "Failed to initialize plugin system\n");
        return 1;
    }
    
    // Load and run plugins
    load_plugins();
    
    // Cleanup
    cleanup_plugin_system();
    
    return 0;
} 