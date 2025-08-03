#!/bin/bash

echo "Building Operating Systems Final Project..."

# Create build directory if it doesn't exist
mkdir -p build

# Compile main application
gcc -Wall -Wextra -std=c99 -I. -c main.c -o build/main.o

# Compile plugin common files
gcc -Wall -Wextra -std=c99 -I. -c plugins/plugin_common.c -o build/plugin_common.o

# Compile monitor plugin
gcc -Wall -Wextra -std=c99 -I. -c plugins/sync/monitor.c -o build/monitor.o

# Compile sync plugin files
gcc -Wall -Wextra -std=c99 -I. -c plugins/sync/consumer_producer.c -o build/consumer_producer.o

# Link everything together
gcc -o build/final_project build/main.o build/plugin_common.o build/monitor.o build/consumer_producer.o -lpthread

echo "Build complete! Executable created at build/final_project" 