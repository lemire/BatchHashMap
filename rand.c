// gcc -fno-inline -std=gnu99 -Wall -O3 -g -march=native rand.c -o rand

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// maximum number of rand() calls per loop
#define MAX_COUNT 10
// (doesn't affect timings, only batch sizes used)

// report minimum cycles after this number of attempts
#define TIMING_REPEATS 10000   
// (can be lower, high is for 'perf record -F10000 rand')

#define RDTSC_START(cycles)                                     \
    do {                                                        \
        register unsigned cyc_high, cyc_low;                    \
        __asm volatile("cpuid\n\t"                              \
                     "rdtsc\n\t"                                \
                     "mov %%edx, %0\n\t"                        \
                     "mov %%eax, %1\n\t"                        \
                     : "=r" (cyc_high), "=r" (cyc_low)          \
                     :: "%rax", "%rbx", "%rcx", "%rdx");        \
        (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;        \
    } while (0)

#define RDTSC_FINAL(cycles)                                     \
    do {                                                        \
        register unsigned cyc_high, cyc_low;                    \
        __asm volatile("rdtscp\n\t"                             \
                     "mov %%edx, %0\n\t"                        \
                     "mov %%eax, %1\n\t"                        \
                     "cpuid\n\t"                                \
                     : "=r" (cyc_high), "=r" (cyc_low)          \
                     :: "%rax", "%rbx", "%rcx", "%rdx");        \
        (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;        \
    } while(0)


#define TIMED_TEST(test, count)                                         \
    do {                                                                \
        fflush(NULL);                                                   \
        uint64_t cycles_start, cycles_final, cycles_diff;               \
        uint64_t min_diff = (uint64_t) -1;                              \
        for (int i = 0; i < TIMING_REPEATS; i++) {                      \
            RDTSC_START(cycles_start);                                  \
            test;                                                       \
            RDTSC_FINAL(cycles_final);                                  \
            cycles_diff = (cycles_final - cycles_start);                \
            if (cycles_diff < min_diff) min_diff = cycles_diff;         \
        }                                                               \
        float cycles_per_rand = min_diff / (float) (count);             \
        printf(" %.2f cycles/rand", cycles_per_rand);                   \
        fflush(NULL);                                                   \
    } while (0)

void rand_to_output(size_t count, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = rand();
    }
}


int main(int argc, char **argv) {
    if (argc > 1) srand(atoi(argv[1]));

    uint32_t output[MAX_COUNT];

    for (size_t count = 1; count < MAX_COUNT; count++) {
        memset(output, 0, count); 
        printf(" Count %4zd ", count);
        TIMED_TEST(rand_to_output(count, output), count);
        printf("\n");
    }

}

