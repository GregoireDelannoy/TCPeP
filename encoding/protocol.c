#include "protocol.h"


int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, int sock_fd, muxstate** statesTable, int* tableLength){
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
    (*statesTable)[(*tableLength) - 1].sock_fd = sock_fd;
    (*statesTable)[(*tableLength) - 1].encoderState = encoderStateInit();
    (*statesTable)[(*tableLength) - 1].decoderState = decoderStateInit();
    
    return (*tableLength) - 1;
}

void removeMux(int index, muxstate** statesTable, int* tableLength){
    if(index >= (*tableLength)){
        my_err("in removeMux : index>= size\n");
        exit(1);
    } else {
        int i;
        encoderStateFree((*statesTable)[index].encoderState);
        decoderStateFree((*statesTable)[index].decoderState);
        for(i = index; i < ((*tableLength) - 1); i++){
            (*statesTable)[i] = (*statesTable)[i + 1];
        }
        (*tableLength)--;
        *statesTable = realloc(*statesTable, (*tableLength) * sizeof(muxstate));
    }
}


void bufferToMuxed(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate mux, uint8_t type){
    uint16_t tmp16;
    uint32_t tmp32;
    uint8_t tmp8;
    
    tmp16 = htons(mux.sport);
    memcpy(dst, &tmp16, 2);
    tmp16 = htons(mux.dport);
    memcpy(dst + 2, &tmp16, 2);
    tmp32 = htonl(mux.remote_ip);
    memcpy(dst + 4, &tmp32, 4);
    tmp8 = type;
    memcpy(dst + 8, &tmp8, 1);
    
    memcpy(dst + 9, src, srcLen);
    
    (*dstLen) = srcLen + 9;
}

void muxedToBuffer(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate* mux, uint8_t* type){
    uint16_t tmp16;
    uint32_t tmp32;
    uint8_t tmp8;

    memcpy(&tmp16, src, 2);
    mux->sport = ntohs(tmp16);
    memcpy(&tmp16, src + 2, 2);
    mux->dport = ntohs(tmp16);
    memcpy(&tmp32, src + 4, 4);
    mux->remote_ip = ntohl(tmp32);
    memcpy(&tmp8, src + 8, 1);
    (*type) = tmp8;
    
    memcpy(dst, src + 9, srcLen - 9);
    
    (*dstLen) = srcLen - 9;
}
