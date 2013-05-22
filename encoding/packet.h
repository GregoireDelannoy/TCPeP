#ifndef _PACKET_
#define _PACKET_

#include "utils.h"

#define FLAG_CLEAR 0b00000000
#define FLAG_CODED 0b10000000
#define BITMASK_NO    0b01111111
#define BITMASK_FLAG  0b10000000

#define DOFS_LENGTH 3 // The number of blocks for which we send the number of dofs

typedef struct datapacket_t {
    uint16_t blockNo; // Block number of the packet
    uint8_t packetNumber; // Flag (1bit) | Packet index in block (if uncoded), number of packets used for coding (if coded)
    uint32_t seqNo; // Sequence number (always increment)
    uint8_t* payloadAndSize; // uint16 | real payload. Note : the uint16 gets encoded when the rest of the payload is.
    
    int size; // Size of the array payloadAndSize. NOT TRANSMISSIBLE !
} datapacket;

typedef struct ackpacket_t {
    uint16_t ack_currBlock; // Smallest undecoded block
    uint8_t* ack_dofs; // Degrees of freedom recovered for the blocks
    uint32_t ack_seqNo; // Sequence Number for the currently acknowledged packet
    uint16_t ack_loss;  // Number of lost packets in the seen set
    uint16_t ack_total; // Total number of packets in the seen set
} ackpacket;

void dataPacketPrint(datapacket p);
void dataPacketToBuffer(datapacket p, uint8_t* buffer, int* size);
datapacket* bufferToData(uint8_t* buffer, int size);

void ackPacketPrint(ackpacket p);
void ackPacketToBuffer(ackpacket p, uint8_t* buffer, int* size);
ackpacket* bufferToAck(uint8_t* buffer, int size);

#endif
