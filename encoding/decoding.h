#ifndef _DECODING_
#define _DECODING_
#include "utils.h"
#include "packet.h"
#include "matrix.h"

#define LOSS_BUFFER_SIZE 1000

typedef struct lossInformationBuffer_t{
    int isReceived[LOSS_BUFFER_SIZE];
    int currentIndex;
} lossInformationBuffer;


typedef struct decoderstate_t {
    matrix** blocks;
    matrix** coefficients;
    
    lossInformationBuffer* lossBuffer; // Store information about received packets, to estimate loss at the receiver side
    
    uint16_t currBlock;
    int numBlock; // Currently allocated blocks
    int* nPacketsInBlock; // Number of packet in each known block
    int** isSentPacketInBlock; // True if a packet has already been sent to the application
    
    uint8_t* dataToSend; // Decoded data, to be send to the application via the TCP socket
    int nDataToSend; // Number of bytes in buffer
    
    uint8_t** ackToSend; // Acks to send, via UDP
    int* ackToSendSize;  // Size of the n-th ack
    int nAckToSend;      // Number of acks
    
    uint32_t lastSeqReceived; // Highest sequence number seen
    
    long unsigned int stats_nAppendedNotInnovativeGaloisFirstBlock;
    long unsigned int stats_nAppendedNotInnovativeGaloisOtherBlock;
    long unsigned int stats_nOutdated;
    long unsigned int stats_nAppendedNotInnovativeCounter;
    long unsigned int stats_nInnovative;
} decoderstate;

void handleInCoded(decoderstate* state, uint8_t* buffer, int size);

decoderstate* decoderStateInit();

void decoderStateFree(decoderstate* state);

void decoderStatePrint(decoderstate state);

#endif
