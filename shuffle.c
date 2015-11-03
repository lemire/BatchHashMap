
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <immintrin.h>


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

// Random Permutations on Distributed􏰀 External and Hierarchical Memory by Peter Sanders
void  shuffle_sanders(int *storage, size_t size) {
    size_t i;
    size_t l;
    size_t BLOCK_SIZE = size;
    size_t NBR_BLOCK = 16;
    size_t * counter = malloc(NBR_BLOCK * sizeof(size_t));
    for(i = 0 ; i < NBR_BLOCK; ++i)
      counter[i] = 0;
    int* buffer = malloc(BLOCK_SIZE * NBR_BLOCK * sizeof(int));
    for(i = 0; i < size; i++) {
      int block = rand() & ( NBR_BLOCK - 1) ; // NBR_BLOCK is a power of two
      buffer[BLOCK_SIZE * block + counter[block]] = storage[i];
      counter[block]++;
    }
    l = 0;
    for(i = 0; i < NBR_BLOCK; i++) {
      shuffle(buffer + BLOCK_SIZE * i, counter[i]);
      memcpy(storage + l,buffer + BLOCK_SIZE * i,counter[i]*sizeof(int));
      l += counter[i];
    }
    free(buffer);
    free(counter);
}

//Fisher-Yates shuffle, shuffling an array of integers
void  shuffle_prefetch2(int *storage, size_t size) {
    size_t i;
    i = size;
    if(size > 2) {
        size_t nextposs[2];
        nextposs[(i)%2] = fairRandomInt(i);
        nextposs[(i-1)%2] = fairRandomInt(i - 1);
        for (; i>2; i--) {
            int thisindex = i % 2;
            size_t nextpos = nextposs[thisindex];
            // prefetching part
            size_t np = fairRandomInt(i-2);
            nextposs[thisindex] = np;
            __builtin_prefetch(storage + np);
            // end of prefetching
            int tmp = storage[i-1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
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
void  shuffle_prefetch4(int *storage, size_t size) {
    size_t i;
    i = size;
    if(size > 4) {
        size_t nextposs[4];
        nextposs[i%4] = fairRandomInt(i);
        nextposs[(i-1)%4] = fairRandomInt(i - 1);
        nextposs[(i-2)%4] = fairRandomInt(i - 2);
        nextposs[(i-3)%4] = fairRandomInt(i - 3);
        for (; i>4; i--) {
            int thisindex = i % 4;
            size_t nextpos = nextposs[thisindex];
            // prefetching part
            size_t np = fairRandomInt(i-4);
            nextposs[thisindex] = np;
            __builtin_prefetch(storage + np);
            // end of prefetching
            int tmp = storage[i-1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
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
void  shuffle_prefetch8(int *storage, size_t size) {
    size_t i,j ;
    i = size;
    if(size > 8) {
        size_t nextposs[8];
        for(j = 0; j < 8; ++j)
            nextposs[(i-j)%8] = fairRandomInt(i-j);

        for (; i>8; i--) {
            int thisindex = i % 8;
            size_t nextpos = nextposs[thisindex];
            // prefetching part
            size_t np = fairRandomInt(i-8);
            nextposs[thisindex] = np;
            __builtin_prefetch(storage + np);
            // end of prefetching
            int tmp = storage[i-1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
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
void  shuffle_prefetch16(int *storage, size_t size) {
    size_t i,j;
    i = size;
    if(size > 16) {
        size_t nextposs[16];
        for(j = 0; j < 16; ++j)
            nextposs[(i-j)%16] = fairRandomInt(i-j);

        for (; i>16; i--) {
            int thisindex = i % 16;
            size_t nextpos = nextposs[thisindex];
            // prefetching part
            size_t np = fairRandomInt(i-16);
            nextposs[thisindex] = np;
            __builtin_prefetch(storage + np);
            // end of prefetching
            int tmp = storage[i-1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
    }
    for (; i>1; i--) {
        size_t nextpos = fairRandomInt(i);
        int tmp = storage[i-1];// likely in cache
        int val = storage[nextpos]; // could be costly
        storage[i - 1] = val;
        storage[nextpos] = tmp; // you might have to read this store later
    }
}

// Random Permutations on Distributed􏰀 External and Hierarchical Memory by Peter Sanders
// with prefetching
void  shuffle_sanders_prefetch16(int *storage, size_t size) {
    size_t i;
    size_t l;
    size_t BLOCK_SIZE = size;
    size_t NBR_BLOCK = 16;
    size_t * counter = malloc(NBR_BLOCK * sizeof(size_t));
    for(i = 0 ; i < NBR_BLOCK; ++i)
      counter[i] = 0;
    int* buffer = malloc(BLOCK_SIZE * NBR_BLOCK * sizeof(int));
    for(i = 0; i < size; i++) {
      int block = rand() & ( NBR_BLOCK - 1) ; // NBR_BLOCK is a power of two
      buffer[BLOCK_SIZE * block + counter[block]] = storage[i];
      counter[block]++;
    }
    l = 0;
    for(i = 0; i < NBR_BLOCK; i++) {
      shuffle_prefetch16(buffer + BLOCK_SIZE * i, counter[i]);
      memcpy(storage + l,buffer + BLOCK_SIZE * i,counter[i]*sizeof(int));
      l += counter[i];
    }
    free(buffer);
    free(counter);
}


int main( int argc, char **argv ) {
    size_t N = 16777216;
    int bogus = 0;
    float cycles_per_search1;
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
    shuffle_sanders( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("sanders shuffle cycles per key  %.2f  \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_prefetch2( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("prefetch 2 shuffle cycles per key  %.2f \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_prefetch4( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("prefetch 4 shuffle cycles per key  %.2f \n", cycles_per_search1);

    RDTSC_START(cycles_start);
    shuffle_prefetch8( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("prefetch 8 shuffle cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    shuffle_prefetch16( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("prefetch 16 shuffle cycles per key  %.2f \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_sanders_prefetch16( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("sanders with prefetch 16 shuffle cycles per key  %.2f  \n", cycles_per_search1);
 
    return bogus;
}
