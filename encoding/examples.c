#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "structures.h"
#include "galois_field.h"
#include "matrix.h"
#include "packet.h"


#define true 1==1
#define false 1==0

#define PACKET_LENGTH 1500
#define CLEAR_PACKETS 20
#define ENCODED_PACKETS 210
#define LOSS 0.2
#define CODING_WINDOW 10
#define REDUNDANCY_FACTOR 1.5


//int addIfInnovative(encodedpacketpool* pool, encodedpacket packet){
    //uint8_t* rrefVector;
    //uint8_t* invertedVector;
    //uint8_t* factorVector;
    //int i;
    //int nullVector;
    //uint8_t factor;
    //int returnValue = true;

    //if(pool->nPackets == 0){ // First packet HAS to be added (plus, we're doing some mallocs here)
        //pool->nPackets = 1;
        //pool->packets = malloc(sizeof(encodedpacket));
        //pool->packets[0] = packet;
        //pool->rrefCoeffs = mcreate(1, packet.nCoeffs);
        //pool->invertedCoeffs = mcreate(1, packet.nCoeffs);
        
        //for(i = 0; i<packet.nCoeffs; i++){
            //pool->rrefCoeffs->data[0][i] = packet.coeffs[i];
            //pool->invertedCoeffs->data[0][i] = 0;
        //}
        //pool->invertedCoeffs->data[0][0] = 1; // First packet

        //factor = pool->rrefCoeffs->data[0][0];
        //rowReduce(pool->rrefCoeffs->data[0],factor , packet.nCoeffs);
        //rowReduce(pool->invertedCoeffs->data[0],factor, packet.nCoeffs);
    //} else {
        //rrefVector = malloc(packet.nCoeffs * sizeof(uint8_t));
        //factorVector = malloc(packet.nCoeffs * sizeof(uint8_t));
        
        //// Packet is innovative iff it can not be reduced to a row of zero with previous coefficients
        //for(i=0; i<packet.nCoeffs; i++){ // Fill vectors
            //rrefVector[i] = packet.coeffs[i];
        //}

        //for(i=0; i<pool->nPackets; i++){ // Eliminate
            //factorVector[i] = rrefVector[i];
            //rowMulSub(rrefVector, pool->rrefCoeffs->data[i], rrefVector[i], packet.nCoeffs);
        //}
        //nullVector = true;
        //for(i=0; i<packet.nCoeffs; i++){
            //if(rrefVector[i] != 0x00){
                //nullVector = false;
            //}
        //}
        //if(!nullVector && (rrefVector[pool->nPackets] != 0x00)){ // Packet is innovative. Add to the pool
            //printf("Packet is innovative ! Let's add it.\n");
            
            //// Compute inverted vector (basically, a unity vector to which we apply the same reduction operations)
            //invertedVector = malloc(packet.nCoeffs * sizeof(uint8_t));
            //for(i=0; i<packet.nCoeffs; i++){ // Fill vectors
                //invertedVector[i] = 0;
            //}
            //invertedVector[pool->nPackets] = 1;
            //for(i=0; i<pool->nPackets; i++){ // Eliminate
                //rowMulSub(invertedVector, pool->invertedCoeffs->data[i], factorVector[i], packet.nCoeffs);
            //}

            //// Reduce rref to 1 (and apply to inverted as well)
            //factor = rrefVector[pool->nPackets];
            //rowReduce(rrefVector, factor, packet.nCoeffs);
            //rowReduce(invertedVector, factor, packet.nCoeffs);

            //// Append to the pool of encoded packets
            //pool->packets = realloc(pool->packets, sizeof(encodedpacket) * (pool->nPackets + 1));
            //pool->packets[pool->nPackets] = packet;
            //mAppendVector(pool->rrefCoeffs, rrefVector);
            //mAppendVector(pool->invertedCoeffs, invertedVector);
            //pool->nPackets++;
        //} else {
            //printf("Packet is not innovative. Dropped.\n");
            //free(rrefVector);
            //returnValue = false;
        //}
        //free(factorVector);
    //}
    
    //return returnValue;
//}

//packetarray* extractPacket(encodedpacketpool pool){
    //// For each packet, try to reduce its coeff to "0..0 1 0.. 0" ; if done, we can decode.
    //encodedpacket currentCodedPacket = pool.packets[0];
    //uint8_t factor, temp;
    //int i, j, k, isReduced;
    //clearpacket* currentClearPacket;
    //packetarray* clearPackets = malloc(sizeof(packetarray));
    //clearPackets->nPackets = 0;
    //clearPackets->packets = 0;

    //for(i=0; i < pool.nPackets; i++){
        //currentCodedPacket = pool.packets[i];
        
        //for(j=0; j < pool.nPackets; j++){
            //if(i!=j){
                //factor = pool.rrefCoeffs->data[i][j];
                //rowMulSub(pool.rrefCoeffs->data[i], pool.rrefCoeffs->data[j], factor, currentCodedPacket.nCoeffs);
                //rowMulSub(pool.invertedCoeffs->data[i], pool.invertedCoeffs->data[j], factor, currentCodedPacket.nCoeffs);
            //}
        //}
        
        //// Test if we really have reduced it :
        //isReduced = true;
        //if(pool.rrefCoeffs->data[i][i] != 0x01){
            //isReduced = false;
        //}
        //for(j=0; (j < currentCodedPacket.nCoeffs) && isReduced; j++){
            //if(i != j){
                //if(pool.rrefCoeffs->data[i][j] != 0x00){
                    //isReduced = false;
                //}
            //}
        //}
        
        //if(isReduced){
            //currentClearPacket = malloc(sizeof(clearpacket));
            //currentClearPacket->payload.size = currentCodedPacket.payload.size;
            //currentClearPacket->payload.data = malloc(currentClearPacket->payload.size * sizeof(uint8_t));
            
            //for(j=0; j < currentClearPacket->payload.size; j++){
                //temp = 0x00;
                //for(k=0; k < pool.nPackets; k++){
                    //temp = gadd(temp, gmul(pool.packets[k].payload.data[j], pool.invertedCoeffs->data[i][k]));
                //}
                //currentClearPacket->payload.data[j] = temp;
            //}
            
            //// Add it to the array of decoded packets
            //clearPackets->packets = realloc(clearPackets->packets, (clearPackets->nPackets + 1) * sizeof(clearpacket));
            //clearPackets->packets[clearPackets->nPackets] = (void*)currentClearPacket;
            //clearPackets->nPackets++;
        //}
    //}
    
    //return clearPackets;
//}

encodedpacketarray* handleInClear(clearpacket clearPacket, clearpacketarray* clearPacketArray){ // Handle a new incoming clear packet and return array of coded packets to send
    static float NUM = 0;
    int i;
    
    // If the packet is a control packet, pass it along
    
    // If the packet is not already in the coding buffer, add it
    
    
    NUM = NUM + REDUNDANCY_FACTOR;
    
    for(i=0; i < NUM; i++){
        // Generate a random combination of packets in the coding window
        
        // Add headers
        
        // Add the packet to the array
    }
    
    // Keep only the fractional part
    NUM = NUM - (int)NUM;
    
    return 0;
}

clearpacketarray* handleInCoded(encodedpacket codedPacket, decoderbuffer* buffer){ // Handle a new incoming encoded packet and return the array of (eventually !) decoded clear packets
    
    return 0;
}


int main(int argc, char **argv){
    int i,j,k;
    // Persistent allocations
    clearpacketarray* encoderBuffer = malloc(sizeof(clearpacketarray));
    decoderbuffer* decoderBuffer = malloc(sizeof(decoderbuffer));
    
    // Transient allocations
    clearpacket* currentClearPacket;
    encodedpacketarray* encoderReturn;
    clearpacketarray* decoderReturn;
    
        
    // Generate clear packets
    matrix* Ps = getRandomMatrix(CLEAR_PACKETS, PACKET_LENGTH);
    printf("Original Packets : \n");
    mprint(*Ps);
    
    // Pass packets to the clear handler and pipe the output to the coded handler
    for(i=0; i<CLEAR_PACKETS; i++){
        printf("Round start : generating clear packet #%d.\n", i);
        // Form a correct clear packet :
        currentClearPacket = clearPacketCreate(i * PACKET_LENGTH, PACKET_LENGTH, Ps->data[i]);
        
        // Handle it
        encoderReturn = handleInClear(*currentClearPacket, encoderBuffer);
        printf("%d encoded packets generated during this round.\n", encoderReturn->nPackets);
        
        // Pipe the generated packets to the coded handler
        for(j = 0; j < encoderReturn->nPackets;j++){
            decoderReturn = handleInCoded(*(encoderReturn->packets[j]), decoderBuffer);
            printf("%d clear packet recovered during this round.\n", decoderReturn->nPackets);
            
            for(k = 0; j < decoderReturn->nPackets;k++){ // For each clear packets returned
                // Send packets to the TCP application
            }
        }
        
        encodedArrayFree(encoderReturn);
        clearArrayFree(decoderReturn);
        clearPacketFree(currentClearPacket);
    }
    
    // Clean
    mfree(Ps);
    clearArrayFree(encoderBuffer);
    mfree(decoderBuffer->rrefCoeffs);
    mfree(decoderBuffer->invertedCoeffs);
    encodedArrayFree(decoderBuffer->array);
    
    
    //// Gen Encoded Packets
    //encodedpacket** encodedData = malloc(ENCODED_PACKETS * (sizeof (encodedpacket*)));
    //int i;
    //for(i = 0; i < ENCODED_PACKETS; i++){
        ////printf("Generating encoded packet #%d\n", i);
        //encodedData[i] = malloc(sizeof(encodedpacket)); 
        //encodedData[i]->payload.size = PACKET_LENGTH;
        //encodedData[i]->nCoeffs = CLEAR_PACKETS;
        //// Generate Coefficients
        //matrix* currentCoeffs = getRandomMatrix(1, CLEAR_PACKETS);
        ////printf("Coeffs :\n");
        ////mprint(*currentCoeffs);
        //encodedData[i]->coeffs = currentCoeffs->data[0];
        //// Compute encoded packet
        //matrix* encodedPacket = mmul(*currentCoeffs, *Ps);  
        ////printf("Encoded packet #%d:\n", i);
        ////mprint(*encodedPacket);
        //encodedData[i]->payload.data = encodedPacket->data[0];
    //}

    //// Add in pool (receiver side ) ; yeld when found
    //encodedpacketpool* pool = malloc(sizeof(encodedpacketpool));
    //pool->nPackets = 0;


    //packetarray* clearArray;
    //int j,k;
    
    //for(i = 0; i < ENCODED_PACKETS; i++){
        //if(((1.0 * random())/RAND_MAX) < LOSS){
            //printf("\nEncoded packet #%d has been lost\n", i);
        //} else {
            //printf("\nReceived encoded packet #%d\n", i);
            //if(addIfInnovative(pool, *(encodedData[i]))){
                //printf("Innovative combination ; packet added to the pool :\n");
                ////poolPrint(*pool);
                //clearArray = extractPacket(*pool);
                //if(clearArray->nPackets > 0){
                    //printf("%d packets decoded in this round\n", clearArray->nPackets);
                    //for(j=0; j < clearArray->nPackets; j++){
                        //printf("Decoded packet #%d: \n", j);
                        //clearpacket* currentPacket = clearArray->packets[j];
                        //clearPacketPrint(*currentPacket);
                    //}
                //}
            //}
        //}
    //}
    
    
    
    return 0;
}

