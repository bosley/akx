#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
AKX_BINARY="$PROJECT_ROOT/build/bin/akx"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ ! -f "$AKX_BINARY" ]; then
    echo -e "${RED}Error: AKX binary not found at $AKX_BINARY${NC}"
    echo "Please build the project first: make"
    exit 1
fi

TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

echo -e "${BLUE}=== AKX Core Test Suite ===${NC}"
echo "Binary: $AKX_BINARY"
echo "Tests directory: $SCRIPT_DIR"
echo ""

for test_file in "$SCRIPT_DIR"/*.akx; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    test_name=$(basename "$test_file" .akx)
    expect_file="${test_file%.akx}.expect"
    
    if [ ! -f "$expect_file" ]; then
        echo -e "${YELLOW}⊘ SKIP${NC} $test_name (no .expect file)"
        ((TESTS_SKIPPED++))
        continue
    fi
    
    echo -n "Testing $test_name... "
    
    actual_output=$(mktemp)
    expected_output=$(mktemp)
    raw_output=$(mktemp)
    
    if (cd "$PROJECT_ROOT" && "$AKX_BINARY" "$test_file") > "$raw_output" 2>&1; then
        exit_code=0
    else
        exit_code=$?
    fi
    
    grep -v "^AK24 Info: Removing stale lock file" "$raw_output" | \
    grep -v "ERROR.*Maximum recursion depth exceeded" | \
    grep -v "ERROR.*Failed to add root source to CJIT unit" | \
    grep -v "^[0-9][0-9]:[0-9][0-9]:[0-9][0-9].*ERROR.*Runtime error:" > "$actual_output" || true
    
    sed 's/<any>/.*/' "$expect_file" > "$expected_output"
    
    rm -f "$raw_output"
    
    if grep -q '<any>' "$expect_file"; then
        if grep -Ezq "$(cat "$expected_output")" "$actual_output"; then
            match=true
        else
            match=false
        fi
    else
        if diff -u "$expected_output" "$actual_output" > /dev/null 2>&1; then
            match=true
        else
            match=false
        fi
    fi
    
    if [ "$match" = "true" ]; then
        echo -e "${GREEN}✓ PASS${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo ""
        echo -e "${YELLOW}Expected output:${NC}"
        cat "$expected_output"
        echo ""
        echo -e "${YELLOW}Actual output:${NC}"
        cat "$actual_output"
        echo ""
        echo -e "${YELLOW}Diff:${NC}"
        diff -u "$expected_output" "$actual_output" || true
        echo ""
        ((TESTS_FAILED++))
    fi
    
    rm -f "$actual_output" "$expected_output"
done

echo ""
echo -e "${BLUE}=== Test Summary ===${NC}"
echo -e "${GREEN}Passed:${NC}  $TESTS_PASSED"
echo -e "${RED}Failed:${NC}  $TESTS_FAILED"
echo -e "${YELLOW}Skipped:${NC} $TESTS_SKIPPED"
echo ""

if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi

