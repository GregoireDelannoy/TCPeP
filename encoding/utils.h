#ifndef _UTILS_
#define _UTILS_
#include <stdint.h>
#include "matrix.h"
#include "packet.h"

#define min(a,b) a<b?a:b
#define max(a,b) a>b?a:b

#define true 1==1
#define false 1==0

typedef struct clearinfos_t {
    uint32_t start;
    uint16_t size;
} clearinfos;


// Temporary ? structure
typedef struct decoderstate_t {
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
    matrix* codedData;
    clearinfos** clearInfosTable;
    int nClearPackets[1];
} decoderstate;

#endif


