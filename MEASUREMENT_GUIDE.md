# Measurement & Benchmark Usage Guide (for Vincent and Rex)

**Maintainer:** Alice (measurement framework & analysis)  
**User:** Vincent and Rex (algorithm optimization & validation)

This document explains how to use the provided measurement framework, which commands to run, and which steps must be repeated after each algorithm change.

---

## 1. Components Already Provided (Do Not Modify)

The following components are implemented and verified. Do not change them.

| Component | Purpose |
|-----------|---------|
| `metrics.h` | Unified performance measurement (wall-clock time, throughput, memory placeholders) |
| `sorting_benchmark.c` | Main performance benchmark (outputs CSV) |
| `correctness_test.c` | Sorting correctness verification |
| `Measurements.md` | Measurement methodology and assumptions |
| `cost_per_GB` metric | Normalized cost metric for fair comparison |

These define the official evaluation pipeline.

---

## 2. One-Time Setup / Verification Steps

Compile and run the correctness test once, unless sorting semantics change.

```bash
gcc -O3 -Isrc \
  "src/Measurement and Testing/correctness_test.c" \
  src/timsort.c src/sorting.c \
  -o test_correctness

./test_correctness
```

Expected output:

```
[OK] Sorting correctness passed
```

If this message does not appear, benchmark results are invalid.

---

## 3. Mandatory Steps After Every Algorithm Change

Any change in algorithm logic requires rerunning the benchmark. This includes optimizing merge strategy, adjusting run size, changing comparison logic, or modifying `timsort.c` / `sorting.c`.

### Step 1: Recompile Benchmark

```bash
gcc -O3 src/sorting_benchmark.c -o sorting_benchmark
```

Recompile whenever algorithm-related code changes.

### Step 2: Run Benchmark

```bash
./sorting_benchmark <N> <R>
```

Recommended standard parameters:

```bash
./sorting_benchmark 1000000 3
```

| Parameter | Meaning |
|-----------|---------|
| `1000000` | Array size |
| `3` | Number of repeated runs per distribution |

### Step 3: Save Output (CSV)

The benchmark prints CSV-formatted data to stdout:

```
algorithm,distribution,size,time_sec,throughput_MB_s,memory_MB,cost_per_GB
timsort_run32,random_uniform,...
...
```

Save this output for spreadsheet comparison, Python analysis, and final report figures.

---

## 4. Key Metrics to Track (Used in Final Report)

| Metric | Meaning | Optimization Goal |
|--------|---------|-------------------|
| `time_sec` | Wall-clock runtime | Lower is better |
| `throughput_MB_s` | CPU efficiency | Higher is better |
| `cost_per_GB` | Primary comparison metric | Lowest wins |

All final conclusions will be based on `cost_per_GB`.

---

## 5. When to Rerun Correctness Test

- No need to rerun if only performance optimizations are applied and sorting semantics are unchanged.
- Must rerun if comparator behavior changes, data type definitions change, or the sorting interface/API changes.

---

## 6. One-Line Workflow Summary (for Vincent and Rex)

Modify algorithm → recompile benchmark → run benchmark → save CSV  
Run correctness test only when sorting semantics may change.

---

## 7. Alice’s Next Steps (Context)

- Measurement framework completed.  
- Analyze performance sensitivity across run sizes, data distributions, and algorithm variants.  
- Generate plots and write final analysis report.

