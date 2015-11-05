
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

#define MT_N              (624)                 // length of state vector
#define MT_M              (397)                 // a period parameter
#define MT_K              (0x9908B0DFU)         // a magic constant
#define hiBit(u)       ((u) & 0x80000000U)   // mask all but highest   bit of u
#define loBit(u)       ((u) & 0x00000001U)   // mask all but lowest    bit of u
#define loBits(u)      ((u) & 0x7FFFFFFFU)   // mask     the highest   bit of u
#define mixBits(u, v)  (hiBit(u)|loBits(v))  // move hi bit of u to hi bit of v

static uint32_t   state[MT_N+1];     // state vector + 1 extra to not violate ANSI C
static uint32_t   *next;          // next random value is computed from here
static int      left = -1;      // can *next++ this many times before reloading

void seedMT(uint32_t seed)
{
    uint32_t x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
    int    j;

    for(left=0, *s++=x, j=MT_N; --j;
            *s++ = (x*=69069U) & 0xFFFFFFFFU);
}


uint32_t reloadMT(void)
{
    uint32_t *p0=state, *p2=state+2, *pM=state+MT_M, s0, s1;
    int    j;

    if(left < -1)
        seedMT(4357U);

    left=MT_N-1, next=state+1;

    for(s0=state[0], s1=state[1], j=MT_N-MT_M+1; --j; s0=s1, s1=*p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? MT_K : 0U);

    for(pM=state, j=MT_M; --j; s0=s1, s1=*p2++)
        *p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? MT_K : 0U);

    s1=state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? MT_K : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 <<  7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;
    return(s1 ^ (s1 >> 18));
}


uint32_t randomMT(void)
{
    uint32_t y;

    if(--left < 0)
        return(reloadMT());

    y  = *next++;
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return(y ^ (y >> 18));
}

static uint32_t x;

uint32_t fastrand(void) {
#ifdef USE_GENERIC
    x = ((x * 1103515245) + 12345) & 0x7fffffff;
    return x;
#else
#ifdef USE_MT
    return randomMT();
#else
#ifdef USE_RAND
    return rand();
#else
    return randomMT();
#endif
#endif
#endif
}


uint32_t round2 (uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}



uint32_t fairRandomInt(uint32_t size) {
    uint32_t candidate, rkey;
    // such a loop is necessary for the result to be fair
    do {
        rkey = fastrand();
        candidate = rkey % size;
    } while(rkey - candidate  > RAND_MAX - size + 1 ); // will be predicted as false
    return candidate;
}

uint32_t fastFairRandomInt(uint32_t size, uint32_t mask, uint32_t bused) {
    uint32_t candidate, rkey;
    int32_t  budget = 31;// assumption
    rkey = fastrand();
    candidate = rkey & mask;
    // such a loop is necessary for the result to be fair
    while(candidate >= size) {
        budget -= bused;// we wasted bused bits
        if(budget >= (int32_t) bused)  {
            rkey >>= bused;
        } else {
            rkey = fastrand();
            budget = 31;// assumption
        }
        candidate = rkey & mask;
    }
    return candidate;
}
// Fisher-Yates shuffle, shuffling an array of integers
void  justrandom(size_t size) {
    size_t i;
    for (i=size; i>1; i--) {
        size_t nextpos = fairRandomInt(i);
    }
}

// Fisher-Yates shuffle, shuffling an array of integers
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

uint32_t fastround2 (uint32_t v) {
    return 1 << (32 - __builtin_clz(v-1));
}

// Fisher-Yates shuffle, shuffling an array of integers
void  fast_shuffle(int *storage, size_t size) {
    size_t i;
    uint32_t bused = 32 - __builtin_clz(size);
    uint32_t m2 = 1 << (32- __builtin_clz(size-1));
    i=size;
    while(i>1) {
        for (; 2*i>m2; i--) {
            size_t nextpos = fastFairRandomInt(i, m2-1,bused);//
            int tmp = storage[i - 1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
        m2 = m2 >> 1;
        bused--;
    }
}


// Random Permutations on Distributed􏰀 External and Hierarchical Memory by Peter Sanders
void  shuffle_sanders(int *storage, size_t size, size_t buffersize) {
    size_t i;
    size_t l;
    size_t NBR_BLOCK = round2((size +buffersize -1)/buffersize);
    size_t BLOCK_SIZE = 4 * buffersize;
    size_t * counter = malloc(NBR_BLOCK * sizeof(size_t));
    for(i = 0 ; i < NBR_BLOCK; ++i)
        counter[i] = 0;
    int* buffer = malloc(BLOCK_SIZE * NBR_BLOCK * sizeof(int));
    for(i = 0; i < size; i++) {
        int block = rand() &  ( NBR_BLOCK - 1) ; // NBR_BLOCK is a power of two
        buffer[BLOCK_SIZE * block + counter[block]] = storage[i];
        counter[block]++;
        if(counter[block]>=BLOCK_SIZE) printf("insufficient mem\n");
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
void  shuffle4(int *storage, size_t size) {
    size_t i,j;
    i = size;
    if(size > 4) {
        size_t nextposs[4];
        int vals[4];

        for (; i>4; i-=4) {
            nextposs[0] = fairRandomInt(i);
            nextposs[1] = fairRandomInt(i - 1);
            nextposs[2] = fairRandomInt(i - 2);
            nextposs[3] = fairRandomInt(i - 3);
            vals[0] = storage[nextposs[0]];
            vals[1] = storage[nextposs[1]];
            vals[2] = storage[nextposs[2]];
            vals[3] = storage[nextposs[3]];
            storage[nextposs[0]] = storage[i - 1];
            storage[nextposs[1]] = storage[i - 2];
            storage[nextposs[2]] = storage[i - 3];
            storage[nextposs[3]] = storage[i - 4];
            storage[i - 1] = vals[0];
            storage[i - 2] = vals[1];
            storage[i - 3] = vals[2];
            storage[i - 4] = vals[3];
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
void  shuffle_sanders_prefetch16(int *storage, size_t size, size_t buffersize) {
    size_t i;
    size_t l;
    size_t NBR_BLOCK = round2((size +buffersize -1)/buffersize);
    size_t BLOCK_SIZE = 4 * buffersize;
    size_t * counter = malloc(NBR_BLOCK * sizeof(size_t));
    for(i = 0 ; i < NBR_BLOCK; ++i)
        counter[i] = 0;
    int* buffer = malloc(BLOCK_SIZE * NBR_BLOCK * sizeof(int));
    for(i = 0; i < size; i++) {
        int block = rand() & ( NBR_BLOCK - 1) ; // NBR_BLOCK is a power of two
        buffer[BLOCK_SIZE * block + counter[block]] = storage[i];
        counter[block]++;
        if(counter[block]>=BLOCK_SIZE) printf("insufficient mem\n");
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
    size_t N =  16777216;
    int bogus = 0;
    size_t i;
    float cycles_per_search1;
    int *array = (int *) malloc( N * sizeof(int) );
    uint64_t cycles_start, cycles_final;
#ifdef USE_GENERIC
    printf("Using some basic random number generator\n");
#else
#ifdef USE_MT
    printf("Using Mersenne Twister\n");
#else
#ifdef USE_RAND
    printf("Using rand\n");
#else
    printf("Using Mersenne Twister\n");
#endif
#endif
#endif

    printf("populating array %zu \n",N);

    for(i = 0; i < N; ++i) {
        array[i] = i;
    }
    x=1;
    seedMT(x);
    printf("\n");
    RDTSC_START(cycles_start);
    justrandom( N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("just random cycles per key  %.2f \n", cycles_per_search1);

    RDTSC_START(cycles_start);
    shuffle( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("normal shuffle cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    fast_shuffle( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("fast shuffle cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    shuffle4( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("shuffle4 cycles per key  %.2f \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_sanders( array, N, 24000 );
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
    shuffle_sanders_prefetch16( array, N, 24000 );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("sanders with prefetch 16 shuffle cycles per key  %.2f  \n", cycles_per_search1);

    return bogus;
}
