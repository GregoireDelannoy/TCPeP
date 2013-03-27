#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include "packet.h"
#include "decoding.h"
#include "encoding.h"

#define HELO_MSG (uint8_t*)"HELO"
#define TYPE_DATA 0x00
#define TYPE_OPEN 0x01
#define TYPE_CLOSE 0x02
#define TYPE_ACK 0x03


typedef struct muxstate_t {
    int sock_fd;
    
    uint16_t sport;
    uint16_t dport;
    uint32_t remote_ip;
    
    encoderstate* encoderState;
    decoderstate* decoderState;
} muxstate;

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, int sock_fd, muxstate** statesTable, int* tableLength);
void removeMux(int i, muxstate** statesTable, int* tableLength);

void bufferToMuxed(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate mux, uint8_t type);

void muxedToBuffer(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate* mux, uint8_t* type);
#endif



