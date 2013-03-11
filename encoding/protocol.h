#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include "coding.h"
#include <netinet/in.h> 

typedef struct muxstate_t {
    uint16_t sport;
    uint16_t dport;
    uint32_t remote_ip;
    
    uint32_t lastByteSent;
    uint32_t lastByteAcked;
    encoderstate* encoderState;
    decoderstate* decoderState;
} muxstate;

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, muxstate** statesTable, int* tableLength);
encodedpacket* bufferToEncodedPacket(char* buffer, int size);
clearpacket* bufferToClearPacket(char* buffer, int size, int ipHdrLen);
void encodedPacketToBuffer(encodedpacket p, char* buffer, int* size);
void clearPacketToBuffer(clearpacket p, char* buffer, int* size);

int ipHeaderLength(char* buffer);

int isData(char* buffer, int size);


/* On packets received via the TUN interface*/
int isTCP(char* buffer, int size);
void extractMuxInfosFromIP(char* buffer, int size, uint16_t* sport, uint16_t* dport, uint32_t* remote_ip);


/* On packets received via UDP*/
void extractMuxInfosFromMuxed(char* buffer, uint16_t* sport, uint16_t* dport, uint32_t* remote_ip);



#endif



