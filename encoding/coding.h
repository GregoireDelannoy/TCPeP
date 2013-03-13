#ifndef _CODING_
#define _CODING_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // Needed for memcpy... don't ask why it's here !

#include "utils.h"
#include "galois_field.h"
#include "matrix.h"
#include "packet.h"

#define CODING_WINDOW 10
#define REDUNDANCY_FACTOR 1

typedef struct clearinfos_t {
    uint32_t start;
    uint16_t size;
} clearinfos;


typedef struct decoderstate_t {
    matrix* rrefCoeffs;
    matrix* invertedCoeffs;
    matrix* codedData;
    clearinfos** clearInfosTable;
    int nClearPackets[1];
    encodedpacketarray* buffer;
} decoderstate;

typedef struct encoderstate_t {
    float NUM;
    clearpacketarray* buffer;
    uint32_t lastByteAcked;
    uint32_t lastByteSent;
} encoderstate;

clearpacketarray* handleInCoded(encodedpacket codedPacket, decoderstate state);

encodedpacketarray* handleInClear(clearpacket clearPacket, encoderstate state);

decoderstate* decoderStateInit();

void decoderStateFree(decoderstate* state);

encoderstate* encoderStateInit();

void encoderStateFree(encoderstate* state);

#endif
