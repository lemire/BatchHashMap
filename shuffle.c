// TODO: see http://arxiv.org/pdf/1508.03167.pdf
// https://github.com/axel-bacher/mergeshuffle
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


/********
* Next part is mersenne Twister
*********/
#include "mersenne.c"


/**************
* next bit is from
* https://github.com/axel-bacher/mergeshuffle/
****/


struct random {
    unsigned long x;
    int c;
};

// mark as used the first b bits of r->x
// they should be shifted out after use (r->x >>= b)

static inline void consume_bits(struct random *r, int b) {
#ifdef COUNT
    count += b;
#endif

    r->c -= b;
    if(r->c < 0) {
        r->x = rand64();
        r->c = 64 - b;
    }
}

static inline unsigned long random_bits(struct random *r, unsigned int i) {
    consume_bits(r, i);
    int y = r->x & ((1UL << i) - 1);
    r->x >>= i;
    return y;
}

static inline int flip(struct random *r) {
    return random_bits(r, 1);
}

static inline unsigned long random_int(struct random *r, unsigned long n) {
    unsigned long v = 1;
    unsigned long d = 0;
    while(1) {
        d += d + flip(r);
        v += v;

        if(v >= n) {
            if(d < n) return d;
            v -= n;
            d -= n;
        }
    }
}

static inline void swap(unsigned int *a, unsigned int *b) {
    unsigned int x = *a;
    unsigned int y = *b;
    *a = y;
    *b = x;
}

void fisher_yates(unsigned int *t, unsigned int n) {
    struct random r = {0, 0};
    unsigned int i;
    for(i = 0; i < n; i ++) {
        unsigned int j = random_int(&r, i + 1);
        swap(t + i, t + j);
    }
}


void merge(unsigned int *t, unsigned int m, unsigned int n) {
    unsigned int *u = t;
    unsigned int *v = t + m;
    unsigned int *w = t + n;

    struct random r = {0, 0};

    // take elements from both halves until one is exhausted

    while(1) {
        if(flip(&r)) {
            if(v == w) break;
            swap(u, v ++);
        } else if(u == v) break;
        u ++;
    }

    // finish using Fisher-Yates

    while(u < w) {
        unsigned int i = random_int(&r, u - t + 1);
        swap(t + i, u ++);
    }
}
unsigned long cutoff = 0x10000;

void mergeshuffle(unsigned int *t, unsigned int n) {
    unsigned int c = 0;
    while((n >> c) > cutoff) c ++;
    unsigned int q = 1 << c;
    unsigned long nn = n;
    unsigned int i,p;
//    #pragma omp parallel for
    for(i = 0; i < q; i ++) {
        unsigned long j = nn * i >> c;
        unsigned long k = nn * (i+1) >> c;
        fisher_yates(t + j, k - j);
    }

    for(p = 1; p < q; p += p) {
        //      #pragma omp parallel for
        for(i = 0; i < q; i += 2*p) {
            unsigned long j = nn * i >> c;
            unsigned long k = nn * (i + p) >> c;
            unsigned long l = nn * (i + 2*p) >> c;
            merge(t + j, k - j, l - j);
        }
    }
}

/****
* End of import from
* https://github.com/axel-bacher/mergeshuffle/
**************/


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
    rkey = fastrand();
    candidate = rkey % size;
    while(rkey - candidate  > RAND_MAX - size + 1 ) { // will be predicted as false
      rkey = fastrand();
      candidate = rkey % size;
    }
    return candidate;
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

uint32_t fastFairRandomInt(randbuf_t * rb, uint32_t size, uint32_t mask, uint32_t bused) {
    uint32_t candidate;
    candidate = grabBits( rb, mask,bused );
    // such a loop is necessary for the result to be fair

    while(candidate >= size) {
        candidate = grabBits( rb, mask,bused );
    }
    return candidate;
}

// Fisher-Yates shuffle, shuffling an array of integers
void  fast_shuffle(int *storage, size_t size) {
    size_t i;
    uint32_t bused = 32 - __builtin_clz(size);
    uint32_t m2 = 1 << (32- __builtin_clz(size-1));
    i=size;
    randbuf_t  rb;
    rbinit(&rb);
    while(i>1) {
        for (; 2*i>m2; i--) {
            size_t nextpos = fastFairRandomInt(&rb, i, m2-1,bused);//
            int tmp = storage[i - 1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
        m2 = m2 >> 1;
        bused--;
    }
}

// Fisher-Yates shuffle, shuffling an array of integers
void  fast_shuffle_floatapprox(int *storage, size_t size) {
  /**
  supposedly, you can map a 64-bit random int v to a double by doing this:
       v * (1.0/18446744073709551616.0L);
      so to get a number between 0 and x, you just multiply this by x?

      But this is bogus.
  * */
    size_t i;
    for(i=size; i>1; i--) {
            uint32_t nextpos = (uint32_t)(fastrand64() * i * (1.0/18446744073709551616.0L));//
            int tmp = storage[i - 1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
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
    size_t i;
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

int demo(size_t array_size) {
    int bogus = 0;
    size_t i;
    float cycles_per_search1;
    int *array = (int *) malloc( array_size * sizeof(int) );
    uint64_t cycles_start, cycles_final;
#ifdef USE_GENERIC
    printf("Using some basic random number generator\n");
#elif USE_SIMDMT
    printf("Using SIMD Mersenne Twister\n");
#elif USE_MT
    printf("Using Mersenne Twister\n");
#elif USE_RAND
    printf("Using rand\n");
#elif USE_HARDWARE
    printf("Using hardware\n");
#else
    printf("Using SIMD Mersenne Twister\n");
#endif
    printf("populating array %zu \n",array_size);
    printf("log(array_size) = %d \n",(33 - __builtin_clz(array_size-1)));

    for(i = 0; i < array_size; ++i) {
        array[i] = i;
    }
    x=1;
    seedMT(x);
    printf("\n");


    RDTSC_START(cycles_start);
    shuffle( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("normal shuffle cycles per key  %.2f \n", cycles_per_search1);

    RDTSC_START(cycles_start);
    fast_shuffle( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("fast shuffle cycles per key  %.2f \n", cycles_per_search1);

    RDTSC_START(cycles_start);
    fast_shuffle_floatapprox( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("fast shuffle with float approx cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    fisher_yates((unsigned int *) array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("from https://github.com/axel-bacher/mergeshuffle/ shuffle cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    mergeshuffle((unsigned int *)  array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("from https://github.com/axel-bacher/mergeshuffle/ mergeshuffle cycles per key  %.2f \n", cycles_per_search1);




    RDTSC_START(cycles_start);
    shuffle4( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("shuffle4 cycles per key  %.2f \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_sanders( array, array_size, 24000 );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("sanders shuffle cycles per key  %.2f  \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_prefetch2( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("prefetch 2 shuffle cycles per key  %.2f \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_prefetch4( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("prefetch 4 shuffle cycles per key  %.2f \n", cycles_per_search1);

    RDTSC_START(cycles_start);
    shuffle_prefetch8( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("prefetch 8 shuffle cycles per key  %.2f \n", cycles_per_search1);


    RDTSC_START(cycles_start);
    shuffle_prefetch16( array, array_size );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("prefetch 16 shuffle cycles per key  %.2f \n", cycles_per_search1);
    RDTSC_START(cycles_start);
    shuffle_sanders_prefetch16( array, array_size, 24000 );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (array_size);
    printf("sanders with prefetch 16 shuffle cycles per key  %.2f  \n", cycles_per_search1);
    free(array);


    return bogus;

}



int testfairness(uint32_t maxsize) {
    printf("checking your RNG.\n");
    uint32_t size;
    for(size = 2; size <maxsize; ++size) {
        uint32_t i;
        uint32_t m2 = 1 << (32- __builtin_clz(size-1));
        double ratio = (double) size / m2;
        uint32_t mask = m2 -1;
        uint32_t count = 10000;
        double predicted = (1-ratio) * count;
        uint32_t missed = 0;
        for(i = 0 ; i < count; ++i ) {
            if((fastrand() & mask) >= size) ++missed;
        }
        if((double)missed > 1.1 * predicted + 20) {
            printf("size = %d missed = %d predicted = %f\n",size, missed, predicted);
            printf("poor RNG \n");
            return -1;
        }
    }
    return 0;
}


int main( int argc, char **argv ) {
    int bogus = 0;
    size_t array_size;
    int r;
    init_gen_rand(0); // simd mersenne
    r = testfairness(1000);
    if(r == 0)
        printf ("good RNG\n");
    else return r;
    for(array_size = 4096; array_size < 2147483648; array_size*=8) {
        bogus += demo(array_size);
        printf("\n");
    }
    return bogus;
}
