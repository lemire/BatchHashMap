/*//////////
// TODO: use SIMD Mersenne Twister.
/////////////*/
#include <x86intrin.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <immintrin.h>

#ifdef IACA
#include </opt/intel/iaca/include/iacaMarks.h>
#else
#define IACA_START
#define IACA_END
#endif

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

// bad, just for testing
int getRandomBit() {
    return fastrand() & 1;
}

// bad, just for testing
int getRandomByte() {
    return fastrand() & 0xFF;
}

void swap(uint32_t * array, uint32_t i, uint32_t j) {
    uint32_t x = array[i];
    array[i] = array[j];
    array[j] = x;
}

uint32_t inplace_onepass_shuffle(uint32_t * array, size_t length) {
    int32_t boundary = -1;
    uint32_t i;
    for(i = 0; i < length; ++i) {
        int coin = getRandomBit();
        if(coin) {
            boundary++;
            swap(array, i,boundary);
        }
    }
    return boundary + 1;
}


int shufflemask [256*8]  __attribute__((aligned(0x100)))  = {
    0,1,2,3,4,5,6,7,/* 0*/
    0,   1,2,3,4,5,6,7,/* 1*/
    1,   0,2,3,4,5,6,7,/* 2*/
    0,1,   2,3,4,5,6,7,/* 3*/
    2,   0,1,3,4,5,6,7,/* 4*/
    0,2,   1,3,4,5,6,7,/* 5*/
    1,2,   0,3,4,5,6,7,/* 6*/
    0,1,2,   3,4,5,6,7,/* 7*/
    3,   0,1,2,4,5,6,7,/* 8*/
    0,3,   1,2,4,5,6,7,/* 9*/
    1,3,   0,2,4,5,6,7,/* 10*/
    0,1,3,   2,4,5,6,7,/* 11*/
    2,3,   0,1,4,5,6,7,/* 12*/
    0,2,3,   1,4,5,6,7,/* 13*/
    1,2,3,   0,4,5,6,7,/* 14*/
    0,1,2,3,   4,5,6,7,/* 15*/
    4,   0,1,2,3,5,6,7,/* 16*/
    0,4,   1,2,3,5,6,7,/* 17*/
    1,4,   0,2,3,5,6,7,/* 18*/
    0,1,4,   2,3,5,6,7,/* 19*/
    2,4,   0,1,3,5,6,7,/* 20*/
    0,2,4,   1,3,5,6,7,/* 21*/
    1,2,4,   0,3,5,6,7,/* 22*/
    0,1,2,4,   3,5,6,7,/* 23*/
    3,4,   0,1,2,5,6,7,/* 24*/
    0,3,4,   1,2,5,6,7,/* 25*/
    1,3,4,   0,2,5,6,7,/* 26*/
    0,1,3,4,   2,5,6,7,/* 27*/
    2,3,4,   0,1,5,6,7,/* 28*/
    0,2,3,4,   1,5,6,7,/* 29*/
    1,2,3,4,   0,5,6,7,/* 30*/
    0,1,2,3,4,   5,6,7,/* 31*/
    5,   0,1,2,3,4,6,7,/* 32*/
    0,5,   1,2,3,4,6,7,/* 33*/
    1,5,   0,2,3,4,6,7,/* 34*/
    0,1,5,   2,3,4,6,7,/* 35*/
    2,5,   0,1,3,4,6,7,/* 36*/
    0,2,5,   1,3,4,6,7,/* 37*/
    1,2,5,   0,3,4,6,7,/* 38*/
    0,1,2,5,   3,4,6,7,/* 39*/
    3,5,   0,1,2,4,6,7,/* 40*/
    0,3,5,   1,2,4,6,7,/* 41*/
    1,3,5,   0,2,4,6,7,/* 42*/
    0,1,3,5,   2,4,6,7,/* 43*/
    2,3,5,   0,1,4,6,7,/* 44*/
    0,2,3,5,   1,4,6,7,/* 45*/
    1,2,3,5,   0,4,6,7,/* 46*/
    0,1,2,3,5,   4,6,7,/* 47*/
    4,5,   0,1,2,3,6,7,/* 48*/
    0,4,5,   1,2,3,6,7,/* 49*/
    1,4,5,   0,2,3,6,7,/* 50*/
    0,1,4,5,   2,3,6,7,/* 51*/
    2,4,5,   0,1,3,6,7,/* 52*/
    0,2,4,5,   1,3,6,7,/* 53*/
    1,2,4,5,   0,3,6,7,/* 54*/
    0,1,2,4,5,   3,6,7,/* 55*/
    3,4,5,   0,1,2,6,7,/* 56*/
    0,3,4,5,   1,2,6,7,/* 57*/
    1,3,4,5,   0,2,6,7,/* 58*/
    0,1,3,4,5,   2,6,7,/* 59*/
    2,3,4,5,   0,1,6,7,/* 60*/
    0,2,3,4,5,   1,6,7,/* 61*/
    1,2,3,4,5,   0,6,7,/* 62*/
    0,1,2,3,4,5,   6,7,/* 63*/
    6,   0,1,2,3,4,5,7,/* 64*/
    0,6,   1,2,3,4,5,7,/* 65*/
    1,6,   0,2,3,4,5,7,/* 66*/
    0,1,6,   2,3,4,5,7,/* 67*/
    2,6,   0,1,3,4,5,7,/* 68*/
    0,2,6,   1,3,4,5,7,/* 69*/
    1,2,6,   0,3,4,5,7,/* 70*/
    0,1,2,6,   3,4,5,7,/* 71*/
    3,6,   0,1,2,4,5,7,/* 72*/
    0,3,6,   1,2,4,5,7,/* 73*/
    1,3,6,   0,2,4,5,7,/* 74*/
    0,1,3,6,   2,4,5,7,/* 75*/
    2,3,6,   0,1,4,5,7,/* 76*/
    0,2,3,6,   1,4,5,7,/* 77*/
    1,2,3,6,   0,4,5,7,/* 78*/
    0,1,2,3,6,   4,5,7,/* 79*/
    4,6,   0,1,2,3,5,7,/* 80*/
    0,4,6,   1,2,3,5,7,/* 81*/
    1,4,6,   0,2,3,5,7,/* 82*/
    0,1,4,6,   2,3,5,7,/* 83*/
    2,4,6,   0,1,3,5,7,/* 84*/
    0,2,4,6,   1,3,5,7,/* 85*/
    1,2,4,6,   0,3,5,7,/* 86*/
    0,1,2,4,6,   3,5,7,/* 87*/
    3,4,6,   0,1,2,5,7,/* 88*/
    0,3,4,6,   1,2,5,7,/* 89*/
    1,3,4,6,   0,2,5,7,/* 90*/
    0,1,3,4,6,   2,5,7,/* 91*/
    2,3,4,6,   0,1,5,7,/* 92*/
    0,2,3,4,6,   1,5,7,/* 93*/
    1,2,3,4,6,   0,5,7,/* 94*/
    0,1,2,3,4,6,   5,7,/* 95*/
    5,6,   0,1,2,3,4,7,/* 96*/
    0,5,6,   1,2,3,4,7,/* 97*/
    1,5,6,   0,2,3,4,7,/* 98*/
    0,1,5,6,   2,3,4,7,/* 99*/
    2,5,6,   0,1,3,4,7,/* 100*/
    0,2,5,6,   1,3,4,7,/* 101*/
    1,2,5,6,   0,3,4,7,/* 102*/
    0,1,2,5,6,   3,4,7,/* 103*/
    3,5,6,   0,1,2,4,7,/* 104*/
    0,3,5,6,   1,2,4,7,/* 105*/
    1,3,5,6,   0,2,4,7,/* 106*/
    0,1,3,5,6,   2,4,7,/* 107*/
    2,3,5,6,   0,1,4,7,/* 108*/
    0,2,3,5,6,   1,4,7,/* 109*/
    1,2,3,5,6,   0,4,7,/* 110*/
    0,1,2,3,5,6,   4,7,/* 111*/
    4,5,6,   0,1,2,3,7,/* 112*/
    0,4,5,6,   1,2,3,7,/* 113*/
    1,4,5,6,   0,2,3,7,/* 114*/
    0,1,4,5,6,   2,3,7,/* 115*/
    2,4,5,6,   0,1,3,7,/* 116*/
    0,2,4,5,6,   1,3,7,/* 117*/
    1,2,4,5,6,   0,3,7,/* 118*/
    0,1,2,4,5,6,   3,7,/* 119*/
    3,4,5,6,   0,1,2,7,/* 120*/
    0,3,4,5,6,   1,2,7,/* 121*/
    1,3,4,5,6,   0,2,7,/* 122*/
    0,1,3,4,5,6,   2,7,/* 123*/
    2,3,4,5,6,   0,1,7,/* 124*/
    0,2,3,4,5,6,   1,7,/* 125*/
    1,2,3,4,5,6,   0,7,/* 126*/
    0,1,2,3,4,5,6,   7,/* 127*/
    7,   0,1,2,3,4,5,6,/* 128*/
    0,7,   1,2,3,4,5,6,/* 129*/
    1,7,   0,2,3,4,5,6,/* 130*/
    0,1,7,   2,3,4,5,6,/* 131*/
    2,7,   0,1,3,4,5,6,/* 132*/
    0,2,7,   1,3,4,5,6,/* 133*/
    1,2,7,   0,3,4,5,6,/* 134*/
    0,1,2,7,   3,4,5,6,/* 135*/
    3,7,   0,1,2,4,5,6,/* 136*/
    0,3,7,   1,2,4,5,6,/* 137*/
    1,3,7,   0,2,4,5,6,/* 138*/
    0,1,3,7,   2,4,5,6,/* 139*/
    2,3,7,   0,1,4,5,6,/* 140*/
    0,2,3,7,   1,4,5,6,/* 141*/
    1,2,3,7,   0,4,5,6,/* 142*/
    0,1,2,3,7,   4,5,6,/* 143*/
    4,7,   0,1,2,3,5,6,/* 144*/
    0,4,7,   1,2,3,5,6,/* 145*/
    1,4,7,   0,2,3,5,6,/* 146*/
    0,1,4,7,   2,3,5,6,/* 147*/
    2,4,7,   0,1,3,5,6,/* 148*/
    0,2,4,7,   1,3,5,6,/* 149*/
    1,2,4,7,   0,3,5,6,/* 150*/
    0,1,2,4,7,   3,5,6,/* 151*/
    3,4,7,   0,1,2,5,6,/* 152*/
    0,3,4,7,   1,2,5,6,/* 153*/
    1,3,4,7,   0,2,5,6,/* 154*/
    0,1,3,4,7,   2,5,6,/* 155*/
    2,3,4,7,   0,1,5,6,/* 156*/
    0,2,3,4,7,   1,5,6,/* 157*/
    1,2,3,4,7,   0,5,6,/* 158*/
    0,1,2,3,4,7,   5,6,/* 159*/
    5,7,   0,1,2,3,4,6,/* 160*/
    0,5,7,   1,2,3,4,6,/* 161*/
    1,5,7,   0,2,3,4,6,/* 162*/
    0,1,5,7,   2,3,4,6,/* 163*/
    2,5,7,   0,1,3,4,6,/* 164*/
    0,2,5,7,   1,3,4,6,/* 165*/
    1,2,5,7,   0,3,4,6,/* 166*/
    0,1,2,5,7,   3,4,6,/* 167*/
    3,5,7,   0,1,2,4,6,/* 168*/
    0,3,5,7,   1,2,4,6,/* 169*/
    1,3,5,7,   0,2,4,6,/* 170*/
    0,1,3,5,7,   2,4,6,/* 171*/
    2,3,5,7,   0,1,4,6,/* 172*/
    0,2,3,5,7,   1,4,6,/* 173*/
    1,2,3,5,7,   0,4,6,/* 174*/
    0,1,2,3,5,7,   4,6,/* 175*/
    4,5,7,   0,1,2,3,6,/* 176*/
    0,4,5,7,   1,2,3,6,/* 177*/
    1,4,5,7,   0,2,3,6,/* 178*/
    0,1,4,5,7,   2,3,6,/* 179*/
    2,4,5,7,   0,1,3,6,/* 180*/
    0,2,4,5,7,   1,3,6,/* 181*/
    1,2,4,5,7,   0,3,6,/* 182*/
    0,1,2,4,5,7,   3,6,/* 183*/
    3,4,5,7,   0,1,2,6,/* 184*/
    0,3,4,5,7,   1,2,6,/* 185*/
    1,3,4,5,7,   0,2,6,/* 186*/
    0,1,3,4,5,7,   2,6,/* 187*/
    2,3,4,5,7,   0,1,6,/* 188*/
    0,2,3,4,5,7,   1,6,/* 189*/
    1,2,3,4,5,7,   0,6,/* 190*/
    0,1,2,3,4,5,7,   6,/* 191*/
    6,7,   0,1,2,3,4,5,/* 192*/
    0,6,7,   1,2,3,4,5,/* 193*/
    1,6,7,   0,2,3,4,5,/* 194*/
    0,1,6,7,   2,3,4,5,/* 195*/
    2,6,7,   0,1,3,4,5,/* 196*/
    0,2,6,7,   1,3,4,5,/* 197*/
    1,2,6,7,   0,3,4,5,/* 198*/
    0,1,2,6,7,   3,4,5,/* 199*/
    3,6,7,   0,1,2,4,5,/* 200*/
    0,3,6,7,   1,2,4,5,/* 201*/
    1,3,6,7,   0,2,4,5,/* 202*/
    0,1,3,6,7,   2,4,5,/* 203*/
    2,3,6,7,   0,1,4,5,/* 204*/
    0,2,3,6,7,   1,4,5,/* 205*/
    1,2,3,6,7,   0,4,5,/* 206*/
    0,1,2,3,6,7,   4,5,/* 207*/
    4,6,7,   0,1,2,3,5,/* 208*/
    0,4,6,7,   1,2,3,5,/* 209*/
    1,4,6,7,   0,2,3,5,/* 210*/
    0,1,4,6,7,   2,3,5,/* 211*/
    2,4,6,7,   0,1,3,5,/* 212*/
    0,2,4,6,7,   1,3,5,/* 213*/
    1,2,4,6,7,   0,3,5,/* 214*/
    0,1,2,4,6,7,   3,5,/* 215*/
    3,4,6,7,   0,1,2,5,/* 216*/
    0,3,4,6,7,   1,2,5,/* 217*/
    1,3,4,6,7,   0,2,5,/* 218*/
    0,1,3,4,6,7,   2,5,/* 219*/
    2,3,4,6,7,   0,1,5,/* 220*/
    0,2,3,4,6,7,   1,5,/* 221*/
    1,2,3,4,6,7,   0,5,/* 222*/
    0,1,2,3,4,6,7,   5,/* 223*/
    5,6,7,   0,1,2,3,4,/* 224*/
    0,5,6,7,   1,2,3,4,/* 225*/
    1,5,6,7,   0,2,3,4,/* 226*/
    0,1,5,6,7,   2,3,4,/* 227*/
    2,5,6,7,   0,1,3,4,/* 228*/
    0,2,5,6,7,   1,3,4,/* 229*/
    1,2,5,6,7,   0,3,4,/* 230*/
    0,1,2,5,6,7,   3,4,/* 231*/
    3,5,6,7,   0,1,2,4,/* 232*/
    0,3,5,6,7,   1,2,4,/* 233*/
    1,3,5,6,7,   0,2,4,/* 234*/
    0,1,3,5,6,7,   2,4,/* 235*/
    2,3,5,6,7,   0,1,4,/* 236*/
    0,2,3,5,6,7,   1,4,/* 237*/
    1,2,3,5,6,7,   0,4,/* 238*/
    0,1,2,3,5,6,7,   4,/* 239*/
    4,5,6,7,   0,1,2,3,/* 240*/
    0,4,5,6,7,   1,2,3,/* 241*/
    1,4,5,6,7,   0,2,3,/* 242*/
    0,1,4,5,6,7,   2,3,/* 243*/
    2,4,5,6,7,   0,1,3,/* 244*/
    0,2,4,5,6,7,   1,3,/* 245*/
    1,2,4,5,6,7,   0,3,/* 246*/
    0,1,2,4,5,6,7,   3,/* 247*/
    3,4,5,6,7,   0,1,2,/* 248*/
    0,3,4,5,6,7,   1,2,/* 249*/
    1,3,4,5,6,7,   0,2,/* 250*/
    0,1,3,4,5,6,7,   2,/* 251*/
    2,3,4,5,6,7,   0,1,/* 252*/
    0,2,3,4,5,6,7,   1,/* 253*/
    1,2,3,4,5,6,7,   0,/* 254*/
    0,1,2,3,4,5,6,7,   /* 255*/
};

int reverseshufflemask [256*8]  __attribute__((aligned(0x100)))  = {
    0,1,2,3,4,5,6,7,   /* 0*/
    1,2,3,4,5,6,7,   0,/* 1*/
    0,2,3,4,5,6,7,   1,/* 2*/
    2,3,4,5,6,7,   0,1,/* 3*/
    0,1,3,4,5,6,7,   2,/* 4*/
    1,3,4,5,6,7,   0,2,/* 5*/
    0,3,4,5,6,7,   1,2,/* 6*/
    3,4,5,6,7,   0,1,2,/* 7*/
    0,1,2,4,5,6,7,   3,/* 8*/
    1,2,4,5,6,7,   0,3,/* 9*/
    0,2,4,5,6,7,   1,3,/* 10*/
    2,4,5,6,7,   0,1,3,/* 11*/
    0,1,4,5,6,7,   2,3,/* 12*/
    1,4,5,6,7,   0,2,3,/* 13*/
    0,4,5,6,7,   1,2,3,/* 14*/
    4,5,6,7,   0,1,2,3,/* 15*/
    0,1,2,3,5,6,7,   4,/* 16*/
    1,2,3,5,6,7,   0,4,/* 17*/
    0,2,3,5,6,7,   1,4,/* 18*/
    2,3,5,6,7,   0,1,4,/* 19*/
    0,1,3,5,6,7,   2,4,/* 20*/
    1,3,5,6,7,   0,2,4,/* 21*/
    0,3,5,6,7,   1,2,4,/* 22*/
    3,5,6,7,   0,1,2,4,/* 23*/
    0,1,2,5,6,7,   3,4,/* 24*/
    1,2,5,6,7,   0,3,4,/* 25*/
    0,2,5,6,7,   1,3,4,/* 26*/
    2,5,6,7,   0,1,3,4,/* 27*/
    0,1,5,6,7,   2,3,4,/* 28*/
    1,5,6,7,   0,2,3,4,/* 29*/
    0,5,6,7,   1,2,3,4,/* 30*/
    5,6,7,   0,1,2,3,4,/* 31*/
    0,1,2,3,4,6,7,   5,/* 32*/
    1,2,3,4,6,7,   0,5,/* 33*/
    0,2,3,4,6,7,   1,5,/* 34*/
    2,3,4,6,7,   0,1,5,/* 35*/
    0,1,3,4,6,7,   2,5,/* 36*/
    1,3,4,6,7,   0,2,5,/* 37*/
    0,3,4,6,7,   1,2,5,/* 38*/
    3,4,6,7,   0,1,2,5,/* 39*/
    0,1,2,4,6,7,   3,5,/* 40*/
    1,2,4,6,7,   0,3,5,/* 41*/
    0,2,4,6,7,   1,3,5,/* 42*/
    2,4,6,7,   0,1,3,5,/* 43*/
    0,1,4,6,7,   2,3,5,/* 44*/
    1,4,6,7,   0,2,3,5,/* 45*/
    0,4,6,7,   1,2,3,5,/* 46*/
    4,6,7,   0,1,2,3,5,/* 47*/
    0,1,2,3,6,7,   4,5,/* 48*/
    1,2,3,6,7,   0,4,5,/* 49*/
    0,2,3,6,7,   1,4,5,/* 50*/
    2,3,6,7,   0,1,4,5,/* 51*/
    0,1,3,6,7,   2,4,5,/* 52*/
    1,3,6,7,   0,2,4,5,/* 53*/
    0,3,6,7,   1,2,4,5,/* 54*/
    3,6,7,   0,1,2,4,5,/* 55*/
    0,1,2,6,7,   3,4,5,/* 56*/
    1,2,6,7,   0,3,4,5,/* 57*/
    0,2,6,7,   1,3,4,5,/* 58*/
    2,6,7,   0,1,3,4,5,/* 59*/
    0,1,6,7,   2,3,4,5,/* 60*/
    1,6,7,   0,2,3,4,5,/* 61*/
    0,6,7,   1,2,3,4,5,/* 62*/
    6,7,   0,1,2,3,4,5,/* 63*/
    0,1,2,3,4,5,7,   6,/* 64*/
    1,2,3,4,5,7,   0,6,/* 65*/
    0,2,3,4,5,7,   1,6,/* 66*/
    2,3,4,5,7,   0,1,6,/* 67*/
    0,1,3,4,5,7,   2,6,/* 68*/
    1,3,4,5,7,   0,2,6,/* 69*/
    0,3,4,5,7,   1,2,6,/* 70*/
    3,4,5,7,   0,1,2,6,/* 71*/
    0,1,2,4,5,7,   3,6,/* 72*/
    1,2,4,5,7,   0,3,6,/* 73*/
    0,2,4,5,7,   1,3,6,/* 74*/
    2,4,5,7,   0,1,3,6,/* 75*/
    0,1,4,5,7,   2,3,6,/* 76*/
    1,4,5,7,   0,2,3,6,/* 77*/
    0,4,5,7,   1,2,3,6,/* 78*/
    4,5,7,   0,1,2,3,6,/* 79*/
    0,1,2,3,5,7,   4,6,/* 80*/
    1,2,3,5,7,   0,4,6,/* 81*/
    0,2,3,5,7,   1,4,6,/* 82*/
    2,3,5,7,   0,1,4,6,/* 83*/
    0,1,3,5,7,   2,4,6,/* 84*/
    1,3,5,7,   0,2,4,6,/* 85*/
    0,3,5,7,   1,2,4,6,/* 86*/
    3,5,7,   0,1,2,4,6,/* 87*/
    0,1,2,5,7,   3,4,6,/* 88*/
    1,2,5,7,   0,3,4,6,/* 89*/
    0,2,5,7,   1,3,4,6,/* 90*/
    2,5,7,   0,1,3,4,6,/* 91*/
    0,1,5,7,   2,3,4,6,/* 92*/
    1,5,7,   0,2,3,4,6,/* 93*/
    0,5,7,   1,2,3,4,6,/* 94*/
    5,7,   0,1,2,3,4,6,/* 95*/
    0,1,2,3,4,7,   5,6,/* 96*/
    1,2,3,4,7,   0,5,6,/* 97*/
    0,2,3,4,7,   1,5,6,/* 98*/
    2,3,4,7,   0,1,5,6,/* 99*/
    0,1,3,4,7,   2,5,6,/* 100*/
    1,3,4,7,   0,2,5,6,/* 101*/
    0,3,4,7,   1,2,5,6,/* 102*/
    3,4,7,   0,1,2,5,6,/* 103*/
    0,1,2,4,7,   3,5,6,/* 104*/
    1,2,4,7,   0,3,5,6,/* 105*/
    0,2,4,7,   1,3,5,6,/* 106*/
    2,4,7,   0,1,3,5,6,/* 107*/
    0,1,4,7,   2,3,5,6,/* 108*/
    1,4,7,   0,2,3,5,6,/* 109*/
    0,4,7,   1,2,3,5,6,/* 110*/
    4,7,   0,1,2,3,5,6,/* 111*/
    0,1,2,3,7,   4,5,6,/* 112*/
    1,2,3,7,   0,4,5,6,/* 113*/
    0,2,3,7,   1,4,5,6,/* 114*/
    2,3,7,   0,1,4,5,6,/* 115*/
    0,1,3,7,   2,4,5,6,/* 116*/
    1,3,7,   0,2,4,5,6,/* 117*/
    0,3,7,   1,2,4,5,6,/* 118*/
    3,7,   0,1,2,4,5,6,/* 119*/
    0,1,2,7,   3,4,5,6,/* 120*/
    1,2,7,   0,3,4,5,6,/* 121*/
    0,2,7,   1,3,4,5,6,/* 122*/
    2,7,   0,1,3,4,5,6,/* 123*/
    0,1,7,   2,3,4,5,6,/* 124*/
    1,7,   0,2,3,4,5,6,/* 125*/
    0,7,   1,2,3,4,5,6,/* 126*/
    7,   0,1,2,3,4,5,6,/* 127*/
    0,1,2,3,4,5,6,   7,/* 128*/
    1,2,3,4,5,6,   0,7,/* 129*/
    0,2,3,4,5,6,   1,7,/* 130*/
    2,3,4,5,6,   0,1,7,/* 131*/
    0,1,3,4,5,6,   2,7,/* 132*/
    1,3,4,5,6,   0,2,7,/* 133*/
    0,3,4,5,6,   1,2,7,/* 134*/
    3,4,5,6,   0,1,2,7,/* 135*/
    0,1,2,4,5,6,   3,7,/* 136*/
    1,2,4,5,6,   0,3,7,/* 137*/
    0,2,4,5,6,   1,3,7,/* 138*/
    2,4,5,6,   0,1,3,7,/* 139*/
    0,1,4,5,6,   2,3,7,/* 140*/
    1,4,5,6,   0,2,3,7,/* 141*/
    0,4,5,6,   1,2,3,7,/* 142*/
    4,5,6,   0,1,2,3,7,/* 143*/
    0,1,2,3,5,6,   4,7,/* 144*/
    1,2,3,5,6,   0,4,7,/* 145*/
    0,2,3,5,6,   1,4,7,/* 146*/
    2,3,5,6,   0,1,4,7,/* 147*/
    0,1,3,5,6,   2,4,7,/* 148*/
    1,3,5,6,   0,2,4,7,/* 149*/
    0,3,5,6,   1,2,4,7,/* 150*/
    3,5,6,   0,1,2,4,7,/* 151*/
    0,1,2,5,6,   3,4,7,/* 152*/
    1,2,5,6,   0,3,4,7,/* 153*/
    0,2,5,6,   1,3,4,7,/* 154*/
    2,5,6,   0,1,3,4,7,/* 155*/
    0,1,5,6,   2,3,4,7,/* 156*/
    1,5,6,   0,2,3,4,7,/* 157*/
    0,5,6,   1,2,3,4,7,/* 158*/
    5,6,   0,1,2,3,4,7,/* 159*/
    0,1,2,3,4,6,   5,7,/* 160*/
    1,2,3,4,6,   0,5,7,/* 161*/
    0,2,3,4,6,   1,5,7,/* 162*/
    2,3,4,6,   0,1,5,7,/* 163*/
    0,1,3,4,6,   2,5,7,/* 164*/
    1,3,4,6,   0,2,5,7,/* 165*/
    0,3,4,6,   1,2,5,7,/* 166*/
    3,4,6,   0,1,2,5,7,/* 167*/
    0,1,2,4,6,   3,5,7,/* 168*/
    1,2,4,6,   0,3,5,7,/* 169*/
    0,2,4,6,   1,3,5,7,/* 170*/
    2,4,6,   0,1,3,5,7,/* 171*/
    0,1,4,6,   2,3,5,7,/* 172*/
    1,4,6,   0,2,3,5,7,/* 173*/
    0,4,6,   1,2,3,5,7,/* 174*/
    4,6,   0,1,2,3,5,7,/* 175*/
    0,1,2,3,6,   4,5,7,/* 176*/
    1,2,3,6,   0,4,5,7,/* 177*/
    0,2,3,6,   1,4,5,7,/* 178*/
    2,3,6,   0,1,4,5,7,/* 179*/
    0,1,3,6,   2,4,5,7,/* 180*/
    1,3,6,   0,2,4,5,7,/* 181*/
    0,3,6,   1,2,4,5,7,/* 182*/
    3,6,   0,1,2,4,5,7,/* 183*/
    0,1,2,6,   3,4,5,7,/* 184*/
    1,2,6,   0,3,4,5,7,/* 185*/
    0,2,6,   1,3,4,5,7,/* 186*/
    2,6,   0,1,3,4,5,7,/* 187*/
    0,1,6,   2,3,4,5,7,/* 188*/
    1,6,   0,2,3,4,5,7,/* 189*/
    0,6,   1,2,3,4,5,7,/* 190*/
    6,   0,1,2,3,4,5,7,/* 191*/
    0,1,2,3,4,5,   6,7,/* 192*/
    1,2,3,4,5,   0,6,7,/* 193*/
    0,2,3,4,5,   1,6,7,/* 194*/
    2,3,4,5,   0,1,6,7,/* 195*/
    0,1,3,4,5,   2,6,7,/* 196*/
    1,3,4,5,   0,2,6,7,/* 197*/
    0,3,4,5,   1,2,6,7,/* 198*/
    3,4,5,   0,1,2,6,7,/* 199*/
    0,1,2,4,5,   3,6,7,/* 200*/
    1,2,4,5,   0,3,6,7,/* 201*/
    0,2,4,5,   1,3,6,7,/* 202*/
    2,4,5,   0,1,3,6,7,/* 203*/
    0,1,4,5,   2,3,6,7,/* 204*/
    1,4,5,   0,2,3,6,7,/* 205*/
    0,4,5,   1,2,3,6,7,/* 206*/
    4,5,   0,1,2,3,6,7,/* 207*/
    0,1,2,3,5,   4,6,7,/* 208*/
    1,2,3,5,   0,4,6,7,/* 209*/
    0,2,3,5,   1,4,6,7,/* 210*/
    2,3,5,   0,1,4,6,7,/* 211*/
    0,1,3,5,   2,4,6,7,/* 212*/
    1,3,5,   0,2,4,6,7,/* 213*/
    0,3,5,   1,2,4,6,7,/* 214*/
    3,5,   0,1,2,4,6,7,/* 215*/
    0,1,2,5,   3,4,6,7,/* 216*/
    1,2,5,   0,3,4,6,7,/* 217*/
    0,2,5,   1,3,4,6,7,/* 218*/
    2,5,   0,1,3,4,6,7,/* 219*/
    0,1,5,   2,3,4,6,7,/* 220*/
    1,5,   0,2,3,4,6,7,/* 221*/
    0,5,   1,2,3,4,6,7,/* 222*/
    5,   0,1,2,3,4,6,7,/* 223*/
    0,1,2,3,4,   5,6,7,/* 224*/
    1,2,3,4,   0,5,6,7,/* 225*/
    0,2,3,4,   1,5,6,7,/* 226*/
    2,3,4,   0,1,5,6,7,/* 227*/
    0,1,3,4,   2,5,6,7,/* 228*/
    1,3,4,   0,2,5,6,7,/* 229*/
    0,3,4,   1,2,5,6,7,/* 230*/
    3,4,   0,1,2,5,6,7,/* 231*/
    0,1,2,4,   3,5,6,7,/* 232*/
    1,2,4,   0,3,5,6,7,/* 233*/
    0,2,4,   1,3,5,6,7,/* 234*/
    2,4,   0,1,3,5,6,7,/* 235*/
    0,1,4,   2,3,5,6,7,/* 236*/
    1,4,   0,2,3,5,6,7,/* 237*/
    0,4,   1,2,3,5,6,7,/* 238*/
    4,   0,1,2,3,5,6,7,/* 239*/
    0,1,2,3,   4,5,6,7,/* 240*/
    1,2,3,   0,4,5,6,7,/* 241*/
    0,2,3,   1,4,5,6,7,/* 242*/
    2,3,   0,1,4,5,6,7,/* 243*/
    0,1,3,   2,4,5,6,7,/* 244*/
    1,3,   0,2,4,5,6,7,/* 245*/
    0,3,   1,2,4,5,6,7,/* 246*/
    3,   0,1,2,4,5,6,7,/* 247*/
    0,1,2,   3,4,5,6,7,/* 248*/
    1,2,   0,3,4,5,6,7,/* 249*/
    0,2,   1,3,4,5,6,7,/* 250*/
    2,   0,1,3,4,5,6,7,/* 251*/
    0,1,   2,3,4,5,6,7,/* 252*/
    1,   0,2,3,4,5,6,7,/* 253*/
    0,   1,2,3,4,5,6,7,/* 254*/
    0,1,2,3,4,5,6,7,/* 255*/
};

uint8_t shufflemask128 [256]  __attribute__((aligned(0x100)))  = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, /* 0*/
    0, 1, 2, 3,      4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, /* 1*/
    4, 5, 6, 7,      0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15, /* 2*/
    0, 1, 2, 3, 4, 5, 6, 7,      8, 9, 10, 11, 12, 13, 14, 15, /* 3*/
    8, 9, 10, 11,      0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, /* 4*/
    0, 1, 2, 3, 8, 9, 10, 11,      4, 5, 6, 7, 12, 13, 14, 15, /* 5*/
    4, 5, 6, 7, 8, 9, 10, 11,      0, 1, 2, 3, 12, 13, 14, 15, /* 6*/
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,      12, 13, 14, 15, /* 7*/
    12, 13, 14, 15,      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* 8*/
    0, 1, 2, 3, 12, 13, 14, 15,      4, 5, 6, 7, 8, 9, 10, 11, /* 9*/
    4, 5, 6, 7, 12, 13, 14, 15,      0, 1, 2, 3, 8, 9, 10, 11, /* 10*/
    0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15,      8, 9, 10, 11, /* 11*/
    8, 9, 10, 11, 12, 13, 14, 15,      0, 1, 2, 3, 4, 5, 6, 7, /* 12*/
    0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15,      4, 5, 6, 7, /* 13*/
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,      0, 1, 2, 3, /* 14*/
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,      /* 15*/
};

int counts[256] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};

uint32_t simd_inplace_onepass_shuffle(uint32_t * array, size_t length) {
    /* we run through the data. Anything in [0,boundary) is black,
    * anything in [boundary, i) is white
    * stuff in [i,...) is grey
    * the function returns the location of the boundary.
    *
    * if length is large enough, at some point we will have a large chunk
    * of black, and a large chunk of white, so we can proceed in vectorized
    * manner, eating random bytes instead of random bits.
    */
    uint32_t boundary = 0;
    uint32_t i;
    uint64_t  randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbudget = 8;

    uint64_t  randbitbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbitbudget = 64;

    for(i = 0; i < length; ) {
        if((boundary + 8 > i) || (i + 8 >= length)) {// will be predicted false
            /* can't vectorize, not enough space, do it the slow way */
            int coin = randbitbuf & 1;//getRandomBit();
            if(randbitbudget == 1) {
                randbitbuf = fastrand() | ((uint64_t) fastrand()<<32);// 64-bit random value
                randbitbudget = 64;
            } else {
                randbitbudget--;
                randbitbuf >>=1;
            }
            if(coin) {
                swap(array, i,boundary);
                boundary++;
            }
            i++;

        } else {// common path follows
            /* we proceed 8 inputs at a time, but this can be generalized
            * it would be ideal to go 32 or 64 ints at a time. The main difficulty
            * is the shuffling.
            */
            uint8_t randbyte = randbuf & 0xFF;//getRandomByte();
            if(randbudget == 1) {
                randbudget = 8;
                randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
            } else {
                randbudget --;
                randbuf >>=8;
            }
            IACA_START;
            __m256i shufm = _mm256_load_si256((__m256i *)(shufflemask + 8 * randbyte));
            uint32_t cnt = _mm_popcnt_u32(randbyte); // might be faster with table look-up?
            __m256i allgrey = _mm256_lddqu_si256((__m256i *)(array + i));// this is all grey
            __m256i allwhite = _mm256_lddqu_si256((__m256i *)(array + boundary));// this is all white
            // we shuffle allgrey so that the first part is black and the second part is white
            __m256i blackthenwhite = _mm256_permutevar8x32_epi32(allgrey,shufm);
            _mm256_storeu_si256 ((__m256i *)(array + boundary), blackthenwhite);
            _mm256_storeu_si256 ((__m256i *)(array + i), allwhite);
            boundary += cnt; // might be faster with table look-up?
            i += 8;
            IACA_END;
        }
    }
    return boundary ;
}


uint32_t simd_inplace_onepass_shuffle2(uint32_t * array, size_t length) {
    /* we run through the data. Anything in [0,boundary) is black,
    * anything in [boundary, i) is white
    * stuff in [i,...) is grey
    * the function returns the location of the boundary.
    *
    * if length is large enough, at some point we will have a large chunk
    * of black, and a large chunk of white, so we can proceed in vectorized
    * manner, eating random bytes instead of random bits.
    */
    uint32_t boundary = 0;
    uint32_t i;
    uint64_t  randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbudget = 8;

    uint64_t  randbitbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbitbudget = 64;

    for(i = 0; i < length; ) {
        if((boundary + 16 > i) || (i + 16 >= length)) {// will be predicted false
            /* can't vectorize, not enough space, do it the slow way */
            int coin = randbitbuf & 1;//getRandomBit();
            if(randbitbudget == 1) {
                randbitbuf = fastrand() | ((uint64_t) fastrand()<<32);// 64-bit random value
                randbitbudget = 64;
            } else {
                randbitbudget--;
                randbitbuf >>=1;
            }
            if(coin) {
                swap(array, i,boundary);
                boundary++;
            }
            i++;

        } else {// common path follows
            /* we proceed 8 inputs at a time, but this can be generalized
            * it would be ideal to go 32 or 64 ints at a time. The main difficulty
            * is the shuffling.
            */
            if(randbudget <= 1) {
                randbudget = 8;
                randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
            }
            uint8_t randbyte = randbuf & 0xFF;
            uint8_t randbyte2 = (randbuf >> 8) & 0xFF;

            randbudget -=2;
            randbuf >>=16;

            __m256i shufm = _mm256_load_si256((__m256i *)(shufflemask + 8 * randbyte));
            uint32_t cnt = _mm_popcnt_u32(randbyte);
            __m256i shufm2 = _mm256_load_si256((__m256i *)(shufflemask + 8 * randbyte2));
            uint32_t cnt2 = _mm_popcnt_u32(randbyte2);

            __m256i allgrey = _mm256_lddqu_si256((__m256i *)(array + i));// this is all grey
            __m256i allgrey2 = _mm256_lddqu_si256((__m256i *)(array + i +8));// this is all grey

            __m256i allwhite = _mm256_lddqu_si256((__m256i *)(array + boundary));// this is all white
            __m256i blackthenwhite = _mm256_permutevar8x32_epi32(allgrey,shufm);
            __m256i blackthenwhite2 = _mm256_permutevar8x32_epi32(allgrey2,shufm2);

            _mm256_storeu_si256 ((__m256i *)(array + boundary), blackthenwhite);

            __m256i allwhite2 = _mm256_lddqu_si256((__m256i *)(array + boundary + cnt));// this is all white
            _mm256_storeu_si256 ((__m256i *)(array + boundary + cnt ), blackthenwhite2);

            _mm256_storeu_si256 ((__m256i *)(array + i), allwhite);
            _mm256_storeu_si256 ((__m256i *)(array + i + 8), allwhite2);
            boundary += cnt + cnt2;
            i += 16;

        }
    }
    return boundary ;
}

// follows roughly what you can find in Fast Quicksort Implementation Using AVX Instructions
void simd_gueron_onepass_shuffle(uint32_t * array,  size_t length, uint32_t* out1, uint32_t *out0,size_t * len1, size_t * len0) {
    uint32_t * out0begin = out0;
    uint32_t * out1begin = out1;

    uint32_t i;
    uint64_t  randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbudget = 8;

    uint64_t  randbitbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbitbudget = 64;
    for(i = 0; i + 7 < length; ) {
        uint8_t randbyte = randbuf & 0xFF;//getRandomByte();
        if(randbudget == 1) {
            randbudget = 8;
            randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
        } else {
            randbudget --;
            randbuf >>=8;
        }
        __m256i shufm = _mm256_load_si256((__m256i *)(shufflemask + 8 * randbyte));
        __m256i revshufm = _mm256_load_si256((__m256i *)(reverseshufflemask + 8 * randbyte));

        uint32_t cnt1 = _mm_popcnt_u32(randbyte); // might be faster with table look-up?
        uint32_t cnt0 = 8 - cnt1;
        __m256i allgrey = _mm256_lddqu_si256((__m256i *)(array + i));// this is all grey
        __m256i blackthenwhite = _mm256_permutevar8x32_epi32(allgrey,shufm);
        __m256i whitethenblack = _mm256_permutevar8x32_epi32(allgrey,revshufm);

        _mm256_storeu_si256 ((__m256i *)(out1), blackthenwhite);
        _mm256_storeu_si256 ((__m256i *)(out0), whitethenblack);
        out1 += cnt1;
        out0 += cnt0;
        i += 8;
    }
    for(; i  < length; i++) {

        /* can't vectorize, not enough space, do it the slow way */
        int coin = randbitbuf & 1;//getRandomBit();
        if(randbitbudget == 1) {
            randbitbuf = fastrand() | ((uint64_t) fastrand()<<32);// 64-bit random value
            randbitbudget = 64;
        } else {
            randbitbudget--;
            randbitbuf >>=1;
        }
        if(coin) {
            out1[0] = array[0];
            out1++;
        } else {
            out0[0] = array[0];
            out0++;
        }
    }
    *len1 = out1 - out1begin;
    *len0 = out0 - out0begin;
}

// improving slightly over Fast Quicksort Implementation Using AVX Instructions
void simd_fasterthangueron_onepass_shuffle(uint32_t * array,  size_t length, uint32_t* out1, uint32_t *out0,size_t * len1, size_t * len0) {
    uint32_t * out0begin = out0;
    uint32_t * out1begin = out1;

    uint32_t i;
    uint64_t  randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbudget = 8;

    uint64_t  randbitbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbitbudget = 64;
    __m256i reverseme = _mm256_set_epi32(0,1,2,3,4,5,6,7);
    for(i = 0; i + 7 < length; ) {
        uint8_t randbyte = randbuf & 0xFF;//getRandomByte();
        if(randbudget == 1) {
            randbudget = 8;
            randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
        } else {
            randbudget --;
            randbuf >>=8;
        }
        __m256i shufm = _mm256_load_si256((__m256i *)(shufflemask + 8 * randbyte));

        uint32_t cnt1 = _mm_popcnt_u32(randbyte); // might be faster with table look-up?
        uint32_t cnt0 = 8 - cnt1;
        __m256i allgrey = _mm256_lddqu_si256((__m256i *)(array + i));// this is all grey
        __m256i blackthenwhite = _mm256_permutevar8x32_epi32(allgrey,shufm);
        __m256i whitethenblack = _mm256_permutevar8x32_epi32(blackthenwhite,reverseme);

        _mm256_storeu_si256 ((__m256i *)(out1), blackthenwhite);
        _mm256_storeu_si256 ((__m256i *)(out0), whitethenblack);
        out1 += cnt1;
        out0 += cnt0;
        i += 8;
    }
    for(; i  < length; i++) {

        /* can't vectorize, not enough space, do it the slow way */
        int coin = randbitbuf & 1;//getRandomBit();
        if(randbitbudget == 1) {
            randbitbuf = fastrand() | ((uint64_t) fastrand()<<32);// 64-bit random value
            randbitbudget = 64;
        } else {
            randbitbudget--;
            randbitbuf >>=1;
        }
        if(coin) {
            out1[0] = array[0];
            out1++;
        } else {
            out0[0] = array[0];
            out0++;
        }
    }
    *len1 = out1 - out1begin;
    *len0 = out0 - out0begin;
}

// uses 128-bit vectors
uint32_t simd128_inplace_onepass_shuffle(uint32_t * array, size_t length) {
    /* we run through the data. Anything in [0,boundary) is black,
    * anything in [boundary, i) is white
    * stuff in [i,...) is grey
    * the function returns the location of the boundary.
    *
    * if length is large enough, at some point we will have a large chunk
    * of black, and a large chunk of white, so we can proceed in vectorized
    * manner, eating random bytes instead of random bits.
    */
    uint32_t boundary = 0;
    uint32_t i;
    uint64_t  randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbudget = 16;

    uint64_t  randbitbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbitbudget = 64;

    for(i = 0; i < length; ) {
        if((boundary + 4 > i) || (i + 4 >= length)) {// will be predicted false
            /* can't vectorize, not enough space, do it the slow way */
            int coin = randbitbuf & 1;//getRandomBit();
            if(randbitbudget == 1) {
                randbitbuf = fastrand() | ((uint64_t) fastrand()<<32);// 64-bit random value
                randbitbudget = 64;
            } else {
                randbitbudget--;
                randbitbuf >>=1;
            }
            if(coin) {
                swap(array, i,boundary);
                boundary++;
            }
            i++;

        } else {// common path follows
            /* we proceed 8 inputs at a time, but this can be generalized
            * it would be ideal to go 32 or 64 ints at a time. The main difficulty
            * is the shuffling.
            */
            uint8_t randbyte = randbuf & 0xF;//getRandomByte();
            if(randbudget == 1) {
                randbudget = 16;
                randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
            } else {
                randbudget --;
                randbuf >>=4;
            }
            IACA_START;
            __m128i shufm = _mm_load_si128((__m128i *)(shufflemask128 + 16 * randbyte));
            uint32_t cnt = _mm_popcnt_u32(randbyte); // might be faster with table look-up?
            __m128i allgrey = _mm_lddqu_si128((__m128i *)(array + i));// this is all grey
            __m128i allwhite = _mm_lddqu_si128((__m128i *)(array + boundary));// this is all white
            // we shuffle allgrey so that the first part is black and the second part is white
            __m128i blackthenwhite = _mm_shuffle_epi8(allgrey,shufm);
            _mm_storeu_si128 ((__m128i *)(array + boundary), blackthenwhite);
            _mm_storeu_si128 ((__m128i *)(array + i), allwhite);
            boundary += cnt; // might be faster with table look-up?
            i += 4;
            IACA_END;
        }
    }
    return boundary ;
}




uint32_t simd_twobuffer_onepass_shuffle(uint32_t * array, size_t length, uint32_t * out) {
    uint32_t * arrayend = array + length;
    /**
    0's are written to top, proceeding toward bottom.

    1's are written to bottom, backward toward top.
    */
    uint32_t * top = out;
    uint32_t *  bottom = out + length - 1;
    uint64_t  randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbudget = 8;
    while(bottom - top >= 15 ) {
        uint8_t randbyte = randbuf & 0xFF;//getRandomByte();
        if(randbudget == 1) {
            randbudget = 8;
            randbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
        } else {
            randbudget --;
            randbuf >>=8;
        }
        IACA_START;
        __m256i shufm = _mm256_load_si256((__m256i *)(shufflemask + 8 * randbyte));
        uint32_t num1s = _mm_popcnt_u32(randbyte);
        uint32_t num0s = 8 - num1s;
        __m256i allgrey = _mm256_lddqu_si256((__m256i *)(array));// this is all grey
        array += 8;
        // we shuffle allgrey so that the first part is black and the second part is white
        __m256i blackthenwhite = _mm256_permutevar8x32_epi32(allgrey,shufm);
        _mm256_storeu_si256 ((__m256i *)(top), blackthenwhite);
        top += num1s;
        _mm256_storeu_si256 ((__m256i *)(bottom - 7), blackthenwhite);
        bottom -= num0s;
        IACA_END;
    }
    /**
    * We finish off the rest with a scalar algo.
    * It could be vectorized for greater speed on small arrays.
    */
    uint64_t  randbitbuf = fastrand() | ((uint64_t) fastrand() << 32);// 64-bit random value
    int randbitbudget = 64;
    for(; array < arrayend; ++array) {
        int coin = randbitbuf & 1;//getRandomBit();
        if(randbitbudget == 1) {
            randbitbuf = fastrand() | ((uint64_t) fastrand()<<32);// 64-bit random value
            randbitbudget = 64;
        } else {
            randbitbudget--;
            randbitbuf >>=1;
        }
        if(coin) {
            *top = *array;
            ++top;
        } else {
            *bottom = *array;
            --bottom;
        }
    }
    return bottom - out;
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
            uint32_t nextpos = fastFairRandomInt(&rb, i, m2-1,bused);//
            int tmp = storage[i - 1];// likely in cache
            int val = storage[nextpos]; // could be costly
            storage[i - 1] = val;
            storage[nextpos] = tmp; // you might have to read this store later
        }
        m2 = m2 >> 1;
        bused--;
    }
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

void recursive_shuffle(int * storage, size_t size, size_t threshold) {
    if(size < threshold)
        fast_shuffle(storage, size);
    else {
        uint32_t bound = simd_inplace_onepass_shuffle((uint32_t*)storage, size);
        recursive_shuffle(storage,bound,threshold);
        recursive_shuffle(storage+bound,size-bound,threshold);
    }
}

void heuristic_shuffle(int * storage, size_t size) {
    uint32_t bused = 32 - __builtin_clz(size);
    while(bused != 0) {
        simd_inplace_onepass_shuffle((uint32_t*)storage, size);
        bused--;
    }
}
uint32_t simd_simplified_prefetch(uint32_t * array, size_t length, uint32_t * out) {
    uint32_t * top = out;
    uint32_t *  bottom = out + length - 1;
    while(bottom - top >= 15 ) {
        _mm_prefetch(top + 32, 1);
        *top = 1;
        top += 4;
        _mm_prefetch(bottom - 32, 1);
        *bottom = 1;
        bottom -= 4;
    }
    return bottom - out;
}

uint32_t simd_simplified(uint32_t * array, size_t length, uint32_t * out) {
    uint32_t * top = out;
    uint32_t *  bottom = out + length - 1;
    while(bottom - top >= 15 ) {
        // _mm_prefetch(top + 32, 1);
        *top = 1;
        top += 4;
        // _mm_prefetch(bottom - 32, 1);
        *bottom = 1;
        bottom -= 4;
    }
    return bottom - out;
}
int compare( const void* a, const void* b)
{
    int int_a = * ( (int*) a );
    int int_b = * ( (int*) b );

    if ( int_a == int_b ) return 0;
    else if ( int_a < int_b ) return -1;
    else return 1;
}

int demo(size_t N) {
    int bogus = 0;
    size_t i;
    size_t len1, len2;
    float cycles_per_search1;
    int *array = (int *) malloc( N * sizeof(int) );
    int *tmparray = (int *) malloc( N * sizeof(int) );
    int *tmparray2 = (int *) malloc(N* sizeof(int) );
    for(i = 0; i < N; i++)
        array[i] = i;
    uint64_t cycles_start, cycles_final;
#ifdef USE_GENERIC
    printf("Using some basic random number generator\n");
#elif USE_MT
    printf("Using Mersenne Twister\n");
#elif USE_RAND
    printf("Using rand\n");
#elif USE_HARDWARE
    printf("Using hardware\n");
#else
    printf("Using Mersenne Twister\n");
#endif

    printf("populating array %zu \n",N);
    printf("log(N) = %d \n",(33 - __builtin_clz(N-1)));

    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }
    x=1;
    seedMT(x);
    printf("\n");

    // sanity testing
    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }
    qsort( tmparray, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(tmparray[i] != i) abort();
    }
    qsort( tmparray2, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(tmparray2[i] != i) abort();
    }


    RDTSC_START(cycles_start);
    bogus += inplace_onepass_shuffle((uint32_t*) array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);
    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }
    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("random split  cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    bogus += simd_inplace_onepass_shuffle((uint32_t*) array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("SIMD (256) random split  cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    bogus += simd_inplace_onepass_shuffle2((uint32_t*) array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("SIMD (256) random split 2-by-2 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }


    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    bogus += simd128_inplace_onepass_shuffle((uint32_t*) array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("SIMD (128) random split  cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    simd_gueron_onepass_shuffle((uint32_t*) array, N, (uint32_t*) tmparray, (uint32_t*) tmparray2, &len1, &len2);
    bogus += len1 + len2;
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("Gueron's SIMD (256) random split  cycles per key  %.2f \n", cycles_per_search1);


    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    simd_fasterthangueron_onepass_shuffle((uint32_t*) array, N, (uint32_t*) tmparray, (uint32_t*) tmparray2, &len1, &len2);
    bogus += len1 + len2;
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("Faster-Than-Gueron's SIMD (256) random split  cycles per key  %.2f \n", cycles_per_search1);

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    bogus += simd_simplified((uint32_t*) array, N ,(uint32_t*)  tmparray);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("SIMD two-buffer simplified (bogus) random split  cycles per key  %.2f \n", cycles_per_search1);



    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }
    RDTSC_START(cycles_start);
    bogus += simd_simplified_prefetch((uint32_t*) array, N ,(uint32_t*)  tmparray);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("SIMD two-buffer simplified with prefetch (bogus) random split  cycles per key  %.2f \n", cycles_per_search1);



    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    bogus += simd_twobuffer_onepass_shuffle((uint32_t*) array, N ,(uint32_t*)  tmparray);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("SIMD two-buffer random split  cycles per key  %.2f \n", cycles_per_search1);

    qsort( tmparray, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(tmparray[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    shuffle( array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("normal shuffle cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }
    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    fast_shuffle(array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("fast shuffle  cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    heuristic_shuffle(array, N );
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("heuristic shuffle  cycles per key  %.2f \n", cycles_per_search1);


    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    recursive_shuffle(array, N ,4096);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 4096 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    recursive_shuffle(array, N ,16384);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 16384 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }

    RDTSC_START(cycles_start);
    recursive_shuffle(array, N ,32768);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 32768 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    recursive_shuffle(array, N ,65536);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 65536 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }


    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }



    RDTSC_START(cycles_start);
    recursive_shuffle(array, N , 131072);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 131072 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    recursive_shuffle(array, N , 262144);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 262144 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    recursive_shuffle(array, N , 524288);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 524288 cycles per key  %.2f \n", cycles_per_search1);

    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }

    // reinitialize the tests so we start fresh
    for(i = 0; i < N; ++i) {
        array[i] = i;
        tmparray[i] = i;
        tmparray2[i] = i;
    }


    RDTSC_START(cycles_start);
    recursive_shuffle(array, N , 2097152);
    bogus += array[0];
    RDTSC_FINAL(cycles_final);

    cycles_per_search1 =
        ( cycles_final - cycles_start) / (float) (N);
    printf("recursive shuffle 2097152 cycles per key  %.2f \n", cycles_per_search1);


    qsort( array, N, sizeof(int), compare );
    for(i = 0; i < N; ++i) {
        if(array[i] != i) abort();
    }
    printf("Ok\n");

    free(array);
    free(tmparray);
    free(tmparray2);

    return bogus;

}


int main() {
    size_t N;
    for(N = 16384; N < 2147483648; N*=8) {
        demo(N);
        printf("\n\n");
    }
    return 0;
}
