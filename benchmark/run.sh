#!/usr/bin/env bash

set -e

if ! command -v akx &> /dev/null; then
    echo "Error: 'akx' command not found in PATH"
    echo "Please ensure AKX is installed and available in your PATH"
    exit 1
fi

echo "=========================================="
echo "AKX Benchmark Suite"
echo "=========================================="
echo ""

BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

run_benchmark() {
    local name="$1"
    local file="$2"
    
    echo "Running: $name"
    echo "----------------------------------------"
    
    local start=$(date +%s%N)
    akx "$BENCHMARK_DIR/$file"
    local end=$(date +%s%N)
    
    local duration_ns=$((end - start))
    local duration_ms=$((duration_ns / 1000000))
    local duration_s=$((duration_ms / 1000))
    local remaining_ms=$((duration_ms % 1000))
    
    echo "----------------------------------------"
    echo "Time: ${duration_s}.$(printf "%03d" $remaining_ms)s"
    echo ""
}

run_benchmark "01. Fibonacci (Tail-Call Recursion)" "01_fibonacci_recursive.akx"
run_benchmark "02. Loop-Intensive Computation" "02_loop_intensive.akx"
run_benchmark "03. List Operations & Lambdas" "03_list_operations.akx"
run_benchmark "04. Filesystem I/O" "04_filesystem_io.akx"
run_benchmark "05. Forms System & Type Operations" "05_forms_system.akx"
run_benchmark "06. Collatz Conjecture Stress Test" "06_collatz_stress.akx"

echo "=========================================="
echo "All benchmarks completed"
echo "=========================================="

