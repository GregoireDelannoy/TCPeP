#ifndef _STRUCTURES_
#define _STRUCTURES_
#include <stdint.h>
#include "matrix.h"
#include "packet.h"

#define min(a,b) a<b?a:b
#define max(a,b) a>b?a:b

#define true 1==1
#define false 1==0

typedef struct decoderpool_t {
    encodedpacketarray* array;
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
} decoderpool;

#endif
