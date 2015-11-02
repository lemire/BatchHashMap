
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

struct bitset_s {
    uint64_t *array;
    size_t arraysize;
};

typedef struct bitset_s bitset_t;


/* Create a new bitset able to contain size bits. Return NULL in case of failure. */
bitset_t *bitset_create( size_t size ) {
  bitset_t *bitset = NULL;
  /* Allocate the bitset itself. */
  if( ( bitset = malloc( sizeof( bitset_t ) ) ) == NULL ) {
      return NULL;
  }
  bitset->arraysize = (size + sizeof(uint64_t) * 8 - 1) / (sizeof(uint64_t) * 8);
  if ((bitset->array = (uint64_t *) malloc(sizeof(uint64_t) * bitset->arraysize)) == NULL) {
    free( bitset);
    return NULL;
  }
  memset(bitset->array,0,sizeof(uint64_t) * bitset->arraysize);
  return bitset;
}

/* Free memory. */
void bitset_free(bitset_t *bitset) {
  free(bitset->array);
  free(bitset);
}

/* Resize the bitset. Return 1 in case of success, 0 for failure. */
int bitset_resize( bitset_t *bitset,  size_t newarraysize ) {
  size_t smallest = newarraysize < bitset->arraysize ? newarraysize : bitset->arraysize;
  uint64_t *newarray;
  if ((newarray = (uint64_t *) malloc(sizeof(uint64_t) * newarraysize)) == NULL) {
    return 0;
  }
  memcpy(newarray,bitset->array,smallest * sizeof(uint64_t));
  if(newarraysize > smallest)
    memset(bitset->array + smallest,0,sizeof(uint64_t) * (newarraysize - smallest));
  free(bitset->array);
  bitset->array = newarray;
  bitset->arraysize = newarraysize;
  return 1; // success!
}

void bitset_describe(bitset_t *bitset) {
  printf("Bitset uses %zu bytes or %zu kB.", bitset->arraysize*sizeof(uint64_t),(bitset->arraysize*sizeof(uint64_t)+1023)/1024);
}


/* Set the ith bit. Return 1 in case of success, 0 for failure. */
int bitset_set(bitset_t *bitset,  size_t i ) {
  if ((i >> 6) >= bitset->arraysize) {
    size_t whatisneeded = ((i+64)>>6);
    if( bitset_resize(bitset,  whatisneeded) == 0) {
        return 0;
    }
  }
  bitset->array[i >> 6] |= ((uint64_t)1) << (i % 64);
  return 1; // success!
}

/* Get the value of the ith bit.  */
int bitset_get(bitset_t *bitset,  size_t i ) {
  uint64_t w = ((i >> 6) >= bitset->arraysize)  ? 0 :  bitset->array[i >> 6];
  return ( w & ( ((uint64_t)1) << (i % 64))) != 0 ;
}

/* Get the value of the ith bit.  */
int bitset_bogus_get(bitset_t *bitset,  size_t i ) {
  return ( 123 & ( ((uint64_t)1) << (i % 64))) != 0 ;
}

/* Get the values of the ith bits in batch. Only up to 32 bits are queried, others are ignored  */
void bitset_batch_get(bitset_t *bitset,  size_t * i,  size_t count, int * answers ) {
  uint64_t words[32];
  if(count > 32) count = 32;
  for(size_t k = 0 ; k < count ; k++) {
    size_t ti = i[k];
    words[k] = ((ti >> 6) >= bitset->arraysize)  ? 0 :  bitset->array[ti >> 6];
  }
  for(size_t k = 0 ; k < count ; k++) {
    size_t ti = i[k];
    uint64_t mask = ((uint64_t)1) << (ti % 64);
    uint64_t w = words[k];
    answers[k] = (( w & mask) != 0);
  }
}




/////////////////




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

// set final cycles to current 64-bit rdtsc value
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

int main( int argc, char **argv ) {
    size_t N = 16777216*64;
    int bogus = 0;
    size_t T= 500000;
    float cycles_per_search1, cycles_per_search2;
    size_t * queries;
    int * answer;
    bitset_t *bitset = bitset_create( N );

    uint64_t cycles_start, cycles_final;
    printf("populating bitset \n");
    size_t actrange = N/8;
    for(size_t i = 0; i < actrange; ++i) {
      bitset_set(bitset, rand() % N);
    }
    bitset_describe(bitset);
    printf("\n");
    for(size_t Nq= 1; Nq<32; Nq++) {
        size_t total;
        printf("=== Trying a batch of %zu queries ===\n",Nq);
        queries = (size_t *) malloc(Nq * sizeof(size_t));
        answer= (int *) malloc(Nq * sizeof(int));
        printf("\n");

        /**
        * Next part is usual one by one
        */
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = rand() % N;
            }
            RDTSC_START(cycles_start);
            for(size_t i = 0; i < Nq; ++i) {
                bogus += bitset_get( bitset, queries[i] );
            }
            RDTSC_FINAL(cycles_final);
            total += cycles_final - cycles_start;

        }
        cycles_per_search1 =
            total / (float) (Nq*T);
        printf("one-by-one cycles %.2f \n", cycles_per_search1);


        /**
        * Next are complicated batched queries.
        */
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = rand() % N;
            }

            RDTSC_START(cycles_start);
            bitset_batch_get( bitset, queries, Nq,  answer ) ;
            for(size_t i = 0; i < Nq; ++i) {
                bogus += answer[i];
            }

            RDTSC_FINAL(cycles_final);

            total += cycles_final - cycles_start;

        }
        cycles_per_search2 =
            total / (float) (Nq*T);
        printf("batch cycles %.2f \n", cycles_per_search2);
        printf("batch is more efficient by %.2f percent\n", (cycles_per_search1-cycles_per_search2)*100.0/cycles_per_search1);
        printf("bogus = %d \n",bogus);


        free(queries);
        free(answer);
    }
    return 0;
}
