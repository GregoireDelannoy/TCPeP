#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"
#include "packet.h"

typedef struct encodedpacketpool_t {
    int nPackets;
    encodedpacket* packets;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} encodedpacketpool;

#endif
