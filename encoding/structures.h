#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"

typedef struct payload_t {
    int indexStart;
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

typedef struct packetarray_t {
    int nPackets;
    void** packets; // Note, the array can contains either encoded or clear packets. Hence the void type
} packetarray;

typedef struct encodedpacketpool_t {
    int nPackets;
    encodedpacket* packets;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} encodedpacketpool;

#endif
