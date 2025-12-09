#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdalign.h>

// ============================================================================
// CONFIGURATION - Adjust these for experiments
// ============================================================================

typedef uint32_t T;  // Data type: uint32_t or float

// RUN sizes to test (tune based on L1 cache size)
// EPYC 9354P: 32KB L1d per core -> ~8192 uint32_t
// Apple M4: 128KB L1d per core -> ~32768 uint32_t  
// Ryzen AI 9: 48KB L1d per core -> ~12288 uint32_t
#define RUN_SMALL   32
#define RUN_MEDIUM  64
#define RUN_LARGE   128
#define RUN_XLARGE  256
#define RUN_CACHE   512  // For cache-line aligned experiments

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// High-resolution timer (microseconds)
static inline double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1e6 + tv.tv_usec;
}

// Compare function for stability
static inline int cmp_le(T a, T b) {
    return a <= b;
}

// Verify sort correctness and stability
static int verify_sorted(T *arr, size_t size) {
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) {
            printf("ERROR: arr[%zu]=%u < arr[%zu]=%u\n", i, arr[i], i-1, arr[i-1]);
            return 0;
        }
    }
    return 1;
}

// Generate test data with different distributions
typedef enum {
    DIST_RANDOM_UNIFORM,
    DIST_RANDOM_NORMAL,
    DIST_SORTED,
    DIST_REVERSE_SORTED,
    DIST_NEARLY_SORTED,
    DIST_FEW_UNIQUE
} Distribution;

static void generate_data(T *arr, size_t size, Distribution dist, unsigned seed) {
    srand(seed);
    switch (dist) {
        case DIST_RANDOM_UNIFORM:
            for (size_t i = 0; i < size; i++) {
                arr[i] = (T)rand();
            }
            break;
        case DIST_SORTED:
            for (size_t i = 0; i < size; i++) {
                arr[i] = (T)i;
            }
            break;
        case DIST_REVERSE_SORTED:
            for (size_t i = 0; i < size; i++) {
                arr[i] = (T)(size - i);
            }
            break;
        case DIST_NEARLY_SORTED:
            for (size_t i = 0; i < size; i++) {
                arr[i] = (T)i;
            }
            // Swap ~1% of elements
            for (size_t i = 0; i < size / 100; i++) {
                size_t a = rand() % size;
                size_t b = rand() % size;
                T tmp = arr[a];
                arr[a] = arr[b];
                arr[b] = tmp;
            }
            break;
        case DIST_FEW_UNIQUE:
            for (size_t i = 0; i < size; i++) {
                arr[i] = (T)(rand() % 100);  // Only 100 unique values
            }
            break;
        default:
            for (size_t i = 0; i < size; i++) {
                arr[i] = (T)rand();
            }
    }
}

static const char* dist_name(Distribution d) {
    switch (d) {
        case DIST_RANDOM_UNIFORM: return "random_uniform";
        case DIST_SORTED: return "sorted";
        case DIST_REVERSE_SORTED: return "reverse_sorted";
        case DIST_NEARLY_SORTED: return "nearly_sorted";
        case DIST_FEW_UNIQUE: return "few_unique";
        default: return "unknown";
    }
}

// ============================================================================
// OPTIMIZATION 1: TIMSORT WITH CONFIGURABLE RUN SIZE
// This tests cache locality - different RUN sizes perform differently
// on different cache hierarchies
// ============================================================================

static void insertion_sort_range(T *arr, size_t left, size_t right) {
    for (size_t i = left + 1; i <= right; i++) {
        T temp = arr[i];
        size_t j = i;
        while (j > left && cmp_le(temp, arr[j - 1])) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = temp;
    }
}

static void merge(T *arr, size_t left, size_t mid, size_t right, T *temp) {
    size_t i = left, j = mid + 1, k = left;
    
    while (i <= mid && j <= right) {
        if (cmp_le(arr[i], arr[j])) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];
    
    memcpy(&arr[left], &temp[left], (right - left + 1) * sizeof(T));
}

// Timsort with configurable RUN size
static void timsort_with_run(T *arr, size_t size, size_t run_size, T *temp) {
    if (size <= 1) return;
    
    // Step 1: Sort small runs with insertion sort
    for (size_t i = 0; i < size; i += run_size) {
        size_t right = (i + run_size - 1 < size - 1) ? i + run_size - 1 : size - 1;
        insertion_sort_range(arr, i, right);
    }
    
    // Step 2: Merge runs
    for (size_t curr_size = run_size; curr_size < size; curr_size *= 2) {
        for (size_t left = 0; left < size; left += 2 * curr_size) {
            size_t mid = left + curr_size - 1;
            if (mid >= size - 1) break;
            size_t right = (left + 2 * curr_size - 1 < size - 1) ? 
                           left + 2 * curr_size - 1 : size - 1;
            merge(arr, left, mid, right, temp);
        }
    }
}


// ============================================================================
// OPTIMIZATION 2: CACHE-OPTIMIZED MERGE WITH PREFETCHING
// Prefetch data into cache before it's needed
// ============================================================================

#ifdef __GNUC__
#define PREFETCH(addr) __builtin_prefetch(addr, 0, 3)
#else
#define PREFETCH(addr) ((void)0)
#endif

static void merge_prefetch(T *arr, size_t left, size_t mid, size_t right, T *temp) {
    size_t i = left, j = mid + 1, k = left;
    
    // Prefetch distance (in elements) - tune based on memory latency
    const size_t PREFETCH_DIST = 16;
    
    while (i <= mid && j <= right) {
        // Prefetch ahead
        if (i + PREFETCH_DIST <= mid) PREFETCH(&arr[i + PREFETCH_DIST]);
        if (j + PREFETCH_DIST <= right) PREFETCH(&arr[j + PREFETCH_DIST]);
        
        if (cmp_le(arr[i], arr[j])) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];
    
    memcpy(&arr[left], &temp[left], (right - left + 1) * sizeof(T));
}

static void timsort_prefetch(T *arr, size_t size, size_t run_size, T *temp) {
    if (size <= 1) return;
    
    for (size_t i = 0; i < size; i += run_size) {
        size_t right = (i + run_size - 1 < size - 1) ? i + run_size - 1 : size - 1;
        insertion_sort_range(arr, i, right);
    }
    
    for (size_t curr_size = run_size; curr_size < size; curr_size *= 2) {
        for (size_t left = 0; left < size; left += 2 * curr_size) {
            size_t mid = left + curr_size - 1;
            if (mid >= size - 1) break;
            size_t right = (left + 2 * curr_size - 1 < size - 1) ? 
                           left + 2 * curr_size - 1 : size - 1;
            merge_prefetch(arr, left, mid, right, temp);
        }
    }
}

// ===========================
// Pairwise merge round (single-thread)
// Merge adjacent sorted blocks into temp, then copy back to arr.
// Place this after merge/merge_prefetch definitions.
// ===========================
static void pairwise_merge_round(T* arr, T* temp,
                                 size_t nblocks,
                                 const size_t* starts,
                                 const size_t* ends) {
    if (nblocks == 0) return;
    size_t npairs = nblocks / 2;

    for (size_t p = 0; p < npairs; p++) {
        size_t left  = starts[2*p];
        size_t mid   = ends[2*p];
        size_t right = ends[2*p + 1];
        // Use existing merge (or switch to merge_prefetch if desired)
        merge(arr, left, mid, right, temp);
        // Alternative:
        // merge_prefetch(arr, left, mid, right, temp);
    }

    if (nblocks % 2 == 1) {
        size_t left  = starts[nblocks - 1];
        size_t right = ends[nblocks - 1];
        memcpy(&temp[left], &arr[left], (right - left + 1) * sizeof(T));
    }

    size_t total_right = ends[nblocks - 1];
    memcpy(arr, temp, (total_right + 1) * sizeof(T));
}


// ============================================================================
// OPTIMIZATION 3: RADIX SORT (LSD - Least Significant Digit)
// Non-comparison sort - O(n*k) where k = number of digits
// Very cache-friendly with counting sort per digit
// STABLE: processes digits from least to most significant
// ============================================================================

#define RADIX_BITS 8
#define RADIX_SIZE (1 << RADIX_BITS)  // 256 buckets
#define RADIX_MASK (RADIX_SIZE - 1)

static void radix_sort_lsd(T *arr, size_t size, T *temp) {
    if (size <= 1) return;
    
    size_t count[RADIX_SIZE];
    
    // Process each byte (4 passes for uint32_t)
    for (int shift = 0; shift < (int)(sizeof(T) * 8); shift += RADIX_BITS) {
        // Count occurrences
        memset(count, 0, sizeof(count));
        for (size_t i = 0; i < size; i++) {
            size_t digit = (arr[i] >> shift) & RADIX_MASK;
            count[digit]++;
        }
        
        // Compute prefix sums (cumulative count)
        for (size_t i = 1; i < RADIX_SIZE; i++) {
            count[i] += count[i - 1];
        }
        
        // Place elements in sorted order (iterate backwards for stability)
        for (size_t i = size; i > 0; i--) {
            size_t digit = (arr[i-1] >> shift) & RADIX_MASK;
            temp[--count[digit]] = arr[i-1];
        }
        
        // Copy back
        memcpy(arr, temp, size * sizeof(T));
    }
}

// ============================================================================
// OPTIMIZATION 4: HYBRID RADIX + INSERTION SORT
// Use radix for large arrays, insertion sort for small buckets
// ============================================================================

static void radix_sort_hybrid(T *arr, size_t size, T *temp) {
    if (size <= 64) {
        insertion_sort_range(arr, 0, size - 1);
        return;
    }
    radix_sort_lsd(arr, size, temp);
}

// ============================================================================
// BENCHMARK INFRASTRUCTURE
// ============================================================================

typedef struct {
T* arr;
size_t left;
size_t right;
T* temp_local;      
size_t run_param; 
void (*func)(T*, size_t, size_t, T*);
alignas(64) char pad[64];            
} ThreadTask;

static void* thread_sort_entry(void* arg) {
ThreadTask* t = (ThreadTask*)arg;
if (t->right < t->left) return NULL;
size_t size = t->right - t->left + 1; 
if (size > 1) {
t->func(t->arr + t->left, size, t->run_param, t->temp_local);
}
return NULL;
}

typedef void (*sort_func_t)(T *arr, size_t size, size_t param, T *temp);

typedef struct {
    const char *name;
    sort_func_t func;
    size_t param;  // e.g., RUN size
} SortAlgorithm;

// Wrapper functions for uniform interface
static void wrap_timsort(T *arr, size_t size, size_t run, T *temp) {
    timsort_with_run(arr, size, run, temp);
}

static void wrap_timsort_prefetch(T *arr, size_t size, size_t run, T *temp) {
    timsort_prefetch(arr, size, run, temp);
}

static void wrap_radix(T *arr, size_t size, size_t unused, T *temp) {
    (void)unused;
    radix_sort_lsd(arr, size, temp);
}

static void wrap_radix_hybrid(T *arr, size_t size, size_t unused, T *temp) {
    (void)unused;
    radix_sort_hybrid(arr, size, temp);
}

// ===========================
// Parallel Timsort wrapper (Method A)
// Place this in the wrapper functions section (after wrap_timsort/_prefetch),
// and before the SortAlgorithm algorithms[] definition.
// ===========================

// If not already at the top of file, ensure these includes exist only once:
// #include <pthread.h>
// #include <stdalign.h>

// typedef struct {
//     T* arr;
//     size_t left;
//     size_t right;
//     T* temp_local;        // per-thread temporary buffer
//     size_t run_param;     // RUN size for timsort
//     // Correct function pointer type (matches sort_func_t signature but with T* arr)
//     void (*func)(T*, size_t, size_t, T*);
//     alignas(64) char pad[64]; // padding to avoid false sharing on task structs
// } ThreadTask;

// static void* thread_sort_entry(void* arg) {
//     ThreadTask* t = (ThreadTask*)arg;
//     if (t->right < t->left) return NULL;
//     size_t len = t->right - t->left + 1;
//     if (len > 1) {
//         // Call the sorting function on this block using the per-thread temp
//         t->func(t->arr + t->left, len, t->run_param, t->temp_local);
//     }
//     return NULL;
// }

static void wrap_timsort_parallel(T *arr, size_t size, size_t threads, T *temp) {
    if (size <= 1) return;

    size_t P = (threads < 1) ? 1 : threads;

    // Partition: evenly divide, align to 16 elements (64B cache line for uint32_t)
    size_t chunk = (size + P - 1) / P;
    chunk = (chunk + 15) & ~((size_t)15); // round up to multiple of 16

    pthread_t* th     = (pthread_t*)malloc(P * sizeof(pthread_t));
    ThreadTask* tasks = (ThreadTask*)malloc(P * sizeof(ThreadTask));
    T** temp_locals   = (T**)malloc(P * sizeof(T*));

    size_t* starts = (size_t*)malloc(P * sizeof(size_t));
    size_t* ends   = (size_t*)malloc(P * sizeof(size_t));
    size_t nblocks = 0;

    for (size_t t = 0; t < P; t++) {
        size_t left = t * chunk;
        if (left >= size) break;
        size_t right = left + chunk - 1;
        if (right >= size) right = size - 1;

        size_t len = (right >= left) ? (right - left + 1) : 0;
        temp_locals[t] = (len > 0) ? (T*)malloc(len * sizeof(T)) : NULL;

        tasks[t].arr        = arr;
        tasks[t].left       = left;
        tasks[t].right      = right;
        tasks[t].temp_local = temp_locals[t];
        tasks[t].run_param  = RUN_MEDIUM; // tune if needed
        // Function pointer points to timsort_with_run (matches void (*)(T*, size, run, temp))
        tasks[t].func       = timsort_with_run;

        starts[nblocks] = left;
        ends[nblocks]   = right;
        nblocks++;
    }

    for (size_t t = 0; t < nblocks; t++) {
        pthread_create(&th[t], NULL, thread_sort_entry, &tasks[t]);
    }
    for (size_t t = 0; t < nblocks; t++) {
        pthread_join(th[t], NULL);
    }

    // Tree-style pairwise merge rounds until one block remains
    while (nblocks > 1) {
        pairwise_merge_round(arr, temp, nblocks, starts, ends);
        size_t new_blocks = (nblocks / 2) + (nblocks % 2);
        for (size_t i = 0; i < new_blocks; i++) {
            starts[i] = starts[2*i];
            ends[i]   = ends[2*i + ((2*i + 1 < nblocks) ? 1 : 0)];
        }
        nblocks = new_blocks;
    }

    // Cleanup
    for (size_t t = 0; t < P; t++) {
        if (temp_locals[t]) free(temp_locals[t]);
    }
    free(temp_locals);
    free(starts);
    free(ends);
    free(tasks);
    free(th);
}


// Run single benchmark
static double benchmark_single(sort_func_t func, T *src, size_t size, 
                               size_t param, T *work, T *temp, int warmup) {
    // Copy source to working array
    memcpy(work, src, size * sizeof(T));
    
    if (warmup) {
        // Warmup run (don't time)
        func(work, size, param, temp);
        memcpy(work, src, size * sizeof(T));
    }
    
    double start = get_time_us();
    func(work, size, param, temp);
    double end = get_time_us();
    
    if (!verify_sorted(work, size)) {
        printf("VERIFICATION FAILED!\n");
        return -1.0;
    }
    
    return end - start;
}

// ============================================================================
// MAIN BENCHMARK DRIVER
// ============================================================================

int main(int argc, char *argv[]) {
    // Default: 256MB of data (64M uint32_t elements)
    // Adjust for your testing: 1GB = 268435456 elements
    size_t size = 64 * 1024 * 1024;  // 64M elements = 256MB
    int num_runs = 3;  // Average over multiple runs
    
    if (argc > 1) {
        size = (size_t)atoll(argv[1]);
    }
    if (argc > 2) {
        num_runs = atoi(argv[2]);
    }
    
    double size_gb = (double)(size * sizeof(T)) / (1024.0 * 1024.0 * 1024.0);
    printf("=== Sorting Benchmark ===\n");
    printf("Array size: %zu elements (%.3f GB)\n", size, size_gb);
    printf("Data type: %zu bytes\n", sizeof(T));
    printf("Runs per test: %d\n\n", num_runs);
    
    // Allocate arrays
    T *source = (T *)malloc(size * sizeof(T));
    T *work = (T *)malloc(size * sizeof(T));
    T *temp = (T *)malloc(size * sizeof(T));
    
    if (!source || !work || !temp) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return 1;
    }
    
    // Define algorithms to test
    SortAlgorithm algorithms[] = {
        // Timsort variants with different RUN sizes
        {"timsort_run32",      wrap_timsort,          RUN_SMALL},
        {"timsort_run64",      wrap_timsort,          RUN_MEDIUM},
        {"timsort_run128",     wrap_timsort,          RUN_LARGE},
        {"timsort_run256",     wrap_timsort,          RUN_XLARGE},
        {"timsort_run512",     wrap_timsort,          RUN_CACHE},
        // Prefetch variants
        {"timsort_pf_run64",   wrap_timsort_prefetch, RUN_MEDIUM},
        {"timsort_pf_run128",  wrap_timsort_prefetch, RUN_LARGE},
        {"timsort_pf_run256",  wrap_timsort_prefetch, RUN_XLARGE},
        // Radix sort
        {"radix_lsd",          wrap_radix,            0},
        {"radix_hybrid",       wrap_radix_hybrid,     0},
        // --- Parallel variants (Method A) ---
        {"timsort_parallel_t2",  wrap_timsort_parallel, 2},
        {"timsort_parallel_t4",  wrap_timsort_parallel, 4},
        {"timsort_parallel_t8",  wrap_timsort_parallel, 8},
        {"timsort_parallel_t16", wrap_timsort_parallel, 16},

    };
    size_t num_algorithms = sizeof(algorithms) / sizeof(algorithms[0]);
    
    // Distributions to test
    Distribution distributions[] = {
        DIST_RANDOM_UNIFORM,
        DIST_NEARLY_SORTED,
        DIST_REVERSE_SORTED,
        DIST_FEW_UNIQUE,
    };
    size_t num_distributions = sizeof(distributions) / sizeof(distributions[0]);
    
    // Print CSV header
    printf("algorithm,distribution,size,time_us,time_sec,throughput_MB_s,cost_per_GB\n");
    
    // Run benchmarks
    for (size_t d = 0; d < num_distributions; d++) {
        Distribution dist = distributions[d];
        
        // Generate fresh data for each distribution
        generate_data(source, size, dist, 42);
        
        for (size_t a = 0; a < num_algorithms; a++) {
            SortAlgorithm *alg = &algorithms[a];
            double total_time = 0.0;
            
            for (int run = 0; run < num_runs; run++) {
                double t = benchmark_single(alg->func, source, size, 
                                           alg->param, work, temp, run == 0);
                if (t < 0) {
                    printf("ERROR in %s\n", alg->name);
                    continue;
                }
                total_time += t;
            }
            
            double avg_time_us = total_time / num_runs;
            double avg_time_sec = avg_time_us / 1e6;
            double throughput_MB = (size * sizeof(T) / (1024.0 * 1024.0)) / avg_time_sec;
            
            // Cost calculation (example: $0.50/hour for CloudLab-ish pricing)
            // Adjust this based on your actual instance pricing
            double hourly_cost = 0.50;  // $/hour
            double cost_per_GB = (hourly_cost / 3600.0) * (avg_time_sec / size_gb);
            
            printf("%s,%s,%zu,%.2f,%.6f,%.2f,%.8f\n",
                   alg->name, dist_name(dist), size,
                   avg_time_us, avg_time_sec, throughput_MB, cost_per_GB);
        }
    }
    
    free(source);
    free(work);
    free(temp);
    
    printf("\n=== Benchmark Complete ===\n");
    return 0;
}