
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>


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


// return and integer between 0 and size -1 inclusively, return -1 in case of trouble
size_t fairRandomInt(size_t size) {
    if(size > RAND_MAX) {
        return -1;
    }
    size_t candidate, rkey;
    // such a loop is necessary for the result to be fair
    do {
        rkey = rand();// is rand fair? hope so...
        candidate = rkey % size;
    } while(rkey + size  > RAND_MAX + candidate + 1 );
    return candidate;
}

//Fisher-Yates shuffle, shuffling an array of integers
void  shuffle(int *storage, size_t size) {
    for (size_t i=size; i>1; i--) {
        size_t nextpos = fairRandomInt(i);
        int tmp = storage[i-1];
        storage[i - 1] = storage[nextpos];
        storage[nextpos] = tmp;
    }
}


//Fisher-Yates shuffle, shuffling an array of integers
void  shuffle2(int *storage, size_t size) {
    size_t i=size;
    for (; i>2; i-=2) {
        size_t nextpos1 = fairRandomInt(i);
        size_t nextpos2 = fairRandomInt(i - 1);
        if(nextpos1 != nextpos2) {
          int tmp1 = storage[nextpos1];
          int tmp2 = storage[nextpos2];
          storage[nextpos1] = storage[i-1];
          storage[nextpos2] = storage[i-2];
          storage[i-1] = tmp1;
          storage[i-2] = tmp2;
        } else {
          // we swap locally
          int tmp = storage[i-1];
          storage[i - 1] = storage[i - 2];
          storage[i - 2] = tmp;
        }
    }
    for (; i>1; i--) {
        size_t nextpos = fairRandomInt(i);
        int tmp = storage[i-1];
        storage[i - 1] = storage[nextpos];
        storage[nextpos] = tmp;
    }
}

int main( int argc, char **argv ) {
    size_t N = 16777216;
    int bogus = 0;
    float cycles_per_search1, cycles_per_search2;
    int *array = (int *) malloc( N * sizeof(int) );

    uint64_t cycles_start, cycles_final;
    printf("populating array \n");
    for(size_t i = 0; i < N; ++i) {
        array[i] = i;
    }
    printf("\n");
    RDTSC_START(cycles_start);
    shuffle( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("normal shuffle cycles per key  %.2f \n", cycles_per_search1);

    RDTSC_START(cycles_start);
    shuffle2( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search2 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("alt shuffle cycles per key %.2f \n", cycles_per_search1);

    return bogus;
}
