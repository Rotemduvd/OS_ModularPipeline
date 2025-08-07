#!/bin/bash
set -e

# colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() {
     echo -e "${GREEN}[BUILD]${NC} $1"; 
     }
print_error() {
     echo -e "${RED}[ERROR]${NC} $1";
      }
print_warning() {
     echo -e "${YELLOW}[WARNING]${NC} $1";
      }

# create output dirs
mkdir -p output/plugins

print_status "compiling the sync files"
gcc -fPIC -c plugins/sync/monitor.c -o output/monitor.o
gcc -fPIC -c plugins/sync/consumer_producer.c -o output/consumer_producer.o

print_status "compiling plugin common"
gcc -fPIC -c plugins/plugin_common.c -o output/plugin_common.o

# build plugins as .so
for plugin in logger uppercaser flipper rotator expander typewriter; do
    print_status "building plugin: $plugin"
    gcc -fPIC -shared plugins/$plugin.c output/plugin_common.o output/consumer_producer.o output/monitor.o -o output/plugins/$plugin.so -lpthread -ldl
done

# build main app
print_status "building main application..."
gcc main.c output/consumer_producer.o output/monitor.o -ldl -lpthread -o output/analyzer

print_status "build complete!"
echo "run with: ./output/analyzer <queue_size> <plugins...>"