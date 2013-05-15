#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include "packet.h"
#include "decoding.h"
#include "encoding.h"

#define TYPE_DATA 0x00
#define TYPE_OPEN 0x01
#define TYPE_OPENACK 0x02
#define TYPE_CLOSE 0x03
#define TYPE_ACK 0x04

#define STATE_CLOSEAWAITING 0x01
#define STATE_OPENED 0x02
#define STATE_OPENACKAWAITING 0x03
#define STATE_INIT 0x04


typedef struct muxstate_t {
    int sock_fd;
    
    uint16_t sport;
    uint16_t dport;
    uint32_t remote_ip;
    
    encoderstate* encoderState;
    decoderstate* decoderState;

    int state;
    
    struct sockaddr_in udpRemote;
} muxstate;

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, int sock_fd, muxstate** statesTable, int* tableLength, struct sockaddr_in udpRemoteAddr);
void removeMux(int i, muxstate** statesTable, int* tableLength);

void bufferToMuxed(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate mux, uint8_t type);

int muxedToBuffer(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate* mux, uint8_t* type);

void printMux(muxstate mux);
#endif



