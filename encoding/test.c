#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "utils.h"
#include "galois_field.h"
#include "matrix.h"
#include "packet.h"
#include "coding.h"

#define PACKET_LENGTH 10
#define CLEAR_PACKETS 10
#define LOSS 0.1


int galoisTest(){
    uint8_t a,b,c;
    int isOk = true;
    
    // Addition :
    a = 0x01; b = 0x0A; c = 0x0B;
    if((gadd(a,b) != c)){
        printf("Galois addition failed\n");
        isOk = false;
    }
    
    // Multiplication :
    if((gmul(a,b) != b) || (gmul(0x02, 0x02)!=0x04) ){
        printf("Galois multiplication failed\n");
        isOk = false;
    }
    
    // Division :
    if((gdiv(b,a) != b) || (gdiv(0x02, 0x02)!=0x01)){
        printf("Galois division failed\n");
        isOk = false;
    }
    
    return isOk;
}

int matrixTest(){
    int isOk = true;
    matrix* a, *b, *identity, *result;
    
    // Create & Destroy Matrices
    a = getRandomMatrix(1000,1000);
    b = getRandomMatrix(1000,1000);
    mFree(a);
    mFree(b);
    
    
    // Gaussian elimination
    a = getRandomMatrix(4, 4);
    identity = getIdentityMatrix(4);
    b = mGauss(*a);
    result = mMul(*a, *b);
    
    if(!mEqual(*identity, *result)){
        printf("matrix gauss or multiplication failed\n");
        isOk = false;
    }
    
    mFree(a);mFree(b);mFree(identity);mFree(result);
    
    mFree(mCreate(0,0));
    
    return isOk;
}

int packetTest(){
    int isOk = true;
    uint8_t data[2] = {0xFF, 0xFF};
    
    clearpacket* cp = clearPacketCreate(10, 2, data);
    encodedpacket* ep = malloc(sizeof(encodedpacket));
    ep->payload = payloadCreate(2, data);
    ep->coeffs = malloc(sizeof(coeffs));
    ep->coeffs->alpha = malloc(2 * sizeof(uint8_t));
    ep->coeffs->size = malloc(2 * sizeof(uint16_t));
    ep->coeffs->start = malloc(2 * sizeof(uint16_t));
    
    clearPacketFree(cp);
    encodedPacketFree(ep);
    
    return isOk;
}

int maxMinTest(){
    if((max(1,2) == 2) && (min(2,1) == 1)){
        return true;
    } else {
        printf("Min/Max failed\n");
        return false;
    }
}

int codingTest(){
    int i,j,k;
    
    // Transient variables
    clearpacket* currentClearPacket;
    encodedpacketarray* encoderReturn;
    clearpacketarray* decoderReturn;
    
    encoderstate* encoderState = encoderStateInit();
    decoderstate* decoderState = decoderStateInit();
        
    // Generate clear packets
    matrix* Ps = getRandomMatrix(CLEAR_PACKETS, PACKET_LENGTH);
    printf("Original Packets : \n");
    mPrint(*Ps);
    
    // Pass packets to the clear handler and pipe the output to the coded handler
    for(i=0; i<CLEAR_PACKETS; i++){
        printf("\n~~ Round start : generating clear packet #%d.\n", i);
        
        if(i != 0){
            // Form a correct clear packet :
            currentClearPacket = clearPacketCreate(i * PACKET_LENGTH, PACKET_LENGTH, Ps->data[i]);
            //currentClearPacket = clearPacketCreate((uint16_t)random(), (uint16_t)random(), Ps->data[i]);
            currentClearPacket->type = TYPE_DATA;
        } else {
            // Form a CONTROL packet :
            currentClearPacket = clearPacketCreate(0, 10, Ps->data[0]);
            currentClearPacket->type = TYPE_CONTROL;
        }
        
        // Handle it
        encoderReturn = handleInClear(*currentClearPacket, *encoderState);
        printf("%d encoded packets generated during this round.\n", encoderReturn->nPackets);
        
        // Pipe the generated packets to the coded handler, with loss
        for(j = 0; j < encoderReturn->nPackets;j++){
            if(((1.0 * random())/RAND_MAX) < LOSS){
                printf("\nEncoded packet #%d has been lost\n", j);
            } else {
                printf("\nReceived encoded packet #%d\n", j);
                decoderReturn = handleInCoded(*(encoderReturn->packets[j]), *decoderState);
                printf("%d clear packet recovered during this round.\n", decoderReturn->nPackets);
                for(k = 0; k < decoderReturn->nPackets;k++){ // For each clear packets returned
                    // Send packets to the TCP application
                    printf("Sending packet to the application...\n");
                    clearPacketPrint(*(decoderReturn->packets[k]));
                    encoderState->lastByteAcked = decoderReturn->packets[k]->indexStart + decoderReturn->packets[k]->payload->size;
                }
                clearArrayFree(decoderReturn);
            }
        }
        
        encodedArrayFree(encoderReturn);
        clearPacketFree(currentClearPacket);
    }
    
    // Clean
    mFree(Ps);
    decoderStateFree(decoderState);
    encoderStateFree(encoderState);
    
    return true;
}

int main(int argc, char **argv){    
    if(galoisTest() && matrixTest() && packetTest() && maxMinTest() && codingTest()){
        printf("All test passed.\n");
        return 0;
    } else {
        printf("Testing failed.\n");
        return -1;
    }
}
