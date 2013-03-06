#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // Needed for memcpy... don't ask why it's here !

#include "utils.h"
#include "galois_field.h"
#include "matrix.h"
#include "packet.h"

#define PACKET_LENGTH 1500
#define CLEAR_PACKETS 1000
#define LOSS 0.4
#define CODING_WINDOW 100
#define REDUNDANCY_FACTOR 2


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
        //pool->rrefCoeffs = mCreate(1, packet.nCoeffs);
        //pool->invertedCoeffs = mCreate(1, packet.nCoeffs);
        
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
    //~~
    static float NUM = 0; // WARNING ! Be cautious with that variable !
    //~~
    int i, j, isFound, currentNCodedPackets, currentBiggestPacket, currentIndex;
    encodedpacketarray* ret = malloc(sizeof(encodedpacketarray));
    ret->nPackets = 0;
    ret->packets = 0;
    
    //If the packet is a control packet, pass it along
    if(clearPacket.type == TYPE_CONTROL){
        printf("Received a control packet\n");
        encodedpacket* currentEncodedPacket = malloc(sizeof(encodedpacket));
        currentEncodedPacket->payload = payloadCreate(clearPacket.payload->size, clearPacket.payload->data);
        currentEncodedPacket->coeffs = 0;
        encodedArrayAppend(ret, currentEncodedPacket);
    } else if(clearPacket.type == TYPE_DATA) {
        // If the packet is not already in the coding buffer, add it. Suppose no repacketization for the moment
        isFound = false;
        for(i=0; i < clearPacketArray->nPackets && !isFound; i++){
            if(clearPacketArray->packets[i]->indexStart == clearPacket.indexStart){
                isFound = true;
            }
        }
        if(!isFound){
            printf("Handle Clear : Adding clear packet #%d to the buffer.\n", clearPacket.indexStart);
            clearArrayAppend(clearPacketArray, clearPacketCreate(clearPacket.indexStart, clearPacket.payload->size, clearPacket.payload->data));
        }
        
        
        NUM = NUM + REDUNDANCY_FACTOR;
        
        for(i=0; i < (int)NUM; i++){
            currentNCodedPackets = min(CODING_WINDOW, clearPacketArray->nPackets);
            // Generate a random combination of packets in the coding window
            matrix* currentCoefficents = getRandomMatrix(1, currentNCodedPackets);
            
            // Find the biggest packet in the buffer
            currentBiggestPacket = 0;
            for(j=0; j<currentNCodedPackets; j++){
                currentBiggestPacket = max(currentBiggestPacket, clearPacketArray->packets[j]->payload->size);
            }
            
            // Compute the encoded payload, using matrices            
            matrix* payloads = mCreate(currentNCodedPackets, currentBiggestPacket);
            
            // Fill the Matrix with data from packets
            for(j=0; j<currentNCodedPackets; j++){
                memcpy(payloads->data[j], clearPacketArray->packets[j]->payload->data, clearPacketArray->packets[j]->payload->size);
                // Padd it with zeroes
                memset(payloads->data[j] + clearPacketArray->packets[j]->payload->size, 0x00, currentBiggestPacket - clearPacketArray->packets[j]->payload->size);
            }
            
            // Compute the encoded payload
            matrix* encodedPayload = mMul(*currentCoefficents, *payloads);
            
            encodedpacket* currentEncodedPacket = malloc(sizeof(encodedpacket));
            currentEncodedPacket->payload = payloadCreate(currentBiggestPacket, encodedPayload->data[0]);
            
            // Add headers
            currentEncodedPacket->coeffs = malloc(sizeof(coeffs));
            currentEncodedPacket->coeffs->n = currentNCodedPackets;
            currentEncodedPacket->coeffs->start1 = clearPacketArray->packets[0]->indexStart;
            currentIndex = clearPacketArray->packets[0]->indexStart;
            currentEncodedPacket->coeffs->start = malloc(currentNCodedPackets * sizeof(uint16_t));
            currentEncodedPacket->coeffs->size = malloc(currentNCodedPackets * sizeof(uint16_t));
            currentEncodedPacket->coeffs->alpha = malloc(currentNCodedPackets * sizeof(uint8_t));
            
            for(j=0; j<currentNCodedPackets; j++){
                currentEncodedPacket->coeffs->start[j] = (uint16_t) (clearPacketArray->packets[j]->indexStart - currentIndex);
                currentEncodedPacket->coeffs->size[j] = clearPacketArray->packets[j]->payload->size;
                currentEncodedPacket->coeffs->alpha[j] = currentCoefficents->data[0][j];
                currentIndex = clearPacketArray->packets[j]->indexStart;
            }
            
            // Add the packet to the return array
            encodedArrayAppend(ret, currentEncodedPacket);
            
            mFree(currentCoefficents);
            mFree(payloads);
            mFree(encodedPayload);
        }
        
        // Keep only the fractional part
        NUM = NUM - (int)NUM;
    }
    
    return ret;
}

clearpacketarray* handleInCoded(encodedpacket codedPacket, decoderpool* buffer){ // Handle a new incoming encoded packet and return the array of (eventually !) decoded clear packets
    clearpacketarray* ret = malloc(sizeof(clearpacketarray));
    ret->nPackets = 0;
    ret->packets = 0;
    
    if(codedPacket.coeffs == 0){ // The packet is not really coded ; must be control
        printf("Handle coded : Received a control packet\n");
        clearpacket* currentClearPacket = malloc(sizeof(clearpacket));
        currentClearPacket->payload = payloadCreate(codedPacket.payload->size, codedPacket.payload->data);
        currentClearPacket->type = TYPE_CONTROL;
        clearArrayAppend(ret, currentClearPacket);
    } else {
        // Retrieve the coding vector
    }
    
    return ret;
}


int main(int argc, char **argv){
    int i,j,k;
    // Persistent allocations
    clearpacketarray* encoderBuffer = malloc(sizeof(clearpacketarray));
    encoderBuffer->nPackets = 0;
    encoderBuffer->packets = 0;
    decoderpool* decoderBuffer = malloc(sizeof(decoderpool));
    decoderBuffer->rrefCoeffs = mCreate(0,0);
    decoderBuffer->invertedCoeffs = mCreate(0,0);
    decoderBuffer->array = malloc(sizeof(encodedpacketarray));
    decoderBuffer->array->nPackets = 0;
    decoderBuffer->array->packets = 0;
    
    // Transient allocations
    clearpacket* currentClearPacket;
    encodedpacketarray* encoderReturn;
    clearpacketarray* decoderReturn;
    
        
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
            currentClearPacket->type = TYPE_DATA;
        } else {
            // Form a CONTROL packet :
            currentClearPacket = clearPacketCreate(0, 10, Ps->data[0]);
            currentClearPacket->type = TYPE_CONTROL;
        }
        
        // Handle it
        encoderReturn = handleInClear(*currentClearPacket, encoderBuffer);
        printf("%d encoded packets generated during this round.\n", encoderReturn->nPackets);
        
        // Pipe the generated packets to the coded handler, with loss
        for(j = 0; j < encoderReturn->nPackets;j++){
            if(((1.0 * random())/RAND_MAX) < LOSS){
                printf("\nEncoded packet #%d has been lost\n", j);
            } else {
                printf("\nReceived encoded packet #%d\n", j);
                decoderReturn = handleInCoded(*(encoderReturn->packets[j]), decoderBuffer);
            printf("%d clear packet recovered during this round.\n", decoderReturn->nPackets);
            
            for(k = 0; k < decoderReturn->nPackets;k++){ // For each clear packets returned
                // Send packets to the TCP application
                printf("Sending packet to the application...\n");
                clearPacketPrint(*(decoderReturn->packets[k]));
            }
            clearArrayFree(decoderReturn);
            }
        }
        
        encodedArrayFree(encoderReturn);
        clearPacketFree(currentClearPacket);
    }
    
    // Clean
    mFree(Ps);
    clearArrayFree(encoderBuffer);
    encodedArrayFree(decoderBuffer->array);
    mFree(decoderBuffer->rrefCoeffs);
    mFree(decoderBuffer->invertedCoeffs);
    free(decoderBuffer);
    
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
        ////mPrint(*currentCoeffs);
        //encodedData[i]->coeffs = currentCoeffs->data[0];
        //// Compute encoded packet
        //matrix* encodedPacket = mMul(*currentCoeffs, *Ps);  
        ////printf("Encoded packet #%d:\n", i);
        ////mPrint(*encodedPacket);
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

