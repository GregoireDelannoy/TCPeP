#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"
#include "packet.h"

typedef struct decoderpool_t {
    encodedpacketarray* array;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} decoderpool;

#endif
