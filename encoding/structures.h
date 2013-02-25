#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"

typedef struct payload_t {
    int size;
    uint8_t* data;
} payload;

typedef struct encodedpacket_t {
    int nCoeffs;
    uint8_t* coeffs;
    payload payload;
} encodedpacket;

typedef struct clearpacket_t {
    payload payload;
} clearpacket;

typedef struct clearpacketarray_t {
    int nPackets;
    clearpacket* packets;
} clearpacketarray;

typedef struct encodedpacketpool_t {
    int nPackets;
    encodedpacket* packets;
    matrix* coeffs;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} encodedpacketpool;

#endif
