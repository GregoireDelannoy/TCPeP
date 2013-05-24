/* Copyright 2013 Gregoire Delannoy
 * 
 * This file is a part of TCPep.
 * 
 * TCPeP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "decoding.h"

int appendCodedPayload(decoderstate* state, uint8_t* coeffsVector, uint8_t* dataVector, int blockNo);
void extractData(decoderstate* state);

int isZeroAndOneAt(uint8_t* vector, int index, int size);

void countLoss(decoderstate state, uint16_t* lost, uint16_t* total);

void handleInCoded(decoderstate* state, uint8_t* buffer, int size){
    do_debug("in handleInCoded\n");
    datapacket* packet = bufferToData(buffer, size);
    matrix *coeffs, *tmp;
    int bufLen, i, delta;
    uint8_t* dataVector;
    uint8_t* coeffVector;
    uint8_t ackBuffer[100];
    uint16_t loss, total;
    
    //printf("Data received :\n");
    //dataPacketPrint(*packet);
    
    do_debug("p->packetNumber = %2x\n", packet->packetNumber);
    
    // ~~ Allocate blocks & coefficient matrix if necessary ~~
    while(state->currBlock + state->numBlock - 1 < packet->blockNo){
        do_debug("CurrBlock = %d, numBlock = %d, blockNo of received Data = %d\n", state->currBlock, state->numBlock, packet->blockNo);
        state->blocks = realloc(state->blocks, (state->numBlock + 1) * sizeof(matrix*));
        state->blocks[state->numBlock] = mCreate(BLKSIZE, PACKETSIZE);
        state->coefficients = realloc(state->coefficients, (state->numBlock + 1) * sizeof(matrix*));
        state->coefficients[state->numBlock] = mCreate(BLKSIZE, BLKSIZE);
        
        state->nPacketsInBlock = realloc(state->nPacketsInBlock, (state->numBlock + 1) * sizeof(int));
        state->nPacketsInBlock[state->numBlock] = 0;
        
        state->isSentPacketInBlock = realloc(state->isSentPacketInBlock, (state->numBlock + 1) * sizeof(int*));
        state->isSentPacketInBlock[state->numBlock] = malloc(BLKSIZE * sizeof(int));
        for(i = 0; i<BLKSIZE; i++){
            state->isSentPacketInBlock[state->numBlock][i] = false;
        } 
        
        state->numBlock ++;
    }
    
    if(packet->blockNo >= state->currBlock){
        if((packet->packetNumber & BITMASK_NO) >= state->nPacketsInBlock[packet->blockNo - state->currBlock]){ // Try to append
            // Compute coefficients
            coeffs = mCreate(1, BLKSIZE);
            dataVector = calloc(PACKETSIZE, sizeof(uint8_t));
            coeffVector = calloc(BLKSIZE, sizeof(uint8_t));
            if(((packet->packetNumber) & BITMASK_FLAG) ==  FLAG_CLEAR){
                coeffs->data[0][((packet->packetNumber) & BITMASK_NO)] = 1;
            } else if(((packet->packetNumber) & BITMASK_FLAG) ==  FLAG_CODED){
                srandom(packet->seqNo);
                tmp = getRandomMatrix(1, BLKSIZE);
                memcpy(coeffs->data[0], tmp->data[0], packet->packetNumber & BITMASK_NO);
                mFree(tmp);
            } else {
                printf("handleInCoded : received a bogus data packet. DIE.");
                exit(1);
            }
            
            // ~~ Append to the matrix and eventually decode ~~
            memcpy(dataVector, packet->payloadAndSize, packet->size);
            memcpy(coeffVector, coeffs->data[0], BLKSIZE);
            
            mFree(coeffs);
            
            if(appendCodedPayload(state, coeffVector, dataVector, packet->blockNo - state->currBlock)){
                do_debug("Received an innovative packet\n");
                state->stats_nInnovative++;
            } else {
                do_debug("Received packet was not innovative. Drop.\n");
                if(packet->blockNo - state->currBlock == 0){
                    state->stats_nAppendedNotInnovativeGaloisFirstBlock++;
                } else {
                    state->stats_nAppendedNotInnovativeGaloisOtherBlock++;
                }
            }
            
            free(dataVector);
            free(coeffVector);
        } else {
            do_debug("Received packet has NO chance to be innovative. Drop.\n");
            state->stats_nAppendedNotInnovativeCounter++;
        }
    } else {
        do_debug("Packet received for an outdated block. Drop.\n");
        state->stats_nOutdated++;
    }
    
    // ~~ Try to decode ~~
    if((state->numBlock > 0) && (state->nPacketsInBlock[0] > 0)){
        do_debug("Calling extractData() while numBlock = %d, currBlock = %d, nPacketInBlock[0] = %d\n", state->numBlock, state->currBlock, state->nPacketsInBlock[0]);
        extractData(state);
    }
    
    
    // ~~ Update the loss information buffer ~~
    delta = packet->seqNo - state->lastSeqReceived;
    if(delta > 0){
        for(i = state->lossBuffer->currentIndex + 1; i < (state->lossBuffer->currentIndex + delta); i++){
            state->lossBuffer->isReceived[i % LOSS_BUFFER_SIZE] = false;
        }
        state->lossBuffer->isReceived[(state->lossBuffer->currentIndex + delta) % LOSS_BUFFER_SIZE] = true;
        state->lossBuffer->currentIndex = (state->lossBuffer->currentIndex + delta) % LOSS_BUFFER_SIZE;
    } else {
        state->lossBuffer->isReceived[(state->lossBuffer->currentIndex + delta + LOSS_BUFFER_SIZE) % LOSS_BUFFER_SIZE] = true;
    }
    state->lastSeqReceived = max(state->lastSeqReceived, packet->seqNo);
    
    // ~~ Send an ACK back ~~
    ackpacket ack;
    ack.ack_dofs = malloc(DOFS_LENGTH * sizeof(uint8_t));
    for(i = 0; i < DOFS_LENGTH; i++){
        if(state->numBlock > i){ // If the i-th block is allocated
            // Include the number of packets received
            ack.ack_dofs[i] = state->nPacketsInBlock[i];
        } else {
            // Otherwise, let it just be zero
            ack.ack_dofs[i] = 0;
        }
    }
    
    ack.ack_seqNo = packet->seqNo;
    ack.ack_currBlock = state->currBlock;
    
    countLoss(*state, &loss, &total);
    ack.ack_loss = loss;
    ack.ack_total = total;
        
    //printf("ACK to send :\n");
    //ackPacketPrint(ack);
    
    ackPacketToBuffer(ack, ackBuffer, &bufLen);
    free(ack.ack_dofs);
    
    state->ackToSend = realloc(state->ackToSend, (state->nAckToSend + 1) * sizeof(uint8_t*));
    state->ackToSendSize = realloc(state->ackToSendSize, (state->nAckToSend + 1) * sizeof(int));

    state->ackToSend[state->nAckToSend] = malloc(bufLen * sizeof(uint8_t));
    memcpy(state->ackToSend[state->nAckToSend], ackBuffer, bufLen);
    state->ackToSendSize[state->nAckToSend] = bufLen;
    state->nAckToSend ++;
    
    free(packet->payloadAndSize);
    free(packet);
}

decoderstate* decoderStateInit(){
    int i;
    decoderstate* ret = malloc(sizeof(decoderstate));
    
    ret->blocks = 0;
    ret->coefficients = 0;
    
    ret->currBlock = 0;
    ret->numBlock = 0;
    ret->nPacketsInBlock = 0;
    ret->isSentPacketInBlock = 0;
    
    ret->lossBuffer = malloc(sizeof(lossInformationBuffer));
    for(i = 0; i < LOSS_BUFFER_SIZE; i++){// Initialize the counter
        ret->lossBuffer->isReceived[i] = -1;
    }
    ret->lossBuffer->currentIndex = 0;
    
    ret->lastSeqReceived = 0;
    
    ret->dataToSend = 0;
    ret->nDataToSend = 0;
    
    ret->ackToSend = 0;
    ret->ackToSendSize = 0;
    ret->nAckToSend = 0;
    
    ret->stats_nAppendedNotInnovativeCounter = 0;
    ret->stats_nAppendedNotInnovativeGaloisFirstBlock = 0;
    ret->stats_nAppendedNotInnovativeGaloisOtherBlock = 0;
    ret->stats_nInnovative = 0;
    ret->stats_nOutdated = 0;

    return ret;
}

void decoderStateFree(decoderstate* state){
    int i;
    
    for(i = 0; i < state->numBlock; i++){
        mFree(state->blocks[i]);
        mFree(state->coefficients[i]);
        free(state->isSentPacketInBlock[i]);
    }
    if(state->numBlock > 0){
        free(state->nPacketsInBlock);
        free(state->blocks);
        free(state->coefficients);
        free(state->isSentPacketInBlock);
    }
    
    if(state->nDataToSend > 0){
        free(state->dataToSend);
    }
    
    for(i = 0; i < state->nAckToSend; i++){
        free(state->ackToSend[i]);
    }
    if(state->nAckToSend > 0){
        free(state->ackToSend);
        free(state->ackToSendSize);
    }
    
    free(state->lossBuffer);
    
    free(state);
}

int appendCodedPayload(decoderstate* state, uint8_t* coeffsVector, uint8_t* dataVector, int blockNo){
    do_debug("in appendCodedPayload\n");
    int i, index, isEmpty = true;
    uint8_t factor;
    // Index = index of the first non-zero element
    for(index=0; index <= BLKSIZE; index++){
        if(index == BLKSIZE){
            return false;
        }
        if(coeffsVector[index] != 0x00){
            break;
        }
    }
    // is the row[index] empty ?
    for(i=0; i < BLKSIZE; i++){
        if(state->coefficients[blockNo]->data[index][i] != 0x00){
            isEmpty = false;
            break;
        }
    }
    
    if(isEmpty){
        // Append reduced
        factor = coeffsVector[index];
        rowReduce(coeffsVector, factor, BLKSIZE);
        rowReduce(dataVector, factor, PACKETSIZE);
        memcpy(state->blocks[blockNo]->data[index], dataVector, PACKETSIZE);
        memcpy(state->coefficients[blockNo]->data[index], coeffsVector, BLKSIZE);
        state->nPacketsInBlock[blockNo] ++;
        return true;
    } else {
        // Try to eliminate
        factor = coeffsVector[index];
        rowMulSub(coeffsVector, state->coefficients[blockNo]->data[index], factor, BLKSIZE);
        rowMulSub(dataVector, state->blocks[blockNo]->data[index], factor, PACKETSIZE);
        
        // Recursive call
        return appendCodedPayload(state, coeffsVector, dataVector, blockNo);
    }
    
}

int isZeroAndOneAt(uint8_t* vector, int index, int size){
    int i;
    for(i = 0; i < size; i++){
        if((i == index) && (vector[i] != 0x01)){
            return false;
        }
        if((i != index) && (vector[i] != 0x00)){
            return false;
        }
    }
    
    return true;
}

void extractData(decoderstate* state){
    do_debug("in extractData\n");
    // We only try to extract data on current block.
    
    int nPackets = state->nPacketsInBlock[0], i, firstNonDecoded = -1;
    uint8_t factor;
    uint16_t size;
    
    // Find the first non-decoded line :
    for(i = 0; i<nPackets; i++){
        // Look for decoded packets to send
        if((isZeroAndOneAt(state->coefficients[0]->data[i], i, BLKSIZE)) && !(state->isSentPacketInBlock[0][i])){
                memcpy(&size, state->blocks[0]->data[i], 2);
                size = ntohs(size);
                do_debug("Got a new decoded packet of size %u to send to the application ! o/\n", size);
                
                // Append to the sending buffer
                state->dataToSend = realloc(state->dataToSend, (state->nDataToSend + size) * sizeof(uint8_t));
                memcpy(state->dataToSend + state->nDataToSend, state->blocks[0]->data[i] + 2, size);
                state->nDataToSend += size;
                
                state->isSentPacketInBlock[0][i] = true;
        }
        if( ! isZeroAndOneAt(state->coefficients[0]->data[i], i, BLKSIZE)){
            firstNonDecoded = i;
            break;
        }
    }
    
    do_debug("FirstNonDecoded = %d\n", firstNonDecoded);
    
    if(firstNonDecoded == -1){
        if((nPackets == BLKSIZE) && (state->isSentPacketInBlock[0][BLKSIZE - 1])){
            // The entire block has been decoded AND sent 
            do_debug("An entire block has been decoded and sent, switch to next block.\n");
            
            mFree(state->blocks[0]);
            mFree(state->coefficients[0]);
            free(state->isSentPacketInBlock[0]);
            
            for(i = 0; i < state->numBlock - 1; i++){
                state->blocks[i] = state->blocks[i+1];
                state->coefficients[i] = state->coefficients[i+1];
                state->nPacketsInBlock[i] = state->nPacketsInBlock[i+1];
                state->isSentPacketInBlock[i] = state->isSentPacketInBlock[i+1];
            }
            
            state->numBlock--;
            state->currBlock++;
            
            state->blocks = realloc(state->blocks, state->numBlock * sizeof(matrix*));
            state->coefficients = realloc(state->coefficients, state->numBlock * sizeof(matrix*));
            state->isSentPacketInBlock = realloc(state->isSentPacketInBlock, state->numBlock * sizeof(int*));
            state->nPacketsInBlock = realloc(state->nPacketsInBlock, state->numBlock * sizeof(int));
        }
        
        return;
    }
    
    // Try to decode it
    for(i = 0; i<nPackets; i++){
        if(i!=firstNonDecoded){
            factor = state->coefficients[0]->data[firstNonDecoded][i];
            rowMulSub(state->coefficients[0]->data[firstNonDecoded], state->coefficients[0]->data[i], factor, BLKSIZE);
            rowMulSub(state->blocks[0]->data[firstNonDecoded], state->blocks[0]->data[i], factor, PACKETSIZE);
        }
    }
    
    if(isZeroAndOneAt(state->coefficients[0]->data[firstNonDecoded], firstNonDecoded, BLKSIZE)){
        // We decoded something => call recursively, in case there's something else waiting, or we finished the block
        do_debug("Something got decoded\n");
        extractData(state);
    }
}


void decoderStatePrint(decoderstate state){
    uint16_t lost, total;
    printf("Decoder state : \n");
    printf("\tCurrent block = %u\n", state.currBlock);
    printf("\tNumber of blocks = %d\n", state.numBlock);
    printf("\tBytes to send to the application = %d\n", state.nDataToSend);
    printf("\tACKs to send = %d\n", state.nAckToSend);
    
    countLoss(state, &lost, &total);
    printf("\tLost packets = %u, Total = %u, loss rate = %f\n", lost, total, 1.0 * lost/total);
    
    printf("\tInnov = %lu ; notInnovCounter = %lu ; notInnovGaloisFirstBlock = %lu ;notInnovGaloisOtherBlock = %lu ; outdated = %lu\n", state.stats_nInnovative, state.stats_nAppendedNotInnovativeCounter, state.stats_nAppendedNotInnovativeGaloisFirstBlock, state.stats_nAppendedNotInnovativeGaloisOtherBlock, state.stats_nOutdated);
}

void countLoss(decoderstate state, uint16_t* lost, uint16_t* total){
    int i;
    *lost = 0;
    *total = 0;
    
    for(i = 0; i < LOSS_BUFFER_SIZE; i++){
        switch(state.lossBuffer->isReceived[i]){
            case true:
                *total += 1;
                break;
            case false:
                *total += 1;
                *lost += 1;
                break;
            default:
                break;
        }
    }
}
