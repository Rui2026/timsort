#!/bin/bash
# =============================================================================
# Sorting Benchmark Runner
# Run this on each machine to collect performance data
# =============================================================================

# Output directory
OUTDIR="results_$(hostname)_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTDIR"

# Compiler flags to test (coordinate with your partner on this)
# For now, use -O3 as baseline
CC=gcc
CFLAGS="-O3 -march=native -Wall"

echo "=== Building benchmark ==="
echo "Compiler: $CC"
echo "Flags: $CFLAGS"
$CC $CFLAGS -o sorting_benchmark sorting_benchmark.c -lm
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

# =============================================================================
# EXPERIMENT 1: Scaling test (different sizes)
# Purpose: Show cost/GB is consistent across sizes, find optimal operating point
# =============================================================================
echo ""
echo "=== Experiment 1: Scaling Test ==="

for SIZE in 1000000 10000000 100000000 268435456; do
    echo "Testing size: $SIZE elements"
    ./sorting_benchmark $SIZE 3 >> "$OUTDIR/scaling_test.csv"
done

# =============================================================================
# EXPERIMENT 2: Cache behavior analysis (using perf on Linux)
# Purpose: Measure L1/L2/L3 cache misses for different RUN sizes
# =============================================================================
echo ""
echo "=== Experiment 2: Cache Analysis ==="

# Check if perf is available (Linux only)
if command -v perf &> /dev/null; then
    echo "Running perf analysis..."
    
    # Key performance counters for cache analysis
    PERF_EVENTS="cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses"
    
    # Run with perf stat
    perf stat -e $PERF_EVENTS -o "$OUTDIR/perf_stats.txt" \
        ./sorting_benchmark 67108864 1 > "$OUTDIR/perf_output.csv" 2>&1
    
    echo "Perf results saved to $OUTDIR/perf_stats.txt"
else
    echo "perf not available (Linux only). Skipping cache analysis."
    echo "On macOS, use Instruments instead."
fi

# =============================================================================
# EXPERIMENT 3: Collect system info
# Purpose: Document the hardware for cross-architecture comparison
# =============================================================================
echo ""
echo "=== Collecting System Info ==="

{
    echo "=== System Information ==="
    echo "Hostname: $(hostname)"
    echo "Date: $(date)"
    echo ""
    
    echo "=== CPU Info ==="
    if [ -f /proc/cpuinfo ]; then
        # Linux
        grep -m1 "model name" /proc/cpuinfo
        grep -m1 "cpu MHz" /proc/cpuinfo
        echo "Cores: $(grep -c ^processor /proc/cpuinfo)"
        grep -m1 "cache size" /proc/cpuinfo
    elif command -v sysctl &> /dev/null; then
        # macOS
        sysctl -n machdep.cpu.brand_string
        echo "Cores: $(sysctl -n hw.ncpu)"
        echo "L1 Cache: $(sysctl -n hw.l1dcachesize 2>/dev/null || echo 'N/A') bytes"
        echo "L2 Cache: $(sysctl -n hw.l2cachesize 2>/dev/null || echo 'N/A') bytes"
        echo "L3 Cache: $(sysctl -n hw.l3cachesize 2>/dev/null || echo 'N/A') bytes"
    fi
    echo ""
    
    echo "=== Memory Info ==="
    if [ -f /proc/meminfo ]; then
        grep MemTotal /proc/meminfo
    elif command -v sysctl &> /dev/null; then
        echo "Total Memory: $(($(sysctl -n hw.memsize) / 1024 / 1024 / 1024)) GB"
    fi
    echo ""
    
    echo "=== Compiler Version ==="
    $CC --version | head -1
    
} > "$OUTDIR/system_info.txt"

cat "$OUTDIR/system_info.txt"

# =============================================================================
# EXPERIMENT 4: Individual algorithm deep-dive
# Purpose: Get detailed timing for each algorithm variant
# =============================================================================
echo ""
echo "=== Experiment 4: Detailed Benchmark (1GB test) ==="
echo "This may take several minutes..."

# 1GB = 268435456 uint32_t elements
./sorting_benchmark 268435456 5 > "$OUTDIR/full_benchmark_1GB.csv"

echo ""
echo "=== All results saved to $OUTDIR/ ==="
echo ""
echo "Files generated:"
ls -la "$OUTDIR/"