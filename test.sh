#!/bin/bash
set -e

# colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[PASS]${NC} $1"; }
print_error() { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
print_info() { echo -e "${YELLOW}[INFO]${NC} $1"; }

# step 1: build project
print_info "building project..."
./build.sh

# helper to run a test and compare expected vs actual
run_test() {
    local description="$1"
    local expected="$2"
    local command="$3"
    local actual=$(eval "$command")

    if [ "$actual" == "$expected" ]; then
        print_status "$description"
    else
        print_error "$description (expected '$expected', got '$actual')"
    fi
}

# positive tests
print_info "running positive tests..."

# test 1: uppercaser -> logger
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "uppercaser + logger"
else
    print_error "uppercaser + logger (expected '$EXPECTED', got '$ACTUAL')"
fi

# test 2: uppercaser -> flipper -> logger
EXPECTED="[logger] OLLEH"
ACTUAL=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser flipper logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "uppercaser + flipper + logger"
else
    print_error "uppercaser + flipper + logger (expected '$EXPECTED', got '$ACTUAL')"
fi

# test 3: empty string
EXPECTED="[logger] "
ACTUAL=$(echo -e "\n<END>" | ./output/analyzer 10 logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "empty string test"
else
    print_error "empty string test (expected '$EXPECTED', got '$ACTUAL')"
fi

# test 4: long string
LONG=$(printf 'a%.0s' {1..1000})
EXPECTED="[logger] ${LONG^^}" # uppercase version
ACTUAL=$(echo -e "$LONG\n<END>" | ./output/analyzer 10 uppercaser logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "long string (1000 chars) uppercaser"
else
    print_error "long string test failed"
fi

# negative tests
print_info "running negative tests..."

# test 5: missing args
if ./output/analyzer 2>&1 | grep -q "Usage:"; then
    print_status "missing arguments shows usage"
else
    print_error "missing arguments did not show usage"
fi

# test 6: invalid queue size
if ./output/analyzer 0 logger 2>&1 | grep -q "invalid queue size"; then
    print_status "invalid queue size handled"
else
    print_error "invalid queue size not handled"
fi

# test 7: invalid plugin name
if ./output/analyzer 10 fakeplugin 2>&1 | grep -q "failed to load"; then
    print_status "invalid plugin name handled"
else
    print_error "invalid plugin name not handled"
fi

print_info "all tests passed successfully!"