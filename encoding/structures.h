#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"

typedef struct payload_t {
    int size;
    uint8_t* data;
} payload;

//typedef struct coeffs_t {
    //int nCoeffs;
    //uint32_t start1; // Start sequence number
    //uint8_t* alpha;  // Coefficient for the n-th segment
    //uint16_t* start; // Starting sequence number for the 1+ packets ; relative to previous packet
    //uint16_t* size;  // Real size of the n-th segment (can be padded with zeros )
//} coeffs;

typedef struct encodedpacket_t {
    int nCoeffs;
    //coeffs coeffs;
    uint8_t* coeffs;
    payload payload;
} encodedpacket;

typedef struct clearpacket_t {
    int indexStart;
    payload payload;
} clearpacket;

typedef struct packetarray_t {
    int nPackets;
    void** packets; // Note : the array can contains either encoded or clear packets. Hence the void type
} packetarray;

typedef struct encodedpacketpool_t {
    int nPackets;
    encodedpacket* packets;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} encodedpacketpool;

#endif
