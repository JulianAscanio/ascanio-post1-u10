#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REPEAT      100
#define REPEAT_RAND 10

/* Medir tiempo de acceso secuencial a un array de n_bytes */
double bench_seq(size_t n_bytes) {
    volatile char *arr = (volatile char *)malloc(n_bytes);
    if (!arr) return -1.0;
    memset((void *)arr, 1, n_bytes);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int r = 0; r < REPEAT; r++) {
        for (size_t i = 0; i < n_bytes; i++) {
            (void)arr[i];
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    free((void *)arr);

    double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return ns / (REPEAT * (double)n_bytes);
}

/* Acceso aleatorio usando indices pre-mezclados (Fisher-Yates) */
double bench_rand(size_t n_bytes) {
    size_t n = n_bytes / sizeof(int);
    int *arr = (int *)malloc(n * sizeof(int));
    size_t *idx = (size_t *)malloc(n * sizeof(size_t));
    if (!arr || !idx) {
        free(arr);
        free(idx);
        return -1.0;
    }

    for (size_t i = 0; i < n; i++) {
        arr[i] = (int)i;
        idx[i] = i;
    }
    srand(42);
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        size_t tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }

    int reps = (n_bytes >= 4 * 1024 * 1024) ? REPEAT_RAND : REPEAT;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    volatile long sum = 0;
    for (int r = 0; r < reps; r++) {
        for (size_t i = 0; i < n; i++) {
            sum += arr[idx[i]];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    free(arr);
    free(idx);
    double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
    return ns / (reps * (double)n);
}

int main(void) {
    size_t sizes[] = {
        4 * 1024, 8 * 1024, 16 * 1024, 32 * 1024,         /* Region L1 (32 KiB) */
        64 * 1024, 128 * 1024, 256 * 1024, 512 * 1024,    /* Region L2 (512 KiB) */
        1024 * 1024, 4 * 1024 * 1024,                     /* Region L3 (4096 KiB) */
        16 * 1024 * 1024, 64 * 1024 * 1024                /* Region RAM */
    };
    int n = (int)(sizeof(sizes) / sizeof(sizes[0]));

    printf("%-12s  %12s  %14s\n", "Array (KiB)", "Seq (ns/B)", "Rand (ns/elem)");
    for (int i = 0; i < n; i++) {
        double lat_seq = bench_seq(sizes[i]);
        double lat_rnd = bench_rand(sizes[i]);
        printf("%-12zu  %12.3f  %14.3f\n", sizes[i] / 1024, lat_seq, lat_rnd);
    }
    return 0;
}
