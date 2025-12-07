# Performance Metrics for Timsort Benchmarking

## Overview

This document defines the quantitative performance metrics used to evaluate timsort algorithm performance across experimental dimensions: algorithm variants, RUN sizes, input sizes, data distributions, compiler optimizations, and machine architectures. All metrics are designed to be measurable on a single machine and suitable for CSV logging.

The benchmark measures performance of stable, in-memory sorting algorithms on large datasets. Metrics are selected to capture both raw performance and architectural characteristics related to memory hierarchy behavior and cost efficiency.

---

## Design Space and Primary Metric

### Primary Metric: Cost per GB

The primary scalar metric for comparing configurations across all experimental dimensions is **cost_per_GB**, defined as:

```
cost_per_GB = (hourly_cost / 3600) × (run_time_sec / data_size_GB)
```

Where:
- `hourly_cost` is the machine cost in dollars per hour (see Cost Model section)
- `run_time_sec` is the measured execution time in seconds
- `data_size_GB` is the input data size in gigabytes: `(n × sizeof(T)) / 10^9`

This metric is suitable for cross-dimensional comparison because:
1. It normalizes performance by data size, enabling fair comparison across different input sizes
2. It incorporates economic factors, making it relevant for deployment decisions
3. It provides a single scalar value that captures both speed and cost
4. It is directly measurable from benchmark timing data without requiring hardware counters

### Supporting Metric: Throughput

A secondary metric, **throughput_MB_s**, provides complementary information about raw performance:

```
throughput_MB_s = (data_size_MB) / run_time_sec
```

where `data_size_MB = (n × sizeof(T)) / 10^6`

Throughput is useful for understanding absolute performance characteristics and identifying performance bottlenecks, but cost_per_GB is preferred for final comparisons because it accounts for economic efficiency.

---

## Cost Model

### Primary Metric Definition

The cost model quantifies the economic cost of sorting operations. The primary metric is:

```
cost_per_GB = (hourly_cost / 3600) × (run_time_sec / data_size_GB)
```

### Cost Model Components

**hourly_cost**: Machine cost in dollars per hour. This value is chosen based on the experimental setup:

- For cloud instances: Use the published hourly rate (e.g., AWS EC2, CloudLab pricing)
- For personal hardware: Use amortized cost based on hardware purchase price, expected lifetime, and utilization, or estimate based on electricity costs
- For academic resources: Use equivalent market pricing if the resource is provided at no direct cost

Example values: $0.10/hour (personal laptop), $0.50/hour (cloud instance), $2.00/hour (high-end cloud instance).

**run_time_sec**: Execution time measured by the benchmark. The benchmark uses high-resolution timing (microsecond precision via `gettimeofday()` or equivalent). Multiple runs are averaged to reduce measurement variance. The timing includes only the sorting operation, excluding data generation and verification overhead.

**data_size_GB**: Input data size in gigabytes, computed as:

```
data_size_GB = (n × sizeof(T)) / 10^9
```

where `n` is the number of elements and `sizeof(T)` is the size of each element in bytes (e.g., 4 bytes for `uint32_t`).

### Secondary Economic Metric

An alternative economic metric, **throughput_per_dollar**, measures performance per unit cost:

```
throughput_per_dollar = throughput_MB_s / (hourly_cost / 3600)
```

This metric is inversely related to cost_per_GB and may be more intuitive for some analyses, but cost_per_GB is preferred as the primary metric because lower values directly indicate better economic efficiency.

---

## Metrics Definitions

The following table defines all performance metrics used in the benchmark. Metrics are categorized by measurement method: direct calculation from timing data, hardware performance counters, or post-processing of multiple runs.

| Name | Formula | How to Measure | Main Architectural Insight |
|------|---------|----------------|---------------------------|
| **Throughput** | `throughput_MB_s = (n × sizeof(T) / 10^6) / run_time_sec` | Direct calculation from timing | Raw data processing rate; sensitive to cache locality and memory bandwidth |
| **Cost per GB** | `cost_per_GB = (hourly_cost / 3600) × (run_time_sec / data_size_GB)` | Direct calculation from timing and cost model | Economic efficiency; combines performance and cost factors |
| **Memory Bandwidth Utilization** | `bandwidth_GB_s = (2 × n × sizeof(T) / 10^9) / run_time_sec` | Direct calculation (estimates 2× data movement) | Effective memory subsystem utilization; higher values indicate better memory efficiency |
| **Cache Efficiency** | `L1_hit_rate = 1 - (L1_misses / L1_loads)`<br>`LLC_miss_rate = LLC_misses / LLC_loads` | Hardware counters via `perf stat` (Linux) or Instruments (macOS) | Cache locality effectiveness; L1 hit rate directly reflects RUN size tuning effectiveness |
| **Instructions per Element** | `instructions_per_elem = total_instructions / n` | Hardware counters via `perf stat` | Algorithmic efficiency; separates compute from memory effects |
| **Scaling Efficiency** | `scaling_efficiency = (size₂ / size₁) / (time₂ / time₁)` | Post-processing of multiple runs at different sizes | Performance scaling with input size; identifies cache capacity boundaries |

### Metric Details

**Throughput (MB/s)**: Measures the rate of data processing. Higher values indicate faster algorithms. This metric is most sensitive to RUN size (cache locality), input size (working set effects), and machine architecture (memory bandwidth). Typical range: 100-2000 MB/s depending on algorithm and hardware.

**Cost per GB**: Primary comparison metric combining performance and economics. Lower values indicate better economic efficiency. Affected by machine cost structure, algorithm performance, and input size (amortization of fixed overhead). Typical range: $0.00001 - $0.0001 per GB.

**Memory Bandwidth Utilization (GB/s)**: Estimates effective memory bandwidth by assuming 2× data movement (read input, write output, plus temp buffer operations). Higher values indicate better memory subsystem utilization. Sensitive to RUN size (cache locality), input size (cache capacity), and system memory bandwidth. Typical range: 5-50 GB/s.

**Cache Efficiency**: Measures cache locality through hardware performance counters. L1 hit rate indicates effectiveness of RUN size tuning (optimal RUN fits in L1 cache). LLC miss rate indicates DRAM access frequency. Primary factor: RUN size (optimal RUN → high L1 hit rate). Secondary factors: input size (larger arrays → more misses), data distribution (random access → lower hit rates). Typical L1 hit rate: 0.85-0.99 (good), 0.5-0.7 (poor). Typical LLC miss rate: 0.01-0.1 (good), 0.3-0.5 (poor).

**Instructions per Element**: Average instructions executed per element sorted. Lower values indicate more efficient algorithms. Primarily affected by algorithm choice (O(n) vs O(n log n) complexity) and compiler optimization flags. Typical range: 20-200 instructions per element depending on algorithm.

**Scaling Efficiency**: Measures deviation from ideal linear scaling. Values near 1.0 indicate good scaling; values below 0.7 indicate performance degradation with size. Primarily affected by input size (performance cliffs at cache boundaries) and cache hierarchy characteristics. Computed by running benchmarks at multiple sizes (e.g., 1M, 10M, 100M, 256M elements) and comparing time ratios.

---

## Practical Measurement Workflow

### Direct Calculation Metrics

The following metrics are computed directly in the benchmark code from timing and size information:

1. **Throughput**: `throughput_MB_s = (size * sizeof(T) / 1e6) / avg_time_sec`
2. **Cost per GB**: `cost_per_GB = (hourly_cost / 3600.0) * (avg_time_sec / size_gb)`
3. **Memory Bandwidth Utilization**: `bandwidth_GB_s = (2.0 * size * sizeof(T) / 1e9) / avg_time_sec`

These are included in the CSV output from each benchmark run.

### Hardware Counter Metrics

Cache efficiency and instruction counts require external profiling tools:

**Linux (perf)**:
```bash
perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses,instructions,cycles \
    ./sorting_benchmark <size> <runs>
```

Compute ratios from perf output:
- `L1_hit_rate = 1.0 - (L1_misses / L1_loads)`
- `LLC_miss_rate = LLC_misses / LLC_loads`
- `instructions_per_element = total_instructions / size`

**macOS (Instruments)**: Use Time Profiler and System Trace to collect similar metrics.

Hardware counter data is collected separately and merged with timing data in post-processing.

### Post-Processing Metrics

**Scaling Efficiency**: Requires benchmark runs at multiple input sizes. For each size pair (S₁, S₂):

```
ideal_ratio = S₂ / S₁
actual_ratio = time(S₂) / time(S₁)
scaling_efficiency = ideal_ratio / actual_ratio
```

Alternatively, plot throughput vs input size on a log-log scale to visualize scaling behavior.

### CSV Output Format

The benchmark outputs CSV with the following columns:

```
algorithm,distribution,size,run_size,time_us,time_sec,throughput_MB_s,cost_per_GB
```

Additional columns for hardware counter metrics and computed values:

```
bandwidth_utilization_GB_s,L1_hit_rate,LLC_miss_rate,instructions_per_element,scaling_efficiency
```

### Measurement Best Practices

1. **Multiple runs**: Average timing over 3-5 runs to reduce variance
2. **Warmup**: Perform one untimed warmup run before measurement
3. **System state**: Ensure consistent system load (close unnecessary processes)
4. **Compiler flags**: Document optimization level (`-O3`, `-march=native`) as it affects all metrics
5. **Hardware counters**: Collect perf data separately to avoid measurement overhead
6. **Scaling tests**: Use powers of 10 for input sizes (1M, 10M, 100M, 256M) to clearly identify cache boundaries

---

## References

- Memory bandwidth: Typically 20-100 GB/s for modern systems
- L1 cache: 32-128 KB per core
- L2 cache: 256 KB - 1 MB per core
- L3 cache: 8-64 MB shared
- Cache line size: 64 bytes (standard)
