#include "timsort.h"

#define RUN 64   

static void insertion_sort(T *arr, size_t left, size_t right) {
    for (size_t i = left + 1; i <= right; i++) {
        T temp = arr[i];
        size_t j = i;
        while (j > left && cmp(temp, arr[j-1])) {
            arr[j] = arr[j-1];
            j--;
        }
        arr[j] = temp;
    }
}

static void merge(T *arr, size_t left, size_t mid, size_t right, T *temp) {
    size_t i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        if (cmp(arr[i], arr[j])) temp[k++] = arr[i++];
        else temp[k++] = arr[j++];
    }
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];

    memcpy(arr + left, temp + left, (right - left + 1) * sizeof(T));
}

void timsort(T *arr, size_t n) {
    if (n <= 1) return;

    T *temp = malloc(sizeof(T) * n);

    // Step1: sort RUN
    for (size_t i = 0; i < n; i += RUN) {
        size_t right = (i + RUN - 1 < n - 1) ? i + RUN - 1 : n - 1;
        insertion_sort(arr, i, right);
    }

    // Step2：merge RUN
    for (size_t size = RUN; size < n; size *= 2) {
        for (size_t left = 0; left < n; left += 2 * size) {
            size_t mid   = left + size - 1;
            if (mid >= n - 1) break;
            size_t right = (left + 2 * size - 1 < n - 1) ? left + 2 * size - 1 : n - 1;

            merge(arr, left, mid, right, temp);
        }
    }

    free(temp);
}
