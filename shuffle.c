
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
    if(size > RAND_MAX) { // will be predicted as false
        return -1;
    }
    size_t candidate, rkey;
    // such a loop is necessary for the result to be fair
    do {
        rkey = rand();// is rand fair? hope so...
        candidate = rkey % size;
    } while(rkey + size  > RAND_MAX + candidate + 1 ); // will be predicted as false
    return candidate;
}

//Fisher-Yates shuffle, shuffling an array of integers
void  shuffle(int *storage, size_t size) {
    size_t i;
    for (i=size; i>1; i--) {
        size_t nextpos = fairRandomInt(i);
        int tmp = storage[i-1];// likely in cache
        int val = storage[nextpos]; // could be costly
        storage[i - 1] = val;
        storage[nextpos] = tmp; // you might have to read this store later
    }
}
//Fisher-Yates shuffle, shuffling an array of integers
void  shuffle_unrolled(int *storage, size_t size) {
    size_t i = size;
    for (; i>4; i-=4) {
        size_t nextpos1 = fairRandomInt(i);
        size_t nextpos2 = fairRandomInt(i - 1);
        size_t nextpos3 = fairRandomInt(i - 2);
        size_t nextpos4 = fairRandomInt(i - 3);
        int tmp,val;
        tmp = storage[i-1];
        val = storage[nextpos1];
        storage[i-1] = val;
        storage[nextpos1] = tmp;
        tmp = storage[i-2];
        val = storage[nextpos2];
        storage[i-2] = val;
        storage[nextpos2] = tmp;
        tmp = storage[i-3];
        val = storage[nextpos3];
        storage[i-3] = val;
        storage[nextpos3] = tmp;
        tmp = storage[i-4];
        val = storage[nextpos4];
        storage[i-4] = val;
        storage[nextpos4] = tmp;
    }
    for (; i>1; i--) {
        size_t nextpos = fairRandomInt(i);
        int tmp = storage[i-1];// likely in cache
        int val = storage[nextpos]; // could be costly
        storage[i - 1] = val;
        storage[nextpos] = tmp; // you might have to read this store later
    }
}



//Fisher-Yates shuffle, shuffling an array of integers
void  shuffle2(int *storage, size_t size) {
    size_t i=size;
    for (; i>4; i-=4) {
        int tmp1 = storage[i-1];// likely in cache
        int tmp2 = storage[i-2];// in cache
        int tmp3 = storage[i-3];// in cache
        int tmp4 = storage[i-4];// in cache
        size_t nextpos1 = fairRandomInt(i);
        int val1 = storage[nextpos1]; // could be costly
        size_t nextpos2 = fairRandomInt(i - 1);
        size_t nextpos3 = fairRandomInt(i - 2);
        size_t nextpos4 = fairRandomInt(i - 3);
        if((nextpos1 != nextpos2) && (nextpos2 != nextpos3) && (nextpos3 != nextpos4) &&(nextpos4 != nextpos1)) {
            storage[nextpos1] = tmp1; // you might have to read this store later
            storage[i - 1] = val1;
            int val2 = storage[nextpos2]; // could be costly
            storage[nextpos2] = tmp2; // you might have to read this store later
            storage[i - 2] = val2;
            int val3 = storage[nextpos3]; // could be costly
            storage[nextpos3] = tmp3; // you might have to read this store later
            storage[i - 3] = val3;
            int val4 = storage[nextpos4]; // could be costly
            storage[nextpos4] = tmp4; // you might have to read this store later
            storage[i - 4] = val4;
        } else {
            int val;
            storage[i-1] = val1;
            storage[nextpos1] = tmp1;
            val = storage[nextpos2]; // might not be val2
            storage[i-2] = val;
            storage[nextpos2] = tmp2;
            val = storage[nextpos3]; // might not be val3
            storage[i-3] = val;
            storage[nextpos3] = tmp3;
            val = storage[nextpos4]; // might not be val4
            storage[i-4] = val;
            storage[nextpos4] = tmp4;
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
    size_t i;
    printf("populating array \n");
    for(i = 0; i < N; ++i) {
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
    shuffle_unrolled( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("unrolled shuffle cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    shuffle2( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search2 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("alt shuffle cycles per key %.2f \n", cycles_per_search2);

    return bogus;
}
