# Cost Model for Sorting Benchmark Analysis

## Mathematical Definition

The cost per gigabyte sorted is defined as:

```
cost_per_GB = (C_hour / 3600) × (T_sec / D_GB)
```

where:
- `C_hour` is the machine cost in dollars per hour (constant)
- `T_sec` is the measured execution time in seconds (variable)
- `D_GB` is the dataset size in gigabytes (constant per experiment)
- `3600` is the conversion factor from hours to seconds (constant)

This can be equivalently expressed as:

```
cost_per_GB = (C_hour × T_sec) / (3600 × D_GB)
```

The numerator represents the cost of compute time used, and the denominator normalizes by data volume.

---

## Variable Definitions and Assumptions

### Constants

**C_hour (Machine Hourly Cost)**: Fixed for each experimental platform. This value represents the economic cost of operating the machine for one hour. Selection criteria:

- **Cloud instances**: Use published on-demand hourly rates (e.g., AWS EC2, Google Cloud, Azure)
- **Personal hardware**: Compute amortized cost as `(purchase_price / expected_lifetime_hours)` or use electricity cost estimate: `(power_W × hours × electricity_rate_$/kWh / 1000)`
- **Academic resources**: Use equivalent market pricing if provided at no direct cost to maintain comparability

Example values: $0.10/hour (personal laptop, electricity-based), $0.50/hour (mid-tier cloud instance), $2.00/hour (high-performance cloud instance).

**D_GB (Dataset Size)**: Fixed per experimental run. Computed as:

```
D_GB = (n × sizeof(T)) / 10^9
```

where `n` is the number of elements and `sizeof(T)` is the element size in bytes (e.g., 4 bytes for `uint32_t`, 8 bytes for `uint64_t`).

**3600**: Conversion constant (seconds per hour).

### Measured Variables

**T_sec (Execution Time)**: Measured for each algorithm configuration. Measurement methodology:

- High-resolution timing using `gettimeofday()` or equivalent (microsecond precision)
- Multiple runs (typically 3-5) with arithmetic mean to reduce variance
- Includes only sorting operation time (excludes data generation, memory allocation, and verification)
- Warmup run performed before measurement to account for cache effects

The timing measurement captures the wall-clock execution time of the sorting algorithm, which directly reflects the compute resources consumed.

---

## Mapping Runtime to Cost per GB

The transformation from measured runtime to cost per GB proceeds as follows:

1. **Measure execution time**: `T_sec` seconds for sorting `n` elements
2. **Compute dataset size**: `D_GB = (n × sizeof(T)) / 10^9` gigabytes
3. **Calculate compute cost**: `cost_compute = C_hour × (T_sec / 3600)` dollars
4. **Normalize by data volume**: `cost_per_GB = cost_compute / D_GB` dollars per GB

The final metric `cost_per_GB` has units of dollars per gigabyte sorted, enabling direct comparison across:
- Different algorithms (lower is better)
- Different input sizes (should be approximately constant for scalable algorithms)
- Different machine architectures (reflects both performance and pricing differences)
- Different compiler optimizations (shows optimization impact on economic efficiency)

---

## Cost Analysis Section Text

The following paragraph is suitable for inclusion in a technical report:

> **Cost Analysis**: We evaluate economic efficiency using a cost-per-gigabyte metric that combines measured execution time with machine pricing. The cost model is defined as `cost_per_GB = (C_hour / 3600) × (T_sec / D_GB)`, where `C_hour` is the machine hourly cost in dollars, `T_sec` is the measured execution time in seconds, and `D_GB` is the dataset size in gigabytes. Machine costs are set based on platform type: cloud instances use published hourly rates, while personal hardware uses amortized purchase cost or electricity-based estimates. Execution time is measured using high-resolution timers with multiple runs averaged to reduce variance. The metric normalizes performance by data volume, enabling fair comparison across different input sizes and algorithm configurations. Lower values indicate better economic efficiency, directly quantifying the cost of sorting operations for deployment decisions.

---

## Implementation Notes

### Fixed Constants vs Measured Variables

**Fixed per experimental setup:**
- `C_hour`: Determined once per machine/platform
- `D_GB`: Fixed per experimental run (varies across runs but constant within a run)

**Measured per algorithm configuration:**
- `T_sec`: Measured for each (algorithm, distribution, size, RUN size) combination

### Reproducibility

To ensure reproducibility:
1. Document `C_hour` value and justification for each platform
2. Report `T_sec` as mean ± standard deviation across multiple runs
3. Specify `D_GB` calculation (element count and type)
4. Note compiler flags and optimization level (affects `T_sec`)
5. Report system specifications (CPU, memory, cache hierarchy)

### Typical Value Ranges

- **cost_per_GB**: $0.00001 - $0.0001 per GB (depends on machine cost and algorithm performance)
- **T_sec**: 0.1 - 100 seconds (depends on dataset size and algorithm)
- **D_GB**: 0.001 - 10 GB (typical experimental range)

---

## Calculation and Result (To be completed)
### Example calculation
Given:
- Machine: Cloud instance at $0.50/hour
- Dataset: 64 million `uint32_t` elements = 256 MB = 0.256 GB
- Execution time: 2.5 seconds (averaged over 5 runs)

Calculation:
```
C_hour = 0.50 $/hour
T_sec = 2.5 seconds
D_GB = (64 × 10^6 × 4 bytes) / 10^9 = 0.256 GB

cost_per_GB = (0.50 / 3600) × (2.5 / 0.256)
            = 0.0001389 × 9.7656
            = 0.001357 $/GB
            ≈ $0.00136 per GB
```

This indicates that sorting 1 GB of data on this configuration costs approximately 0.136 cents.

