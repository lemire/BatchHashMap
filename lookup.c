#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// gcc -DSTRING_LENGTH="4" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="rand() % 4" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="16" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="rand() % 16" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="32" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="rand() % 32" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="64" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="rand() % 64" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="128" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup
// gcc -DSTRING_LENGTH="rand() % 128" -std=gnu99 -Wall -O3 -g -march=native lookup.c -o lookup

#ifndef STRING_LENGTH
#define STRING_LENGTH rand() % 32
#endif

#ifndef NUM_STRINGS
#define NUM_STRINGS 1024 * 1024 * 16L
#endif

size_t batch_sizes[] =  {
    1, 2, 4, 6, 8, 10, 12, 14, 16, 20,
    24, 28, 32, 40, 60, 100, 200, 400, 1000
};

size_t prefetch_sizes[] =  {
    2, 4, 8, 12, 16, 24, 32, 40, 60, 100, 200
};

// no need to change unless STRING_LENGTH will exceed this
#define ALLOC_STRING_LEN (128 + 1)

// caution: too small allows individual queries to be cached
#define NUM_QUERIES (1024 * 1024)

// report minimum cycles after this number of attempts
#define TIMING_REPEATS 4


#define COUNT(array) (sizeof(array)/sizeof(*array))

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


size_t init_storage_strings(char *storage, size_t storage_size,
                            char **strings, size_t num_strings) {

    char *string = storage;
    for (size_t i = 0; i < num_strings; i++) {
        strings[i] = string;
        size_t string_length = STRING_LENGTH;
        size_t random_char = (rand() % 255) + 1;
        if (string + string_length < storage + storage_size) {
            memset(string, random_char, string_length);
            string[string_length] = 0;
            string += (string_length + 1);
        } else {
            printf("Strings buffer too small at i = %zd\n", i);
            exit(1);
        }
    }

    size_t storage_bytes_used = string - storage;
    return storage_bytes_used;
}

size_t test_baseline(uint32_t *query_list, size_t num_queries,
                     char **strings, char *output) {
    size_t length = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t query = query_list[i];
        char *string = strings[query];
        while ((output[length++] = *string++));
    }
    return length;
}
size_t test_baseline_l(uint32_t *query_list, size_t num_queries,
                     char **strings, char *output, uint8_t * lens) {
    size_t length = 0;
    for (size_t i = 0; i < num_queries; i++) {
        size_t query = query_list[i];
        uint8_t l = lens[query];
        char *string = strings[query];
        memcpy(output+length,string,l+1);
        length +=l+1;
    }
    return length;
}

// stpcpy() is a GNU version of strcpy() that returns a pointer to the end of dest
// requires compilation with -std=gnu99 or defining _POSIX_C_SOURCE >= 200809L
size_t test_stpcpy(uint32_t *query_list, size_t num_queries,
                   char **strings, char *output) {
    char *orig = output;
    for (size_t i = 0; i < num_queries; i++) {
        uint32_t query = query_list[i];
        char *string = strings[query];
        output = stpcpy(output, string) + 1;
    }
    return output - orig;
}

size_t test_prefetch(uint32_t *query_list, size_t num_queries,
                     char **strings, size_t prefetch, char *output) {
    size_t length = 0;
    for (size_t i = 0; i < num_queries; i++) {
        if (i + prefetch < num_queries) {
            uint32_t prefetch_query = query_list[i + prefetch];
            __builtin_prefetch(strings[prefetch_query]);
        }
        uint32_t query = query_list[i];
        char *string = strings[query];
        while ((output[length++] = *string++));
    }
    return length;
}


size_t test_prefetch_l(uint32_t *query_list, size_t num_queries,
                     char **strings, size_t prefetch, char *output, uint8_t * lens) {
    size_t length = 0;
    for (size_t i = 0; i < num_queries; i++) {
        if (i + prefetch < num_queries) {
            uint32_t prefetch_query = query_list[i + prefetch];
            __builtin_prefetch(strings[prefetch_query]);
        }
        uint32_t query = query_list[i];
        uint8_t l = lens[query];
        char *string = strings[query];
        memcpy(output+length,string,l+1);
        length += l+1;
    }
    return length;
}


size_t test_prefetch_stpcpy(uint32_t *query_list, size_t num_queries,
                            char **strings, size_t prefetch, char *output) {
    char *orig = output;
    for (size_t i = 0; i < num_queries; i++) {
        if (i + prefetch < num_queries) {
            uint32_t prefetch_query = query_list[i + prefetch];
            __builtin_prefetch(strings[prefetch_query]);
        }
        uint32_t query = query_list[i];
        char *string = strings[query];
        output = stpcpy(output, string) + 1;
    }
    return output - orig;
}

size_t test_batch(uint32_t *query_list, size_t num_queries,
                  size_t batch_size, char **strings,
                  char *test_out) {
    size_t test_len = 0;
    size_t i = 0;
    while (i < num_queries) {
        if (i + batch_size > num_queries) {
            batch_size = num_queries - i;
        }

        for (size_t j = i; j < i + batch_size; j++) {
            uint32_t query = query_list[j];
            char *string = strings[query];
            __builtin_prefetch(string);
        }

        for (size_t j = i; j < i + batch_size; j++) {
            uint32_t query = query_list[j];
            char *string = strings[query];
            while ((test_out[test_len++] = *string++));
        }

        i += batch_size;
    }

    return test_len;
}

size_t test_batch_stpcpy(uint32_t *query_list, size_t num_queries,
                         size_t batch_size, char **strings,
                         char *test_out) {
    char *orig_out = test_out;
    size_t i = 0;
    while (i < num_queries) {
        if (i + batch_size > num_queries) {
            batch_size = num_queries - i;
        }

        for (size_t j = i; j < i + batch_size; j++) {
            uint32_t query = query_list[j];
            char *string = strings[query];
            __builtin_prefetch(string);
        }

        for (size_t j = i; j < i + batch_size; j++) {
            uint32_t query = query_list[j];
            char *string = strings[query];
            test_out = stpcpy(test_out, string) + 1;
        }

        i += batch_size;
    }

    return test_out - orig_out;
}

int verify_output(size_t test_len, size_t good_len, char *test_out, char *good_out) {
    if (test_len != good_len) {
        printf("Length mismatch [%zd != %zd]\n", test_len, good_len);
        return 0;
    }
    for (size_t i = 0; i < good_len; i++) {
        if (test_out[i] != good_out[i]) {
            printf("Output mismatch at %zd\n", i);
            return 0;
        }
    }
    printf("\n");
    return 1;
}


#define TIMED_TEST(test)                                                \
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
        float cycles_per_query = min_diff / (float) (NUM_QUERIES);      \
        printf(" %.2f cycles/query", cycles_per_query);                 \
        fflush(NULL);                                                   \
    } while (0)


int main(int argc, char **argv) {
    if (argc > 1) srand(atoi(argv[1]));

    size_t storage_size = NUM_STRINGS * ALLOC_STRING_LEN;
    char *storage = malloc(storage_size);
    char **strings = malloc(NUM_STRINGS * sizeof(char *));
    uint8_t * strlens = malloc(NUM_STRINGS);
    init_storage_strings(storage, storage_size, strings, NUM_STRINGS);
    for (size_t i = 0; i < NUM_STRINGS; i++) {
            char *string = strings[i];
            size_t l = strlen(string);// assume it fits in a byte
            if(l > UINT8_MAX) {
              printf("you have long strings and we are in trouble here.\n");
              return -1;
            }
            strlens[i] = l;
    }
    uint32_t query_list[NUM_QUERIES];
    for (size_t i = 0; i < NUM_QUERIES; i++) {
        query_list[i] = rand() % (NUM_STRINGS);
    }

    char *good_out = malloc(ALLOC_STRING_LEN * (NUM_QUERIES));
    char *test_out = malloc(ALLOC_STRING_LEN * (NUM_QUERIES));

    size_t good_len = test_baseline(query_list, NUM_QUERIES, strings, good_out);
    size_t test_len = test_baseline(query_list, NUM_QUERIES, strings, test_out);

#define xstringify(a) #a  /* double macro for string expansion of macro */
#define stringify(a) xstringify(a)
    printf("Timing string_length='%s' num_strings='%s'\n",
           stringify(STRING_LENGTH), stringify(NUM_STRINGS));

    printf(" Baseline         ");
    memset(test_out, 0, good_len);
    TIMED_TEST(test_len = test_baseline(query_list, NUM_QUERIES, strings, test_out));
    verify_output(good_len, test_len, good_out, test_out);
    printf(" Baseline  length       ");
    memset(test_out, 0, good_len);
    TIMED_TEST(test_len = test_baseline_l(query_list, NUM_QUERIES, strings, test_out,strlens));
    verify_output(good_len, test_len, good_out, test_out);

    for (size_t i = 0; i < COUNT(batch_sizes); i++) {
        size_t batch_size = batch_sizes[i];
        if (batch_size > NUM_QUERIES) batch_size = NUM_QUERIES;
        memset(test_out, 0, good_len);
        printf(" Batch %4zd       ", batch_size);
        TIMED_TEST(test_len = test_batch(query_list, NUM_QUERIES, batch_size, strings, test_out));
        verify_output(good_len, test_len, good_out, test_out);
    }

    printf(" stpcpy           ");
    memset(test_out, 0, good_len);
    TIMED_TEST(test_len = test_stpcpy(query_list, NUM_QUERIES, strings, test_out));
    verify_output(good_len, test_len, good_out, test_out);

    for (size_t i = 0; i < COUNT(batch_sizes); i++) {
        size_t batch_size = batch_sizes[i];
        if (batch_size > NUM_QUERIES) batch_size = NUM_QUERIES;
        memset(test_out, 0, good_len);
        printf(" Batch %4zd stpcpy", batch_size);
        TIMED_TEST(test_len = test_batch_stpcpy(query_list, NUM_QUERIES, batch_size, strings, test_out));
        verify_output(good_len, test_len, good_out, test_out);
    }

    for (size_t i = 0; i < COUNT(prefetch_sizes); i++) {
        size_t prefetch = prefetch_sizes[i];
        printf(" Prefetch %3zd       ", prefetch);
        TIMED_TEST(test_len = test_prefetch(query_list, NUM_QUERIES, strings, prefetch, test_out));
        verify_output(good_len, test_len, good_out, test_out);
    }
    for (size_t i = 0; i < COUNT(prefetch_sizes); i++) {
        size_t prefetch = prefetch_sizes[i];
        printf(" Prefetch %3zd l      ", prefetch);
        TIMED_TEST(test_len = test_prefetch_l(query_list, NUM_QUERIES, strings, prefetch, test_out, strlens));
        verify_output(good_len, test_len, good_out, test_out);
    }

    for (size_t i = 0; i < COUNT(prefetch_sizes); i++) {
        size_t prefetch = prefetch_sizes[i];
        printf(" Prefetch %3zd stpcpy", prefetch);
        TIMED_TEST(test_len = test_prefetch_stpcpy(query_list, NUM_QUERIES, strings, prefetch, test_out));
        verify_output(good_len, test_len, good_out, test_out);
    }
}
