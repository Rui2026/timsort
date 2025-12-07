#pragma once

#include <stddef.h>
#include "timsort.h"
#include "metrics.h"

typedef enum {
    RANDOM,
    SORTED,
    REVERSED
} data_dist_t;

/* Exposed helpers for tests */
void generate_data(T *arr, size_t n, data_dist_t dist);
int is_sorted(T *arr, size_t n);
void sort_array(T *arr, size_t size);
