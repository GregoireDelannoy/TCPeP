#ifndef _ENCODING_
#define _ENCODING_
#include "utils.h"
#include "packet.h"
#include "matrix.h"

#define BASE_WINDOW 8.0 // Number of tokens to start with
#define BLKSIZE 3 // Block size (in number of packets)
#define PACKETSIZE 10 // Maximum payload size (in bytes)
#define SS_THRESHOLD 16 // Slow start threshold
#define SMOOTHING_FACTOR 0.05 // Smoothing factor
#define TIMEOUT_FACTOR 3 // Timeout = factor * rtt
#define INFLIGHT_FACTOR 1.5 // Gamma from the papers
#define COMPUTING_DELAY  10000 // Time taken by the coding operations, estimation in uSeconds. Becomes important if the link RTT is low (LAN for example)

typedef struct packetsentinfo_t{
    uint32_t seqNo;
    uint16_t blockNo;
    struct timeval sentAt;
} packetsentinfo;

typedef struct block_t{
    matrix* dataMatrix;
    
    int nPackets; // Number of packets allocated
    int isSentPacket[BLKSIZE]; // True if a packet has already been sent uncoded
} block;

typedef struct encoderstate_t {
    block* blocks;
    int numBlock; // Number of blocks allocated
    struct timeval nextTimeout;
    packetsentinfo** packetSentInfos;
    int* nPacketSent;
    float p; // Short-term average packet loss
    unsigned long int RTT ; // Floating Average RTT (microseconds)
    unsigned long int RTTmin;
    uint32_t seqNo_Next; // Sequence number of the next packet to be transmitted
    uint32_t seqNo_Una;  // Sequence number of the last unacknowledged packet
    struct timeval time_lastAck;
    float congestionWindow; // Maximum number of packets in flight
    uint16_t currBlock; // Current block (not yet acked) => Block 0 in the matrix table
    uint8_t currDof; // Degrees of freedom for current block
    int slowStartMode;
    
    uint8_t** dataToSend;  // Encoded data packets to send via UDP
    int* dataToSendSize;   // Size of the n-th packet
    int nDataToSend;       // Number of packets
} encoderstate;


void handleInClear(encoderstate* state, uint8_t* buffer, int size);

void onTimeOut(encoderstate* state);

void onAck(encoderstate* state, uint8_t* buffer, int size);

encoderstate* encoderStateInit();

void encoderStateFree(encoderstate* state);

void encoderStatePrint(encoderstate state);

#endif