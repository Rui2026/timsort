#include <stdio.h>
#include <stdlib.h>
#include "Measurement and Testing/sorting_test.h"
#include "timsort.h"

int main() {
    const size_t n = 10000;
    T* arr = malloc(n * sizeof(T));

    generate_data(arr, n, RANDOM);
    sort_array(arr, n);

    if (!is_sorted(arr, n)) {
        printf("[ERROR] Sorting failed\n");
        return 1;
    }

    printf("[OK] Sorting correctness passed\n");
    free(arr);
    return 0;
}

/* Generate dataset */
void generate_data(T *arr, size_t n, data_dist_t dist) {
    for (size_t i = 0; i < n; i++)
        arr[i] = rand();

    if (dist == SORTED) {
        timsort(arr, n);
    } else if (dist == REVERSED) {
        timsort(arr, n);
        for (size_t i = 0; i < n / 2; i++) {
            T tmp = arr[i];
            arr[i] = arr[n - 1 - i];
            arr[n - 1 - i] = tmp;
        }
    }
}

/* Correctness check */
int is_sorted(T *arr, size_t n) {
    for (size_t i = 1; i < n; i++)
        if (arr[i - 1] > arr[i])
            return 0;
    return 1;
}
