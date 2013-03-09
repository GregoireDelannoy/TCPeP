#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include <netinet/in.h> 

typedef struct muxinfos_t {
    uint16_t sport;
    uint16_t dport;
    uint32_t remote_ip;
} muxinfos;

int isTCP(char* buffer, int size);

muxinfos extractMuxInfos(char* buffer, int size);

#endif



