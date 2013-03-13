#include "protocol.h"


// Given a raw IP packet, decide wether or not it is a TCP packet
int isTCP(char* buffer, int size){
    if(size > 40){ // If packet size < 40 bytes, it cannot contain both IP and TCP headers.
        // Assuming that we got an IP packet.
        if((buffer[0] & 0xF0) == 0x40){
            if(buffer[9] == 0x06){ // Buffer[9] = protocol field ; 6 = TCP
                return true;
            }
        } else {
            printf("Got an IPv6 packet... HELP !\n");
        }
    }
    
    return false;
}

// Given a TCP packet, decide wether or not there's data in
int isData(char* buffer, int size){
    uint8_t tcpHdrLength;
    memcpy(&tcpHdrLength, buffer + 12, 1); // Copy the Data Offset & reserved fields
    
    tcpHdrLength = 4 * ((tcpHdrLength >> 4) & 0x0F);
    do_debug("isData: Data Offset = %u and size = %d\n", tcpHdrLength, size);
    if(size > 4 * tcpHdrLength){
        return true;
    } else {
        return false;
    }
}

// Return the length of the IP header, in bytes.
int ipHeaderLength(char* buffer){
    return 4*(buffer[0] & 0x0F);
}

// Get ports and dst address from a TCP packet
void extractMuxInfosFromIP(char* buffer, int size, uint16_t* sport, uint16_t* dport, uint32_t* source_ip, uint32_t* destination_ip){
    int ipHdrLen = ipHeaderLength(buffer);
    
    memcpy(source_ip, buffer + 16 , 4);
    *source_ip = ntohl(*source_ip);
    memcpy(destination_ip, buffer + 12 , 4);
    *destination_ip = ntohl(*destination_ip);
    
    memcpy(sport, buffer + ipHdrLen, 2);
    *sport = ntohs(*sport);
    memcpy(dport, buffer + ipHdrLen + 2, 2);
    *dport = ntohs(*dport);
}

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, muxstate** statesTable, int* tableLength){
    // If the mux is already known, return its index, otherwise create it
    int i;
    
    for(i=0; i<(*tableLength); i++){
        if(((*statesTable)[i].sport == sport) && ((*statesTable)[i].dport == dport) && ((*statesTable)[i].remote_ip == remote_ip)){
            do_debug("Corresponding mux found : %d\n", i);
            return i;
        }
    }
    
    do_debug("No existing mux ; create one\n");
    (*tableLength)++;
    *statesTable = realloc(*statesTable, (*tableLength) * sizeof(muxstate));
    (*statesTable)[(*tableLength) - 1].sport = sport;
    (*statesTable)[(*tableLength) - 1].dport = dport;
    (*statesTable)[(*tableLength) - 1].remote_ip = remote_ip;
    (*statesTable)[(*tableLength) - 1].lastByteSent = 0;
    (*statesTable)[(*tableLength) - 1].encoderState = encoderStateInit();
    (*statesTable)[(*tableLength) - 1].decoderState = decoderStateInit();
    
    return (*tableLength) - 1;
}

void extractMuxInfosFromMuxed(char* buffer, uint16_t* sport, uint16_t* dport, uint32_t* remote_ip){
    memcpy(remote_ip, buffer, 4);
    *remote_ip = ntohl(*remote_ip);
    memcpy(sport, buffer + 4, 2);
    *sport = ntohs(*sport);
    memcpy(dport, buffer + 6, 2);
    *dport = ntohs(*dport);
}

encodedpacket* bufferToEncodedPacket(char* buffer, int size){
    encodedpacket* ret = malloc(sizeof(encodedpacket));
    int i;
    
    ret->coeffs = malloc(sizeof(coeffs));
    memcpy(&(ret->coeffs->start1), buffer,4);
    ret->coeffs->start1 = ntohl(ret->coeffs->start1);
    memcpy(&(ret->coeffs->n), buffer + 4, 1);
    
    ret->coeffs->alpha = malloc(ret->coeffs->n * sizeof(uint8_t));
    ret->coeffs->start = malloc(ret->coeffs->n * sizeof(uint16_t));
    ret->coeffs->size = malloc(ret->coeffs->n * sizeof(uint16_t));
    
    for(i=0; i<ret->coeffs->n; i++){
        memcpy(&(ret->coeffs->alpha[i]), buffer + 5 + 5*i, 1);
        memcpy(&(ret->coeffs->start[i]), buffer + 6 + 5*i, 2);
        ret->coeffs->start[i] = ntohs(ret->coeffs->start[i]);
        memcpy(&(ret->coeffs->size[i]), buffer + 8 + 5*i, 2);
        ret->coeffs->size[i] = ntohs(ret->coeffs->size[i]);
    }
    
    ret->payload = payloadCreate(size - (5 + 5 * ret->coeffs->n), (uint8_t*)(buffer + (5 + 5 * ret->coeffs->n)));
    
    return ret;
}

void encodedPacketToBuffer(encodedpacket p, char* buffer, int* size){
    int i;
    uint32_t tmp32;
    uint16_t tmp16; // Needed to perform the byte-order reversal
    
    *size = (p.payload->size + 5 + p.coeffs->n * 5);
    tmp32 = htonl(p.coeffs->start1);
    memcpy(buffer, &tmp32, 4);
    memcpy(buffer + 4, &(p.coeffs->n), 1);
    for(i=0; i<p.coeffs->n; i++){
        memcpy(buffer + 5 + 5*i, &(p.coeffs->alpha[i]), 1);
        tmp16 = htons(p.coeffs->start[i]);
        memcpy(buffer + 6 + 5*i, &tmp16, 2);
        tmp16 = htons(p.coeffs->size[i]);
        memcpy(buffer + 8 + 5*i, &tmp16, 2);
    }
    
    memcpy(buffer + 5 + p.coeffs->n * 5, (char*)(p.payload->data), p.payload->size);
}

clearpacket* bufferToClearPacket(char* buffer, int size, int ipHdrLen){
    clearpacket* ret = malloc(sizeof(clearpacket));
    uint32_t tmp;
    memcpy(&tmp, buffer + 4 + ipHdrLen, 4);
    ret->indexStart = ntohl(tmp);
    do_debug("bTCP: indexStart = %u\n", ret->indexStart);
    
    ret->type = TYPE_DATA;
    ret->payload = payloadCreate(size, (uint8_t*)buffer);
    
    return ret;
}



void clearPacketToBuffer(clearpacket p, char* buffer, int* size){
    *size = p.payload->size;
    memcpy(buffer, p.payload->data, *size);
}


uint32_t getAckNumber(char* buffer){
    uint32_t ret;
    if((*(buffer + 13) & 0x10)){ // ack bit set
        memcpy(&ret, buffer + 8, 4);
        return ntohl(ret);
    }
    return 0;
}

uint32_t getSeqNumber(char* buffer){
    uint32_t ret;
    memcpy(&ret, buffer + 4, 4);
    return ntohl(ret);
}
