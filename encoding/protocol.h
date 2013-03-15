#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include "coding.h"
#include <netinet/in.h> 

#define HELO_MSG "HELO"

typedef struct muxstate_t {
    uint16_t sport;
    uint16_t dport;
    uint32_t remote_ip;
    
    encoderstate* encoderState;
    decoderstate* decoderState;
} muxstate;

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, muxstate** statesTable, int* tableLength);
encodedpacket* bufferToEncodedPacket(char* buffer, int size);
clearpacket* bufferToClearPacket(char* buffer, int size, int ipHdrLen);
void encodedPacketToBuffer(encodedpacket p, char* buffer, int* size);
void clearPacketToBuffer(clearpacket p, char* buffer, int* size);

int ipHeaderLength(char* buffer);

int tcpDataLength(char* buffer, int size);
uint32_t getAckNumber(char* buffer);
uint32_t getSeqNumber(char* buffer);



/* On packets received via the TUN interface*/
int isTCP(char* buffer, int size);
void extractMuxInfosFromIP(char* buffer, int size, uint16_t* sport, uint16_t* dport, uint32_t* source_ip, uint32_t* destination_ip);


/* On packets received via UDP*/
void extractMuxInfosFromMuxed(char* buffer, uint16_t* sport, uint16_t* dport, uint32_t* remote_ip);



#endif



