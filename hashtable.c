/* minimalist C hash table originally from https://gist.github.com/tonious/1377667
The author declared it to be in the public domain. */

/* Enable strdup */
#ifndef __USE_XOPEN2K8
#define __USE_XOPEN2K8 1
#endif
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

struct entry_s {
    char *key;
    char *value;
    struct entry_s *next;
};

typedef struct entry_s entry_t;

struct hashtable_s {
    size_t size;
    struct entry_s **table;
};


typedef struct hashtable_s hashtable_t;

/* Create a new hashtable. */
hashtable_t *ht_create( size_t size ) {

    hashtable_t *hashtable = NULL;
    size_t i;

    if( size < 1 ) return NULL;

    /* Allocate the table itself. */
    if( ( hashtable = malloc( sizeof( hashtable_t ) ) ) == NULL ) {
        return NULL;
    }

    /* Allocate pointers to the head nodes. */
    if( ( hashtable->table = malloc( sizeof( entry_t * ) * size ) ) == NULL ) {
        return NULL;
    }
    for( i = 0; i < size; i++ ) {
        hashtable->table[i] = NULL;
    }

    hashtable->size = size;

    return hashtable;
}

/* Hash a string for a particular hash table. */
size_t ht_hash( hashtable_t *hashtable, char *key ) {

    size_t hashval = 0;
    size_t i = 0;
    size_t keyLength = strlen( key );
    for (i = 0; i < keyLength; ++i ) {
        hashval= 57*hashval + (size_t) key[i];
    }
    return hashval % hashtable->size;
}

void ht_describe(hashtable_t *hashtable) {
    size_t empty = 0;
    size_t justoneentry = 0;
    size_t maxentry = 0;
    size_t morethanjustoneentry = 0;

    printf("You have %zu buckets.\n",hashtable->size);
    for(size_t i = 0; i < hashtable->size; ++i) {
        entry_t *home = hashtable->table[ i ];
        if(home == NULL) empty++;
        else if (home->next == NULL) justoneentry++;
        else {
            size_t thiscount = 0;
            while( home != NULL  ) {
                home = home->next;
                thiscount ++;
            }
            morethanjustoneentry++;
            if(thiscount > maxentry) maxentry = thiscount;

        }
    }
    printf("You have %zu entries.\n",morethanjustoneentry+justoneentry);
    printf("Percentage of entries in one-entry bucket: %f.\n",justoneentry*100.0/(morethanjustoneentry+justoneentry));
}



/* Create a key-value pair. */
entry_t *ht_newpair( char *key, char *value ) {
    entry_t *newpair;

    if( ( newpair = malloc( sizeof( entry_t ) ) ) == NULL ) {
        return NULL;
    }

    if( ( newpair->key = strdup( key ) ) == NULL ) {
        return NULL;
    }

    if( ( newpair->value = strdup( value ) ) == NULL ) {
        return NULL;
    }

    newpair->next = NULL;

    return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, char *key, char *value ) {
    size_t bin = ht_hash( hashtable, key );
    entry_t *home = hashtable->table[ bin ];
    entry_t *next = home;
    if(key == NULL) return; // nah!
    if(home == NULL) {
        hashtable->table[ bin ] = ht_newpair( key, value);
        return;
    }
    while( next != NULL && next->key != NULL && strcmp( key, next->key ) != 0 ) {
        next = next->next;
    }

    /* There's already a pair.  Let's replace that string. */
    if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {
        free( next->value );
        next->value = strdup( value );
        /* Nope, could't find it.  Time to grow a pair. */
    } else {
        next = ht_newpair( key, value);
        next->next = home->next;
        home->next = next;

    }
}

/* Retrieve a key-value pair from a hash table. */
char *ht_get( hashtable_t *hashtable, char *key ) {
    size_t bin = ht_hash( hashtable, key );
    entry_t *pair;


    /* Step through the bin, looking for our value. */
    pair = hashtable->table[ bin ];
    while(pair != NULL) {
        if(strcmp( key, pair->key ) == 0) return pair-> value;
        pair = pair->next;
    }
    return NULL;
}

/* Warn the hash table that you will soon retrieve the key. Return the hash value. */
size_t ht_prefetch( hashtable_t *hashtable, char *key ) {
    size_t bin = ht_hash( hashtable, key );
    __builtin_prefetch(& hashtable->table[ bin ]);
    return bin;
}

/* Retrieve a key-value pair from a hash table. You need to call ht_prefetch first. */
char *ht_get_fast( hashtable_t *hashtable, char *key, size_t bin ) {
    entry_t *pair;


    /* Step through the bin, looking for our value. */
    pair = hashtable->table[ bin ];
    while(pair != NULL) {
        if(strcmp( key, pair->key ) == 0) return pair-> value;
        pair = pair->next;
    }
    return NULL;
}



/* Retrieve a key-value pair from a hash table. */
void ht_batch_get( hashtable_t *hashtable, char **k, size_t count, char ** answer, entry_t ** buffer ) {
    for(size_t i = 0; i < count; ++i ) {
        char * key = k[i];
        size_t bin = ht_hash( hashtable, key );
        buffer[i] = hashtable->table[ bin ];
    }
    for(size_t i = 0; i < count; ++i ) {
        entry_t *pair = buffer[i];
        char * key = k[i];

        while( pair != NULL  && strcmp( key, pair->key ) != 0 ) {
            pair = pair->next;
        }

        if( pair != NULL ) {
            answer[i] = pair->value;
        } else {
            answer[i] = NULL;
        }
    }

}

/* Retrieve a key-value pair from a hash table. Assume that whatever is found is correct. */
char *ht_get_optimistic( hashtable_t *hashtable, char *key ) {
    size_t bin = ht_hash( hashtable, key );
    entry_t *pair;


    /* Step through the bin, looking for our value. */
    pair = hashtable->table[ bin ];
    return pair-> value;
}

/* Retrieve a key-value pair from a hash table. Assume that whatever is found is correct.  */
void ht_batch_get_optimistic( hashtable_t *hashtable, char **k, size_t count, char ** answer, entry_t ** buffer ) {
    for(size_t i = 0; i < count; ++i ) {
        char * key = k[i];
        size_t bin = ht_hash( hashtable, key );
        buffer[i] = hashtable->table[ bin ];
    }
    for(size_t i = 0; i < count; ++i ) {
        entry_t *pair = buffer[i];
        char * key = k[i];
        answer[i] = pair->value;
    }

}



/****
* Rest is experimental stuff
****/

char * givemeastring(int i) {
    char * answer = malloc(16);
    int r = snprintf ( answer, 16, "%d", i );
    if(( r>=0 ) && ( r < 16)) {
        return answer;
    } else {
        printf("bad");
        free (answer);
        return NULL;
    }
}


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
    size_t N = 16777216;
    int bogus = 0;
    size_t T= 50000;
    float cycles_per_search1, cycles_per_search2, cycles_per_search3;
    char ** queries;
    char ** answer;
    size_t * hashbuffer;
    entry_t ** buffer;
    hashtable_t *hashtable = ht_create( N );

    uint64_t cycles_start, cycles_final;
    printf("creating hash table (25 percent fill ratio) \n");
    size_t actrange = N/4;
    for(size_t i = 0; i < actrange; ++i) {
        char * key = givemeastring(i);
        char * val = givemeastring(i);
        ht_set( hashtable, key, val );
        free ( key );
        free ( val );
    }
    ht_describe(hashtable);
    printf("\n");
    for(size_t Nq= 1; Nq<15; Nq++) {
        size_t total;
        printf("=== Trying a batch of %zu queries ===\n",Nq);
        queries = (char **) malloc(Nq * sizeof(char *));
        hashbuffer = (size_t *) malloc(Nq * sizeof(size_t));
        answer= (char **) malloc(Nq * sizeof(char *));

        buffer = (entry_t **) malloc(Nq * sizeof(entry_t *));
        printf("\n");

        /***
        * next part we cheat: we prefetch (outside the counted loop)
        * the values and then we query them. This should allow us to estimate
        * the time spent waiting for data.
        */
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = givemeastring(rand() % actrange);
            }

            for(size_t i = 0; i < Nq; ++i) {
                hashbuffer[i] = ht_prefetch( hashtable, queries[i] );
            }
            RDTSC_START(cycles_start);
            for(size_t i = 0; i < Nq; ++i) {
                bogus += ht_get_fast( hashtable, queries[i], hashbuffer[i] )[0];
            }
            RDTSC_FINAL(cycles_final);
            total += cycles_final - cycles_start;

        }
        cycles_per_search1 =
            total / (float) (Nq*T);
        printf("one-by-one-with-outside-prefetch (cheating) cycles %.2f \n", cycles_per_search1);
        /**
        * Next part is usual one by one
        */
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = givemeastring(rand() % actrange);
            }
            RDTSC_START(cycles_start);
            for(size_t i = 0; i < Nq; ++i) {
                bogus += ht_get( hashtable, queries[i] )[0];
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
                queries[i] = givemeastring(rand() % actrange);
            }

            RDTSC_START(cycles_start);
            ht_batch_get( hashtable, queries, Nq,  answer, buffer ) ;
            for(size_t i = 0; i < Nq; ++i) {
                bogus += answer[i][0];
            }

            RDTSC_FINAL(cycles_final);

            total += cycles_final - cycles_start;

        }
        cycles_per_search2 =
            total / (float) (Nq*T);
        printf("batch cycles %.2f \n", cycles_per_search2);
        printf("batch is more efficient by %.2f percent\n", (cycles_per_search1-cycles_per_search2)*100.0/cycles_per_search1);
        /**
        * We try a two-step approach, fetch and compute hash, then query.
        */
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = givemeastring(rand() % actrange);
            }

            RDTSC_START(cycles_start);
            for(size_t i = 0; i < Nq; ++i) {
                hashbuffer[i] = ht_prefetch( hashtable, queries[i] );
            }
            for(size_t i = 0; i < Nq; ++i) {
                bogus += ht_get_fast( hashtable, queries[i], hashbuffer[i] )[0];
            }
            RDTSC_FINAL(cycles_final);
            total += cycles_final - cycles_start;

        }
        cycles_per_search3 =
            total / (float) (Nq*T);
        printf("two-step cycles %.2f \n", cycles_per_search3);
        printf("two-step is more efficient by %.2f percent\n", (cycles_per_search1-cycles_per_search3)*100.0/cycles_per_search1);
        printf("bogus = %d \n\n\n",bogus);


        /**
        * rest are optimistic queries.
        */
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = givemeastring(rand() % N/4);
            }
            RDTSC_START(cycles_start);
            for(size_t i = 0; i < Nq; ++i) {
                bogus += ht_get_optimistic( hashtable, queries[i] )[0];
            }
            RDTSC_FINAL(cycles_final);
            total += cycles_final - cycles_start;

        }
        cycles_per_search1 =
            total / (float) (Nq*T);
        printf("one-by-one optimistic  cycles %.2f \n", cycles_per_search1);
        total = 0;
        for(size_t t=0; t<T; ++t) {
            for(size_t i = 0; i < Nq; ++i) {
                queries[i] = givemeastring(rand() % N/4);
            }

            RDTSC_START(cycles_start);
            ht_batch_get_optimistic( hashtable, queries, Nq,  answer, buffer ) ;
            for(size_t i = 0; i < Nq; ++i) {
                bogus += answer[i][0];
            }

            RDTSC_FINAL(cycles_final);

            total += cycles_final - cycles_start;

        }
        cycles_per_search2 =
            total / (float) (Nq*T);
        printf("batch optimistic cycles %.2f \n", cycles_per_search2);
        printf("batch optimistic is more efficient by %.2f percent\n", (cycles_per_search1-cycles_per_search2)*100.0/cycles_per_search1);
        printf("bogus = %d \n\n\n",bogus);






        for(size_t i = 0; i < Nq; ++i) {
            free(queries[i]);
        }
        free(queries);
        free(answer);
        free(hashbuffer);
    }
    return 0;
}
