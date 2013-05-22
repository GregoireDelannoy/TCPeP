#ifndef _ENCODING_
#define _ENCODING_
#include "utils.h"
#include "packet.h"
#include "matrix.h"

#define BASE_WINDOW 5.0 // Number of tokens to start with
#define SS_THRESHOLD 15.0 // Slow start threshold
#define MAX_WINDOW 5000 // Should not be needed...
#define SMOOTHING_FACTOR_LONG 0.0001 // Smoothing factor for the long term average
#define SMOOTHING_FACTOR_SHORT 0.1 // Smoothing factor for the short term average
#define TIMEOUT_FACTOR 50 // Timeout = factor * rtt
#define INFLIGHT_FACTOR 1.3 // Gamma from the papers
#define COMPUTING_DELAY  1000 // Time taken by the coding operations, estimation in uSeconds. Becomes important if the link RTT is very low (LAN or VMs for example)
#define ALPHA 0.05
#define BETA 0.2  // Alpha and Beta are Thresholds for the congestion-control algorithm
#define INCREMENT 5.0 // The increment factor for modifying the CWN

typedef struct packetsentinfo_t{
    uint32_t seqNo;
    uint16_t blockNo;
    struct timeval sentAt;
} packetsentinfo;

typedef struct block_t{
    matrix* dataMatrix;
    
    int nPackets; // Number of packets allocated
    int isSentPacket[BLKSIZE]; // True if a packet has already been sent uncoded
    
    uint8_t dofs; // Already received degrees of freedom for the block
} block;

typedef struct encoderstate_t {
    block* blocks;
    int numBlock; // Number of blocks allocated
    struct timeval nextTimeout;
    packetsentinfo** packetSentInfos;
    int* nPacketSent;
    float p; // Average packet loss from last ack
    double shortTermRttAverage ; // Floating Average RTT (microseconds), short term
    double longTermRttAverage ; // Floating Average RTT (microseconds), long term
    uint32_t seqNo_Next; // Sequence number of the next packet to be transmitted
    uint32_t seqNo_Una;  // Sequence number of the last unacknowledged packet
    struct timeval time_lastAck;
    float congestionWindow; // Maximum number of packets in flight
    uint16_t currBlock; // Current block (not yet acked) => Block 0 in the matrix table
    int slowStartMode;
    int timeOutCounter;
    
    int isOutstandingData; // True if there is still data from the TCP socket that has not been transfered yet
    
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
