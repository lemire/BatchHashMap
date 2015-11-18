// icc -std=gnu99 -Wall -O3 -g -march=native ranged.c -o ranged
// usage: ranged [range]

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>
#define DEFAULT_RANGE 100
#define LOOP_COUNT 1000
#define TIMING_REPEATS 10000

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
        printf("%s: ", #test);                                          \
        fflush(NULL);                                                   \
        uint64_t cycles_start, cycles_final, cycles_diff;               \
        uint64_t min_overhead = (uint64_t) -1;                          \
        for (int i = 0; i < TIMING_REPEATS; i++) {                      \
            RDTSC_START(cycles_start);                                  \
            RDTSC_FINAL(cycles_final);                                  \
            cycles_diff = (cycles_final - cycles_start);                \
            if (cycles_diff < min_overhead) min_overhead = cycles_diff; \
        }                                                               \
        uint64_t min_diff = (uint64_t) -1;                              \
        for (int i = 0; i < TIMING_REPEATS; i++) {                      \
            RDTSC_START(cycles_start);                                  \
            test;                                                       \
            RDTSC_FINAL(cycles_final);                                  \
            cycles_diff = (cycles_final - cycles_start);                \
            if (cycles_diff < min_diff) min_diff = cycles_diff;         \
        }                                                               \
        min_diff = min_diff - min_overhead;                             \
        float cycles_per_rand = min_diff / (float) (count);             \
        printf(" %.2f cycles/rand\n", cycles_per_rand);                 \
    } while (0)


#include "pcg_basic.c"

// probably not fair
uint32_t __attribute__ ((noinline)) ranged_random_recycle_mult(uint32_t range) {
    uint64_t rotations = 0;
    uint64_t random32bit = pcg32_random();
    while (1) {
        uint32_t candidate = (random32bit * range) >> 32;
        if (random32bit - candidate > UINT32_MAX - range + 1) {
            if (rotations == 31) {
                rotations = 0; // get new random number
                random32bit = pcg32_random();
            } else {
                rotations++; // recycle by rotating right 1 bit
                random32bit = (random32bit >> 1) | (random32bit << 31);
            }
        } else {
            return candidate; // [0, range)
        }
    }
    // return from within loop
}


// this is probably not fair
uint32_t __attribute__ ((noinline)) ranged_random_recycle_mod(uint32_t range) {
    uint64_t rotations = 0;
    uint64_t random32bit = pcg32_random();
    while (1) {
        uint32_t candidate = (uint32_t)(random32bit) % range;
        if (random32bit - candidate > UINT32_MAX - range + 1) {
            if (rotations == 31) {
                rotations = 0; // get new random number
                random32bit = pcg32_random();
            } else {
                rotations++; // recycle by rotating right 1 bit
                random32bit = (random32bit >> 1) | (random32bit << 31);
            }
        } else {
            return candidate; // [0, range)
        }
    }
    // return from within loop
}

uint32_t __attribute__ ((noinline)) ranged_random_mult(uint32_t range) {
    uint64_t random32bit, candidate, multiresult;
    uint32_t leftover;
    uint32_t threshold = (uint32_t)((1ULL<<32)/range * range  - 1);
    do {
        random32bit = pcg32_random();
        multiresult = random32bit * range;
        candidate =  multiresult >> 32;
        leftover = (uint32_t) multiresult;
    } while (leftover > threshold);
    return candidate; // [0, range)
}

uint32_t ranged_random_mult_lazy(uint32_t range) {
    uint64_t random32bit, candidate, multiresult;
    uint32_t leftover;
    uint32_t threshold;
#ifdef __BMI2__
    uint32_t lsbset =  _pdep_u32(1,range);
#else
    uint32_t lsbset =  0; // range & (~(range-1)); // too expensive
#endif
    random32bit = pcg32_random();
    multiresult = random32bit * range;
    candidate =  multiresult >> 32;
    leftover = (uint32_t) multiresult;
    if(leftover > lsbset - range - 1 ) {//2^32 -range +lsbset <= leftover
        threshold = (uint32_t)((1ULL<<32)/range * range  - 1);
        do {
            random32bit = pcg32_random();
            multiresult = random32bit * range;
            candidate =  multiresult >> 32;
            leftover = (uint32_t) multiresult;
        } while (leftover > threshold);
    }
    return candidate; // [0, range)
}

uint32_t ranged_random_mult_lazynopower2(uint32_t range) {
    uint64_t random32bit, candidate, multiresult;
    uint32_t leftover;
    uint32_t threshold;
    random32bit = pcg32_random();
    multiresult = random32bit * range;
    candidate =  multiresult >> 32;
    leftover = (uint32_t) multiresult;
    if(leftover >  - range - 1 ) {//2^32 -range  <= leftover
        threshold = (uint32_t)((1ULL<<32)/range * range  - 1);
        do {
            random32bit = pcg32_random();
            multiresult = random32bit * range;
            candidate =  multiresult >> 32;
            leftover = (uint32_t) multiresult;
        } while (leftover > threshold);
    }
    return candidate; // [0, range)
}



uint32_t __attribute__ ((noinline)) ranged_random_mod(uint32_t range) {
    uint64_t random32bit, candidate;
    do {
        random32bit = pcg32_random();
        candidate = (uint32_t)(random32bit) % range;
    } while (random32bit - candidate  > UINT32_MAX - range + 1);
    return candidate; // [0, range)
}

uint32_t __attribute__ ((noinline)) ranged_random_pcg32_boundedrand(uint32_t range) {
    return pcg32_boundedrand(range);
}


void loop_mult_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult(range  + i);
    }
}


void loop_mult_lazy_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_lazy(range  + i);
    }
}

void loop_mult_lazynopower2_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_lazynopower2(range  + i);
    }
}

void loop_mod_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mod(range + i );
    }
}

void loop_recycle_mult_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_recycle_mult(range  + i);
    }
}

void loop_recycle_mod_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_recycle_mod(range  + i);
    }
}

void loop_pcg32_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_pcg32_boundedrand(range + i );
    }
}




void loop_mult(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult(range);
    }
}


void loop_mult_lazy(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_lazy(range);
    }
}

void loop_mult_lazynopower2(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_lazynopower2(range);
    }
}

void loop_mod(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mod(range);
    }
}

void loop_recycle_mult(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_recycle_mult(range);
    }
}

void loop_recycle_mod(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_recycle_mod(range);
    }
}

void loop_pcg32(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_pcg32_boundedrand(range);
    }
}

int main(int argc, char **argv) {
    uint32_t range = DEFAULT_RANGE;
    if (argc > 1) range = atoi(argv[1]);
    printf("range = %d \n", range);

    uint64_t cycles_start, cycles_final,cycles_diff;

    uint32_t output[LOOP_COUNT];
    size_t count = LOOP_COUNT;
    memset(output, 0, count);
    printf("\n repeated calls with range value %d \n",range);

    TIMED_TEST(loop_mult(count, range, output), count);
    TIMED_TEST(loop_mult_lazy(count, range, output), count);
    TIMED_TEST(loop_mult_lazynopower2(count, range, output), count);
     TIMED_TEST(loop_mod(count, range, output), count);
    TIMED_TEST(loop_pcg32(count, range, output), count);

    printf("\n range value will increment starting at %d and going toward %lu \n",range,range+count);

    TIMED_TEST(loop_mult_linear(count, range, output), count);
    TIMED_TEST(loop_mult_lazy_linear(count, range, output), count);
    TIMED_TEST(loop_mult_lazynopower2_linear(count, range, output), count);
     TIMED_TEST(loop_mod_linear(count, range, output), count);
    TIMED_TEST(loop_pcg32_linear(count, range, output), count);

    printf("\n Hint: try large powers of two, ./ranged 1073741824 \n");
    return 0;

}
