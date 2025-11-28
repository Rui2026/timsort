#include "timsort.h"
#include <stdio.h>
#include <time.h>

int main() {
    size_t n = 1000000;  // data size
    T *arr = malloc(n * sizeof(T));
    if (!arr) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // generate random data
    for (size_t i = 0; i < n; i++)
        arr[i] = rand();

    clock_t start = clock();
    timsort(arr, n);
    clock_t end   = clock();

    printf("Sorted %zu items in %.3f seconds\n",
           n, (double)(end - start) / CLOCKS_PER_SEC);

    // simple verification
    for (int i = 0; i < 10; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    free(arr);
    return 0;
}
