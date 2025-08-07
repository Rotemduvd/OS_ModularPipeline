#!/bin/bash
set -e

# colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { echo -e "${GREEN}[PASS]${NC} $1"; }
print_error() { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
print_info() { echo " $1"; }

 # step 1: build project
echo " building project..."
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
echo " running positive tests..."

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
echo " running negative tests..."

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

echo " all tests passed successfully!"
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

# test 12: repeated 1024 'A's (64 times) = 64KiB
payload=""
for i in {1..64}; do payload="${payload}$(head -c 1024 < /dev/zero | tr '\0' A)\n"; done

ACTUAL=$(printf "${payload}<END>\n" | ./output/analyzer 64 logger | grep "\[logger\]" | wc -l)

if [ "$ACTUAL" -eq 64 ]; then
    print_status "64KiB logger test (split lines)"
else
    print_error "64KiB logger test failed: got $ACTUAL lines"
    exit 1
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

# test 15: all plugins chained together
OUT=$(echo -e "hello\n<END>" | ./output/analyzer 5 uppercaser rotator flipper expander typewriter logger | grep "\[logger\]")
if [[ "$OUT" =~ \[logger\]\ .* ]]; then
    print_status "full plugin chain (uppercaser -> rotator -> flipper -> expander -> typewriter -> logger)"
else
    print_error "full plugin chain failed (output: $OUT)"
fi

# test 16: whitespace-only input
EXPECTED="[logger]      "
ACTUAL=$(echo -e "     \n<END>" | ./output/analyzer 5 logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "whitespace-only input"
else
    print_error "whitespace-only input (expected '$EXPECTED', got '$ACTUAL')"
fi

# test 17: non-ASCII characters input
OUT=$(echo -e "héllö\n<END>" | ./output/analyzer 5 logger | grep "\[logger\]")
if [[ "$OUT" =~ \[logger\]\ .* ]]; then
    print_status "non-ASCII characters input"
else
    print_error "non-ASCII characters input failed"
fi

: << 'COMMENT_BLOCK'
# test 18: multiple <END> markers
OUT=$(echo -e "hello\n<END>\nworld\n<END>" | ./output/analyzer 5 uppercaser logger | grep "\[logger\]" | wc -l)
if [ "$OUT" -eq 2 ]; then
    print_status "multiple <END> markers handled"
else
    print_error "multiple <END> markers test failed (got $OUT lines)"
fi
COMMENT_BLOCK

# test 19: trailing blank lines
EXPECTED="[logger] HELLO"
ACTUAL=$(echo -e "hello\n\n\n<END>" | ./output/analyzer 5 uppercaser logger | grep "\[logger\]")
if [ "$ACTUAL" == "$EXPECTED" ]; then
    print_status "trailing blank lines handled"
else
    print_error "trailing blank lines test failed (expected '$EXPECTED', got '$ACTUAL')"
fi