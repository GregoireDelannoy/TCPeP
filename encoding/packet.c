#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "packet.h"

void payloadPrint(payload p){
    int i;
    
    if(p.size < MAX_PAYLOAD_PRINT){
        printf("Payload : ");
        for(i = 0; i < p.size ; i++){
            printf("%2x|", p.data[i]);
        } 
        printf("\n");
    } else {
        printf("Payload too large to be printed.\n");
    }
}

void encodedPacketPrint(encodedpacket packet){
    int i;
    // Print coefficients
    printf("Coeffs : ");
    printf("Base start: %u\n", packet.coeffs->start1);
    uint32_t currentStart = packet.coeffs->start1;
    
    for(i = 0; i < packet.coeffs->n ; i++){
        currentStart += packet.coeffs->start[i];
        printf("start%d: %u | ",i, packet.coeffs->start[i]);
        printf("realStart%d: %u | ",i, currentStart);
        printf("Size%d : %u | ",i, packet.coeffs->size[i]);
        printf("Headers Size%d : %u | ",i, packet.coeffs->hdrSize[i]);
        printf("alpha%d : %2x | ",i, packet.coeffs->alpha[i]);
        printf("\n");
    }
    
    payloadPrint(*(packet.payload));
}


void encodedPacketFree(encodedpacket* p){
    if(p->coeffs != 0){
        // Clean coeffs
        free(p->coeffs->alpha);
        free(p->coeffs->start);
        free(p->coeffs->size);
        free(p->coeffs->hdrSize);
        free(p->coeffs);
    }
    
    payloadFree(p->payload);
    free(p);
}

void clearPacketPrint(clearpacket packet){
    printf("Start : %u | Size : %u | hdr Size : %u\n", packet.indexStart, packet.payload->size, packet.hdrSize);
    payloadPrint(*(packet.payload));
}

clearpacket* clearPacketCreate(uint32_t index, uint16_t size, uint8_t hdrSize, uint8_t* data){
    clearpacket* ret = malloc(sizeof(clearpacket));
    payload* p = payloadCreate(size, data);
    
    ret->indexStart = index;
    ret->payload = p;
    
    ret->hdrSize = hdrSize;
    
    return ret;
}

void clearPacketFree(clearpacket* p){
    payloadFree(p->payload);
    free(p);
}

payload* payloadCreate(int size, uint8_t* data){ // Create a payload, copying the data
    payload* ret = malloc(sizeof(payload));
    ret->data = malloc(size * sizeof(uint8_t));
    
    ret->size = size;
    memcpy(ret->data, data, size * sizeof(uint8_t));
    
    return ret;
}

void payloadFree(payload* p){
    free(p->data);
    
    free(p);
}

void encodedArrayFree(encodedpacketarray* a){
    int i;
    
    for(i=0; i < a->nPackets; i++){
        encodedPacketFree(a->packets[i]);
    }
    
    free(a->packets);
    
    free(a);
}

encodedpacket* encodedPacketCopy(encodedpacket p){
    // Return a new encoded packet, copied from p
    encodedpacket* ret = malloc(sizeof(encodedpacket));
    ret->payload = payloadCreate(p.payload->size, p.payload->data);
    ret->coeffs = malloc(sizeof(coeffs));
    
    ret->coeffs->n = p.coeffs->n;
    ret->coeffs->start1 = p.coeffs->start1;
    ret->coeffs->start = malloc(p.coeffs->n * sizeof(uint16_t));
    memcpy(ret->coeffs->start, p.coeffs->start, p.coeffs->n * sizeof(uint16_t));
    ret->coeffs->size = malloc(p.coeffs->n * sizeof(uint16_t));
    memcpy(ret->coeffs->size, p.coeffs->size, p.coeffs->n * sizeof(uint16_t));
    ret->coeffs->hdrSize = malloc(p.coeffs->n * sizeof(uint8_t));
    memcpy(ret->coeffs->hdrSize, p.coeffs->hdrSize, p.coeffs->n * sizeof(uint8_t));
    ret->coeffs->alpha = malloc(p.coeffs->n * sizeof(uint8_t));
    memcpy(ret->coeffs->alpha, p.coeffs->alpha, p.coeffs->n * sizeof(uint8_t));
    
    return ret;
}

void encodedArrayAppend(encodedpacketarray* a, encodedpacket* p){
    a->packets = realloc(a->packets, (a->nPackets + 1) * sizeof(encodedpacket*));
    
    a->packets[a->nPackets] = p;
    a->nPackets++;
}

void clearArrayAppend(clearpacketarray* a, clearpacket* p){
    a->packets = realloc(a->packets, (a->nPackets + 1) * sizeof(clearpacket*));

    a->packets[a->nPackets] = p;
    a->nPackets++;
}

void clearArrayFree(clearpacketarray* a){
    int i;
    
    for(i=0; i < a->nPackets; i++){
        clearPacketFree(a->packets[i]);
    }
    
    free(a->packets);
    
    free(a);
}

void clearArrayRemove(clearpacketarray* a, int index){
    int i;
    clearPacketFree(a->packets[index]);
    for(i=index; i<a->nPackets-1; i++){
        a->packets[i] = a->packets[i+1];
    }
    a->packets = realloc(a->packets, (a->nPackets -1) * sizeof(clearpacket*));
    a->nPackets--;
}
