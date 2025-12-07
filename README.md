# Sorting Optimization Guide

## 4 Optimizations Explained

### Optimization 1: RUN Size Tuning (Cache Locality)

**What it does:**
Timsort first sorts small "runs" with insertion sort, then merges them. The RUN size determines how much data stays in L1 cache during insertion sort.

**Why it matters:**
- L1 cache access: ~4 cycles
- L2 cache access: ~12 cycles  
- L3 cache access: ~40 cycles
- DRAM access: ~200 cycles

If our RUN fits in L1 cache, insertion sort is blazing fast. If it spills to L2/L3, you lose performance.

**Our machines' L1 cache sizes:**
| Machine | L1d Cache | Max RUN (uint32_t) | Suggested RUN |
|---------|-----------|--------------------| --------------|
| EPYC 9354P | 32 KB | 8,192 | 64-256 |
| Apple M4 | 128 KB | 32,768 | 256-512 |
| Ryzen AI 9 | 48 KB | 12,288 | 128-256 |

**Experiment to run:**
Compare timsort_run32, timsort_run64, timsort_run128, timsort_run256 across all 3 machines.

**What to look for:**
- On EPYC: performance should drop after RUN=256 (exceeds L1)
- On M4: larger RUNs should still be fast (bigger L1)
- Graph: X-axis = RUN size, Y-axis = throughput (MB/s)

---

### Optimization 2: Software Prefetching

**What it does:**
During merge, we tell the CPU to start loading data we'll need soon:
```c
__builtin_prefetch(&arr[i + 16], 0, 3);  // Load arr[i+16] into cache
```

**Why it matters:**
Memory access has ~200 cycle latency. If we tell the CPU to start loading data 16 elements ahead, by the time we need it, it's already in cache.

**Experiment to run:**
Compare `timsort_run64` vs `timsort_pf_run64` (prefetch version)

**What to look for:**
- Bigger improvement on systems with higher memory latency
- EPYC (server memory) may benefit more than M4 (unified memory)
- Use `perf stat` on Linux to measure cache miss rates

---

### Optimization 3: Radix Sort

**What it does:**
Instead of comparing elements, radix sort groups by digit value:
```
Pass 1: Group by least significant byte (0-255)
Pass 2: Group by next byte
Pass 3: Group by next byte  
Pass 4: Group by most significant byte
Result: Sorted!
```

**Why it matters:**
- Comparison sorts: O(n log n) comparisons
- Radix sort: O(n × k) where k = 4 bytes = 4 passes
- For large n, radix wins (no branches, very cache-friendly)

**Stability:**
LSD (Least Significant Digit) radix sort IS stable because it processes from right to left and preserves relative order.

**Experiment to run:**
Compare `timsort_run64` vs `radix_lsd` across different data distributions.

**What to look for:**
- Radix should dominate on uniform random data
- Timsort may win on nearly-sorted data (adaptive)
- Radix has consistent performance regardless of distribution

---

### Optimization 4: Cross-Architecture Analysis

Run the SAME benchmark on all 3 and compare.

**Key metrics to collect:**
1. **Time per GB** (raw speed)
2. **Cost per GB** (time × hourly cost)
3. **Cache miss rate** (why is it faster/slower)
4. **Optimal RUN size** (different per machine)

**Pricing estimates:**
| Machine | Est. $/hour | Notes |
|---------|-------------|-------|
| CloudLab EPYC | ~$0.50 | Free for academic use, but calculate equivalent |
| M4 Pro MacBook | ~$0.15 | Electricity only (~45W × $0.15/kWh) |
| G14 Laptop | ~$0.10 | Electricity only (~35W × $0.15/kWh) |

---

## How to Run the Experiments

### Step 1: Compile
```bash
# On Linux (CloudLab, G14)
gcc -O3 -march=native -o sorting_benchmark sorting_benchmark.c

# On macOS (M4)
clang -O3 -mcpu=native -o sorting_benchmark sorting_benchmark.c
```

### Step 2: Run scaling test
```bash
# Test with 64M elements (~256MB)
./sorting_benchmark 67108864 3

# Test with 256M elements (~1GB) 
./sorting_benchmark 268435456 3
```

### Step 3: Run with perf (Linux only)
```bash
perf stat -e cycles,instructions,cache-misses,L1-dcache-load-misses \
    ./sorting_benchmark 67108864 1
```

### Step 4: Use the runner script
```bash
chmod +x run_benchmark.sh
./run_benchmark.sh
```

---

## What Graphs to Make for Your Report

### Graph 1: RUN Size vs Throughput (per machine)
```
Throughput (MB/s)
     ^
     |     ___________
     |    /           \
     |   /             \___
     |  /
     | /
     +-------------------------> RUN Size
       32   64  128  256  512
       
Caption: "Optimal RUN size varies by L1 cache size"
```

### Graph 2: Algorithm Comparison (bar chart)
```
Time (seconds)
     ^
     |  ████
     |  ████  ████
     |  ████  ████  ████
     |  ████  ████  ████  ████
     +---------------------------
       Tim64 Tim128 Radix Radix+PF
       
Caption: "Radix sort outperforms Timsort on random data"
```

### Graph 3: Cross-Architecture Comparison
```
Cost per GB ($)
     ^
     |  ████
     |  ████  ████
     |  ████  ████  ████
     +---------------------------
       EPYC   M4    Ryzen
       
Caption: "Cost efficiency varies by architecture"
```

### Graph 4: Distribution Sensitivity
```
Show how each algorithm performs on:
- Random uniform
- Nearly sorted  
- Reverse sorted
- Few unique values

Caption: "Radix has consistent performance; Timsort adapts to input"
```

---

## Sample Report Section (your contribution)

```markdown
## Cache Optimization

We tuned the Timsort RUN parameter based on L1 cache size. Table X shows
the L1 data cache sizes for our test systems.

[TABLE: Machine, L1d size, Optimal RUN]

Figure Y shows throughput vs RUN size on the EPYC 9354P. Performance 
peaks at RUN=128, corresponding to 512 bytes, which fits comfortably 
in the 32KB L1 cache with room for the merge buffer.

We added software prefetching to the merge phase, improving throughput
by X% on the EPYC system. The improvement was smaller on the M4 Pro 
(Y%) due to its lower memory latency from unified memory architecture.

## Radix Sort Comparison

For uniformly distributed uint32_t data, radix sort achieved 
X MB/s compared to Timsort's Y MB/s, a Z% improvement. However,
on nearly-sorted data, Timsort's adaptive behavior provided better
performance (see Figure W).
```

---

## Files Provided

| File | Purpose |
|------|---------|
| `sorting_benchmark.c` | Main benchmark with all optimizations |
| `run_benchmark.sh` | Automated test runner |
| `README.md` | This guide |

## Next Steps

1. Copy files to all 3 machines
2. Run `./run_benchmark.sh` on each
3. Collect CSV results
4. Make graphs in Python/Excel
5. Merge with your partner's parallelism results
6. Write report