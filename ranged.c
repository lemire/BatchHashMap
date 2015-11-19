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
    uint64_t random32bit,  multiresult;
    uint32_t leftover, threshold;
    if((range & ( range - 1 )) == 0) return pcg32_random() & (range - 1);
    if(range >0x80000000) {// if range > 1<<31
        while(random32bit >= range) {
            random32bit = pcg32_random();
        }
        return random32bit; // [0, range)
    }
    threshold = 0xFFFFFFFF / range * range - 1;//(uint32_t)((((uint64_t)1)<<32)/range) * range  - 1;
    do {
        random32bit = pcg32_random();
        multiresult = random32bit * range;
        leftover = (uint32_t) multiresult;
    } while (leftover > threshold);
    return multiresult >> 32; // [0, range)
}

uint32_t __attribute__ ((noinline)) ranged_random_mult64(uint32_t range) {
    uint64_t random64bit = pcg32_random() | ((uint64_t) pcg32_random() << 32);
    uint64_t high;
    uint64_t  low = _mulx_u64(random64bit,range,&high);
    return (uint32_t) high;
}


uint32_t ranged_random_mult_lazy(uint32_t range) {
    uint64_t random32bit, multiresult;
    uint32_t leftover;
    uint32_t threshold;
    uint32_t lsbset;
    random32bit = pcg32_random();
    if(range >0x80000000) {// if range > 1<<31
        while(random32bit >= range) {
            random32bit = pcg32_random();
        }
        return random32bit; // [0, range)
    }
    lsbset =  _blsi_u32(range);//  range & (~(range-1));
    multiresult = random32bit * range;
    leftover = (uint32_t) multiresult;
    if(leftover > lsbset - range - 1 ) {//2^32 -range +lsbset <= leftover
        threshold = 0xFFFFFFFF / range * range - 1;//(uint32_t)((((uint64_t)1)<<32)/range) * range  - 1;
        do {
            random32bit = pcg32_random();
            multiresult = random32bit * range;
            leftover = (uint32_t) multiresult;
        } while (leftover > threshold);
    }
    return multiresult >> 32; // [0, range)
}

// just to test how fast it could get in principle
uint32_t ranged_random_mult_fake(uint32_t range) {
    uint64_t random32bit, multiresult;
    random32bit = pcg32_random();
    multiresult = random32bit * range;
    return multiresult >> 32; // [0, range)
}


uint32_t ranged_random_mult_lazynopower2(uint32_t range) {
    uint64_t random32bit,  multiresult;
    uint32_t leftover;
    uint32_t threshold;
    random32bit = pcg32_random();
    if(range >0x80000000) {// if range > 1<<31
        while(random32bit >= range) {
            random32bit = pcg32_random();
        }
        return random32bit; // [0, range)
    }
    multiresult = random32bit * range;
    leftover = (uint32_t) multiresult;
    if(leftover >  - range - 1 ) {//2^32 -range  <= leftover
        if((range & (range - 1)) == 0) {
            return pcg32_random() & (range - 1);
        }
        threshold = 0xFFFFFFFF / range * range - 1;
        //threshold = (uint32_t)((((uint64_t)1)<<32)/range) * range  - 1;
        do {
            random32bit = pcg32_random();
            multiresult = random32bit * range;
            leftover = (uint32_t) multiresult;
        } while (leftover > threshold);
    }
    return multiresult >> 32; // [0, range)
}

uint32_t ranged_random_mult_lazycheckpower2(uint32_t range) {
    uint64_t random32bit, multiresult;
    uint32_t leftover;
    uint32_t threshold;
    random32bit = pcg32_random();
    if((range & (range - 1)) == 0) {
        return random32bit & (range - 1);
    }
    if(range >0x80000000) {// if range > 1<<31
        while(random32bit >= range) {
            random32bit = pcg32_random();
        }
        return random32bit; // [0, range)
    }
    multiresult = random32bit * range;
    leftover = (uint32_t) multiresult;
    if(leftover >  - range - 1 ) {//2^32 -range  <= leftover
        threshold = 0xFFFFFFFF / range * range - 1;//(uint32_t)((((uint64_t)1)<<32)/range) * range  - 1;
        do {
            random32bit = pcg32_random();
            multiresult = random32bit * range;
            leftover = (uint32_t) multiresult;
        } while (leftover > threshold);
    }
    return multiresult >> 32; // [0, range)
}


uint32_t __attribute__ ((noinline)) ranged_random_mod(uint32_t range) {
    uint32_t random32bit, candidate;
    do {
        random32bit = pcg32_random();
        candidate = random32bit % range;
    } while (random32bit - candidate  > UINT32_MAX - range + 1);
    return candidate; // [0, range)
}

// inspired by golang
uint32_t __attribute__ ((noinline)) ranged_random_modgolang(uint32_t range) {
    uint32_t random32bit, maxval;
    if((range & (range - 1)) == 0) {
        return pcg32_random() & (range - 1);
    }
    if(range >= (1U<<31)) {
        abort(); // not good
    }
    maxval = (1U<<31) - 1 - ((1U<<31) % range);
    random32bit = pcg32_random() & 0x7FFFFFFF;
    while(random32bit > maxval) {
        random32bit = pcg32_random() & 0x7FFFFFFF;
    }
    return random32bit % range; // [0, range)
}

uint32_t __attribute__ ((noinline)) ranged_random_pcg32_boundedrand(uint32_t range) {
    return pcg32_boundedrand(range);
}


void loop_mult_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult(range  + i);
    }
}

void loop_mult64_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult64(range  + i);
    }
}

void loop_mult_fake_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_fake(range  + i);
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

void loop_mult_lazycheckpower2_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_lazycheckpower2(range  + i);
    }
}


void loop_mod_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mod(range + i );
    }
}

void loop_modgolang_linear(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_modgolang(range + i );
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

void loop_mult64(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult64(range);
    }
}

void loop_mult_fake(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_fake(range);
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


void loop_mult_lazycheckpower2(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mult_lazycheckpower2(range);
    }
}

void loop_mod(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_mod(range);
    }
}


void loop_modgolang(size_t count, uint32_t range, uint32_t *output) {
    for (size_t i = 0; i < count; i++) {
        *output++ = ranged_random_modgolang(range);
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
    printf("range = %llu \n", (unsigned long long )range);

    uint64_t cycles_start, cycles_final,cycles_diff;

    uint32_t output[LOOP_COUNT];
    size_t count = LOOP_COUNT;
    memset(output, 0, count);
    printf("\n repeated calls with range value %llu \n",(unsigned long long )range);

    TIMED_TEST(loop_mult(count, range, output), count);
    TIMED_TEST(loop_mult64(count, range, output), count);
    TIMED_TEST(loop_mult_fake(count, range, output), count);
    TIMED_TEST(loop_mult_lazy(count, range, output), count);
    TIMED_TEST(loop_mult_lazynopower2(count, range, output), count);
    TIMED_TEST(loop_mult_lazycheckpower2(count, range, output), count);
    TIMED_TEST(loop_mod(count, range, output), count);
    if(range < (1<<31)) TIMED_TEST(loop_modgolang(count, range, output), count);
    TIMED_TEST(loop_pcg32(count, range, output), count);

    printf("\n range value will increment starting at %llu and going toward %llu \n",(unsigned long long )range,(unsigned long long )range+count);

    TIMED_TEST(loop_mult_linear(count, range, output), count);
    TIMED_TEST(loop_mult64_linear(count, range, output), count);
    TIMED_TEST(loop_mult_fake_linear(count, range, output), count);
    TIMED_TEST(loop_mult_lazy_linear(count, range, output), count);
    TIMED_TEST(loop_mult_lazynopower2_linear(count, range, output), count);
    TIMED_TEST(loop_mult_lazycheckpower2_linear(count, range, output), count);
    TIMED_TEST(loop_mod_linear(count, range, output), count);
    if(range < (1<<31)) TIMED_TEST(loop_modgolang_linear(count, range, output), count);
    TIMED_TEST(loop_pcg32_linear(count, range, output), count);

    printf("\n Hint: try large powers of two, ./ranged 1073741824 or near powers of two like 805306368 \n");
    return 0;
}
