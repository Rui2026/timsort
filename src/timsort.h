#ifndef TIMSORT_H
#define TIMSORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t T;        // unified element type for all sorting components

static inline bool cmp(T a, T b) {
    return a <= b;         // sort rule
}

void timsort(T *arr, size_t n);

#endif
