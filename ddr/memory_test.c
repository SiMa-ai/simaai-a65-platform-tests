#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <simaai/simaai_memory.h>


#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

typedef struct {
    void *dest;
    void *src;
    size_t start;
    size_t size;
    pthread_barrier_t *barrier;
} thread_data_t;

static inline void simaai_memcpy_inline(simaai_memory_t *dest, simaai_memory_t *src,  long unsigned int size){
    register simaai_memory_t *r_dest asm("x0") = dest;
    register simaai_memory_t *r_src asm("x1") = src;
    register size_t r_size asm("x2") = size;
    asm volatile("startr:SUBS %2, %2,#128\n\
                    LDP x4, x5, [%1, #0]\n\
                    LDP x6, x7, [%1, #16]\n\
                    LDP x8, x9, [%1, #32]\n\
                    LDP x10, x11, [%1, #48]\n\
                    LDP x12, x13, [%1, #64]\n\
                    LDP x14, x15, [%1, #80]\n\
                    LDP x16, x17, [%1, #96]\n\
                    LDP x18, x19, [%1, #112]\n\
                    STP x4, x5, [%0, #0]\n\
                    STP x6, x7, [%0, #16]\n\
                    STP x8, x9, [%0, #32]\n\
                    STP x10, x11, [%0, #48]\n\
                    STP x12, x13, [%0, #64]\n\
                    STP x14, x15, [%0, #80]\n\
                    STP x16, x17, [%0, #96]\n\
                    STP x18, x19, [%0, #112]\n\
                    ADD %0, %0, #128\n\
                    ADD %1, %1, #128\n\
                    BGT startr"  :
                    : [dest] "r" (r_dest), [src] "r" (r_src), [size] "r" (r_size)
                    : "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "cc", "memory"
                );
}

void simaai_read(void *dest, const void *src, size_t size) {
    if (!dest || !src || size == 0) {
        fprintf(stderr, "simaai_read: Invalid arguments\n");
        return;
    }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    memcpy(dest, src, size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double read_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("simaai_read: Read %zu bytes in %fs\n", size, read_time);
}

void simaai_write(void *dest, const void *src, size_t size) {
    if (!dest || !src || size == 0) {
        fprintf(stderr, "simaai_write: Invalid arguments\n");
        return;
    }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    memcpy(dest, src, size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double write_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("simaai_write: Wrote %zu bytes in %fs\n", size, write_time);
}


void *threaded_memcpy(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    pthread_barrier_wait(data->barrier);
    memcpy((char *)data->dest + data->start, (char *)data->src + data->start, data->size);
    return NULL;
}

void multithreaded_memcpy(void *dest, void *src, size_t size, int threads) {
    pthread_t thread_ids[threads];
    pthread_barrier_t barrier;
    thread_data_t thread_data[threads];
    size_t chunk_size = size / threads;

    pthread_barrier_init(&barrier, NULL, threads);

    for (int i = 0; i < threads; i++) {
        thread_data[i].dest = dest;
        thread_data[i].src = src;
        thread_data[i].start = i * chunk_size;
        thread_data[i].size = (i == threads - 1) ? (size - i * chunk_size) : chunk_size;
        thread_data[i].barrier = &barrier;

        pthread_create(&thread_ids[i], NULL, threaded_memcpy, &thread_data[i]);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }
    
    pthread_barrier_destroy(&barrier);
}

void measure_time(size_t data_size, int test, int threads) {

    static int target = SIMAAI_MEM_TARGET_DMS0;
    int i = 0;

    simaai_memory_t *input_buffer = simaai_memory_alloc_flags(data_size, target, SIMAAI_MEM_FLAG_CACHED);
    if (!input_buffer) {
        perror("Input buffer allocation failed");
        return;
    }
    void *input_addr = simaai_memory_map(input_buffer);
    if (!input_addr) {
        perror("Input buffer mapping failed");
        simaai_memory_free(input_buffer);
        return;
    }

    simaai_memory_t *output_buffer = simaai_memory_alloc_flags(data_size, target, SIMAAI_MEM_FLAG_CACHED);
    if (!output_buffer) {
        perror("Output memory allocation failed");
        return;
    }
    void *output_addr = simaai_memory_map(output_buffer);
    if (!output_addr) {
        perror("Output memory mapping failed");
        simaai_memory_free(output_buffer);
        return;
    }

    memset(input_addr, 0xAA, data_size);
    double t1_max = 0, t2_max = 0, t3_max = 0;
    double t1_min = 10, t2_min = 10, t3_min = 10;
    double t1_sum = 0, t2_sum = 0, t3_sum = 0;

    void (*memcpy_func)(void *, const void *, size_t) = NULL;

    if (test == 1) {
        memcpy_func = memcpy;
    } else if (test == 2) {
        memcpy_func = simaai_memcpy_inline;
    } else if (test == 4) {
        simaai_read(output_addr, input_addr, data_size);
        goto cleanup;
    } else if (test == 5) {
        simaai_write(output_addr, input_addr, data_size);
        goto cleanup;
    } else if (test != 3) {
        fprintf(stderr, "Invalid test number\n");
        simaai_memory_unmap(input_buffer);
        simaai_memory_unmap(output_buffer);
        simaai_memory_free(input_buffer);
        simaai_memory_free(output_buffer);
        return;
    }

    for( i = 0; i < 1000; i++){

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        simaai_memory_invalidate_cache(input_buffer);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double invalidate_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        t1_sum += invalidate_time;
        t1_max = (t1_max < invalidate_time) ? invalidate_time : t1_max;
        t1_min = (t1_min > invalidate_time) ? invalidate_time : t1_min;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (test == 3) {
            multithreaded_memcpy(output_addr, input_addr, data_size, threads);
        } else {
            memcpy_func(output_addr, input_addr, data_size);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double memcpy_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        t2_sum += memcpy_time;
        t2_max = (t2_max < memcpy_time) ? memcpy_time : t2_max;
        t2_min = (t2_min > memcpy_time) ? memcpy_time : t2_min;

        clock_gettime(CLOCK_MONOTONIC, &start);
        simaai_memory_flush_cache(output_buffer);
        clock_gettime(CLOCK_MONOTONIC, &end);

        double flush_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        t3_sum += flush_time;
        t3_max = (t3_max < flush_time) ? flush_time : t3_max;
        t3_min = (t3_min > flush_time) ? flush_time : t3_min;
    }
    printf("Test No: %d\n", test);
    printf("T1: max - %fs, min - %fs, average - %fs\n", t1_max, t1_min, t1_sum/1000);
    printf("T2: max - %fs, min - %fs, average - %fs\n", t2_max, t2_min, t2_sum/1000);
    printf("T3: max - %fs, min - %fs, average - %fs\n", t3_max, t3_min, t3_sum/1000);
    goto cleanup;

    cleanup:
        simaai_memory_unmap(input_buffer);
        simaai_memory_unmap(output_buffer);
        simaai_memory_free(input_buffer);
        simaai_memory_free(output_buffer);
}

size_t get_size_from_string(const char *size_str) {
    if (strcmp(size_str, "1MB") == 0) return 1 * MB;
    if (strcmp(size_str, "2MB") == 0) return 2 * MB;
    if (strcmp(size_str, "4MB") == 0) return 4 * MB;
    if (strcmp(size_str, "8MB") == 0) return 8 * MB;
    if (strcmp(size_str, "256MB") == 0) return 256 * MB;
    if (strcmp(size_str, "512MB") == 0) return 512 * MB;
    if (strcmp(size_str, "1GB") == 0) return 1 * GB;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <test_number> <size> [threads]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int test = atoi(argv[1]);
    if (test < 1 || test > 5) {
        fprintf(stderr, "Invalid test number. Use 1, 2, or 3.\n");
        return EXIT_FAILURE;
    }

    size_t data_size = get_size_from_string(argv[2]);
    if (data_size == 0) {
        fprintf(stderr, "Invalid size specified. Use one of the following: 1MB, 2MB, 4MB, 8MB, 256MB, 512MB, 1GB\n");
        return EXIT_FAILURE;
    }

    if ((test == 4 || test == 5) && !(data_size == 1 * 1024 * 1024 || data_size == 4 * 1024 * 1024 ||
                                      data_size == 8 * 1024 * 1024 || data_size == 1 * 1024 * 1024 * 1024)) {
        fprintf(stderr, "Invalid size for test 4 or test 5. Allowed sizes are: 1MB, 4MB, 8MB, or 1GB.\n");
        return EXIT_FAILURE;
    }

    int threads = 0;
    if (test == 3) {
        if (argc < 4) {
            fprintf(stderr, "Test 3 requires a thread count. Usage: %s 3 <size> <threads>\n", argv[0]);
            return EXIT_FAILURE;
        }
        threads = atoi(argv[3]);
        if (threads != 1 && threads != 2 && threads != 4 && threads != 8) {
            fprintf(stderr, "Invalid thread count specified. Use 1, 2, 4, or 8.\n");
            return EXIT_FAILURE;
        }
    }

    if (test >= 1 && test <= 5) {
        int thread_count = (test == 3) ? threads : 0;
        measure_time(data_size, test, thread_count);
    } else {
        printf("Invalid option\n");
    }

    return EXIT_SUCCESS;
}