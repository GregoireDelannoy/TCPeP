#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"
#include "packet.h"

typedef struct decoderbuffer_t {
    encodedpacketarray* array;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} decoderbuffer;

#endif
