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
if ./output/analyzer 0 logger 2>&1 | grep -q "error- not a valid queue size"; then
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
# additional tests

# test 8: uppercaser -> rotator -> logger
EXPECTED="[logger] OHELL"
ACTUAL=$(printf "hello\n<END>\n" | ./output/analyzer 20 uppercaser rotator logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "uppercaser + rotator + logger"
else
    print_error "uppercaser + rotator + logger (expected '$EXPECTED', got '$ACTUAL')"
fi

# test 9: exit code for missing args
set +e
printf "hello\n<END>\n" | ./output/analyzer >/dev/null 2>&1
if [ $? -ne 1 ]; then
    set -e
    print_error "exit code for missing args not handled"
fi
set -e
print_status "exit code for missing args handled"

# test 10: exit code for invalid plugin
set +e
printf "hello\n<END>\n" | ./output/analyzer 20 fakeplugin >/dev/null 2>&1
if [ $? -ne 1 ]; then
    set -e
    print_error "exit code for invalid plugin not handled"
fi
set -e
print_status "exit code for invalid plugin handled"

# test 11: exit code for invalid queue size
set +e
printf "<END>\n" | ./output/analyzer 0 logger >/dev/null 2>&1
if [ $? -ne 1 ]; then
    set -e
    print_error "exit code for invalid queue size not handled"
fi
set -e
print_status "exit code for invalid queue size handled"


# test 12: large 64 KiB payload through logger
payload=$(head -c 65536 /dev/zero | tr '\0' A)
logger_line=$(
  printf '%s\n<END>\n' "$payload" |
  ./output/analyzer 64 logger |
  sed $'s/\x1B\\[[0-9;]*[a-zA-Z]//g' |
  tail -n 1
)
msg=${logger_line#*] } ; msg=${msg#*] }
if [[ ${#msg} -eq 65536 ]] && [[ $(printf %s "$msg" | sha256sum) == $(printf %s "$payload" | sha256sum) ]]; then
    print_status "large 64KiB payload logger test"
else
    print_error "large 64KiB payload logger test failed"
fi


# test 13: high throughput (50,000 lines) logger
tmp=$(mktemp)
{
  for i in {1..50000}; do echo x; done
  echo '<END>'
} | ./output/analyzer 32 logger >"$tmp"
if (( $(grep -o "x" "$tmp" | wc -l) == 50000 )); then
    print_status "high throughput logger (50,000 lines)"
else
    print_error "high throughput logger failed"
fi
rm "$tmp"

# test 14: multiple sequential inputs through uppercaser + typewriter
OUT=$(printf 'a\nb\nc\nd\n<END>\n' | ./output/analyzer 1 uppercaser typewriter)
grep -q 'A' <<<"$OUT" && grep -q 'B' <<<"$OUT" && grep -q 'C' <<<"$OUT" && grep -q 'D' <<<"$OUT" && print_status "multiple sequential inputs (uppercaser + typewriter)" || print_error "multiple sequential inputs failed"