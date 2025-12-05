#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// data type 
typedef uint32_t T;

// compare function
static inline int cmp(T a, T b) {
    return a <= b;
}

// RUN size
#define RUN 64

// insertion_sort in range of [left, right] 
static void insertion_sort(T *arr, size_t left, size_t right) {
    for (size_t i = left + 1; i <= right; i++) {
        T temp = arr[i];
        size_t j = i;
        while (j > left && cmp(temp, arr[j - 1])) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = temp;
    }
}

// merge [left, mid] and [mid+1, right]
static void merge(T *arr, size_t left, size_t mid, size_t right, T *temp) {
    size_t i = left;
    size_t j = mid + 1;
    size_t k = left;

    while (i <= mid && j <= right) {
        if (cmp(arr[i], arr[j])) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }
    while (i <= mid) {
        temp[k++] = arr[i++];
    }
    while (j <= right) {
        temp[k++] = arr[j++];
    }

    for (size_t idx = left; idx <= right; idx++) {
        arr[idx] = temp[idx];
    }
}

// Avoid making changes to this function skeleton, apart from data type changes if required
// In this starter code we have used uint32_t, feel free to change it to any other data type if required
void sort_array(uint32_t *arr, size_t size) {

    if (size <= 1 || arr == NULL) return;

    // create temp cache
    T *temp = (T *)malloc(size * sizeof(T));
    if (!temp) {
        fprintf(stderr, "Error: failed to allocate temp buffer for sorting.\n");
        return;
    }

    // Step 1: sort
    for (size_t i = 0; i < size; i += RUN) {
        size_t right = i + RUN - 1;
        if (right >= size) {
            right = size - 1;
        }
        insertion_sort(arr, i, right);
    }

    // Step 2: merge
    for (size_t curr_size = RUN; curr_size < size; curr_size *= 2) {
        for (size_t left = 0; left < size; left += 2 * curr_size) {
            size_t mid = left + curr_size - 1;
            if (mid >= size - 1) {
                break;
            }
            size_t right = left + 2 * curr_size - 1;
            if (right >= size) {
                right = size - 1;
            }
            merge(arr, left, mid, right, temp);
        }
    }

    free(temp);
}

int main() {
    // array size
    size_t size = 1000000; 

    // initialization
    T *arr = (T *)malloc(size * sizeof(T));
    if (!arr) {
        fprintf(stderr, "Error: failed to allocate input array.\n");
        return 1;
    }

    srand((unsigned)time(NULL));
    for (size_t i = 0; i < size; i++) {
        // uniform random
        arr[i] = (T)rand();
    }

    // copy
    T *sorted_arr = (T *)malloc(size * sizeof(T)); // Allocate memory for the sorted array
    if (!sorted_arr) {
        fprintf(stderr, "Error: failed to allocate sorted array.\n");
        free(arr);
        return 1;
    }
    for (size_t i = 0; i < size; i++) {
        sorted_arr[i] = arr[i];
    }

    // time
    clock_t start = clock();
    // sort the copied array
    sort_array(sorted_arr, size);
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Sorted %zu elements in %.6f seconds.\n", size, elapsed);

    // check
    printf("First 10 elements after sorting:\n");
    for (size_t i = 0; i < 10 && i < size; i++) {
        printf("%u ", (unsigned)sorted_arr[i]);
    }
    printf("\n");

    free(arr);
    free(sorted_arr);
    return 0;
}
