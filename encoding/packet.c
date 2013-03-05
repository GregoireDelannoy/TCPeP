#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
    printf("Base start (start1): %d | ", packet.coeffs->start1);
    printf("Size1 : %d | ", packet.coeffs->size[0]);
    printf("alpha1 : %2x | ", packet.coeffs->alpha[0]);
    
    for(i = 1; i < packet.coeffs->n ; i++){
        printf("Base start (start%d): %d | ",i, packet.coeffs->start[i]);
        printf("Size%d : %d | ",i, packet.coeffs->size[i]);
        printf("alpha%d : %2x | ",i, packet.coeffs->alpha[i]);
    } 
    printf("\n");
    
    payloadPrint(*(packet.payload));
}

void encodedPacketFree(encodedpacket* p){
    if(p->coeffs != 0){
        // Clean coeffs
        free(p->coeffs->alpha);
        free(p->coeffs->start);
        free(p->coeffs->size);
        free(p->coeffs);
    }
    
    payloadFree(p->payload);
    free(p);
}

void clearPacketPrint(clearpacket packet){
    payloadPrint(*(packet.payload));
}

clearpacket* clearPacketCreate(int index, int size, uint8_t* data){
    clearpacket* ret = malloc(sizeof(clearpacket));
    payload* p = payloadCreate(size, data);
    
    ret->indexStart = index;
    ret->payload = p;
    
    return ret;
}

void clearPacketFree(clearpacket* p){
    payloadFree(p->payload);
    free(p);
}

payload* payloadCreate(int size, uint8_t* data){ // Create a payload, copying the data
    payload* ret = malloc(sizeof(payload));
    int i;
    ret->data = malloc(size * sizeof(uint8_t));
    
    ret->size = size;
    for(i=0; i<size; i++){
        ret->data[i] = data[i];
    }
    
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
