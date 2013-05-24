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

#include "encoding.h"

void onWindowUpdate(encoderstate* state);
struct timeval sentAtTime(packetsentinfo* table, int nPacketSent, uint32_t seqNo);
uint16_t sentBlock(packetsentinfo* table, int nPacketSent, uint32_t seqNo);
void addToPacketSentInfos(packetsentinfo** table, int* nPacketSent, uint32_t seqNo, uint16_t blockNo, struct timeval sentAtTime);
void removeFromPacketSentInfos(packetsentinfo** table, int* nPacketSent, uint32_t seqNo);

block blockCreate();
void blockFree(block b);

void sendFromBlock(encoderstate* state, int blockNo);

void generateEncodedPayload(matrix data, int nPackets, uint32_t seed, uint8_t* buffer, int* bufLen);

void onWindowUpdate(encoderstate* state){
    //printf("in onWindowUpdate\n");
    if(!state->isOutstandingData){
        printf("No outstanding data. Return\n");
        return;
    }
    
    int sentInThisRound = true, totalInFlight = 0, i;
    int* nPacketsInFlight = calloc(state->numBlock, sizeof(int));
    struct timeval timeOfArrival, currentTime;
    uint16_t sentBlockNumber;
    
    // ~~ Count packets in flight (total, and for each block) ~~
    gettimeofday(&currentTime, NULL);
    //do_debug("Current Time : (%d,%d)\n", (int)currentTime.tv_sec, (int)currentTime.tv_usec);
    for(i = state->seqNo_Una; i < state->seqNo_Next; i++){
        timeOfArrival = sentAtTime(*(state->packetSentInfos), *(state->nPacketSent), i);
        if(timeOfArrival.tv_sec == 0){
            continue;
        }
        
        if(state->shortTermRttAverage != 0){
            addUSec(&timeOfArrival, (state->shortTermRttAverage * INFLIGHT_FACTOR) + COMPUTING_DELAY);
        } else {
            addUSec(&timeOfArrival, 10000000); // Add 10 seconds, if RTT = 0
        }
        
        if(isSooner(currentTime, timeOfArrival)){ // All packets
            //printf("Packet #%d might still be in flight\n", i);
            sentBlockNumber = sentBlock(*(state->packetSentInfos), *(state->nPacketSent), i);
            
            if(sentBlockNumber - state->currBlock >= 0){
                nPacketsInFlight[sentBlockNumber - state->currBlock] ++;
            }
            totalInFlight++;
        }
    }
    
    do_debug("%d packets in flight, state->nDataToSend = %d\n", totalInFlight, state->nDataToSend);
    totalInFlight = max(totalInFlight, state->nDataToSend); // Ensure that you don't send more than congestionWindow on a single round, even with really low RTT.
    
    
    // ~~ If we're allowed to send, find a block that would be worth it ~~
    //printf("\nBefore while statement, totalInFlight = %d, congWin = %f\n", totalInFlight, state->congestionWindow);
    while((totalInFlight < ((int)state->congestionWindow)) && (sentInThisRound)){
        sentInThisRound = false;
        for(i = 0; i < state->numBlock; i++){
            //printf("Block %d should receive ~%f packets while %d are known and %u dofs have been ack-ed\n", i, (1 - state->p) * nPacketsInFlight[i], state->blocks[i].nPackets, state->blocks[i].dofs);
            
            if((ceilf(((1 - state->p) * nPacketsInFlight[i])) < (state->blocks[i].nPackets - state->blocks[i].dofs))){
                //printf("Sending from block #%d\n", i);
                sendFromBlock(state, i);
                totalInFlight ++;
                nPacketsInFlight[i]++;
                sentInThisRound = true;
                break;
            }
        }
        if(totalInFlight == 0){
            //printf("TotalInFlight = 0 while win = %f => All available data has been transfered to the other side\n", state->congestionWindow);
            state->isOutstandingData = false;
        }
    }
    
    //printf("After : totalInFlight = %d\n", totalInFlight);
    
    free(nPacketsInFlight);
}

void handleInClear(encoderstate* state, uint8_t* buffer, int size){
    do_debug("in handleInClear\n");
    // We just have to put data in the next available packet
    int sizeAllocated = 0, i;
    uint16_t currentWriteSize, tmp16;
    
    while(sizeAllocated < size){
        for(i = 0; (i < state->numBlock) && (sizeAllocated < size); i++){ // Look in already allocated blocks
            if(state->blocks[i].nPackets < BLKSIZE){
                currentWriteSize = min(PACKETSIZE - 2, size - sizeAllocated);
                tmp16 = htons(currentWriteSize); // Write the size on a uint16.
                memcpy(state->blocks[i].dataMatrix->data[state->blocks[i].nPackets], &tmp16, 2);
                memcpy(state->blocks[i].dataMatrix->data[state->blocks[i].nPackets] + 2, buffer + sizeAllocated, currentWriteSize);
                
                sizeAllocated += currentWriteSize;
                state->blocks[i].nPackets ++;
                do_debug("Appended %d bytes in the block #%d which now contains %d packets\n", currentWriteSize, i, state->blocks[i].nPackets);
                i = -1;
            }
        }
        
        if(sizeAllocated < size){ // Not all data fit in the already allocated blocks => create one
            state->blocks = realloc(state->blocks, (state->numBlock + 1) * sizeof(block));
            state->blocks[state->numBlock] = blockCreate();
            state->numBlock++;
        }
    }
    
    state->isOutstandingData = true;
    
    onWindowUpdate(state);
}

void onTimeOut(encoderstate* state){
    printf("in onTimeOut\n");
    state->slowStartMode = true;
    state->congestionWindow = BASE_WINDOW;
    state->timeOutCounter++;
    
    // ~~ Set time for the next timeOut event ~~
    if(state->isOutstandingData){
        gettimeofday(&(state->nextTimeout), NULL);
        addUSec(&(state->nextTimeout), (state->timeOutCounter * (COMPUTING_DELAY + (TIMEOUT_FACTOR * state->longTermRttAverage))));
    } else { // No data left to send... let the TO be ~long !
        state->nextTimeout.tv_sec = 0;
        state->nextTimeout.tv_usec = 0;
        
        addUSec(&(state->nextTimeout), (1 + state->timeOutCounter) * TIMEOUT_INCREMENT);
    }
    
    onWindowUpdate(state);
}

void onAck(encoderstate* state, uint8_t* buffer, int size){
    do_debug("in onAck :\n");
    ackpacket* ack = bufferToAck(buffer, size);
    int i, currentRTT;
    float delta;
    
    //printf("ACK received :\n");
    //ackPacketPrint(*ack);
    
    // Arbitrary idea : if ACK < seqNo_una (= sequence already ACKed somehow) => Do not consider it
    if(ack->ack_seqNo < state->seqNo_Una){
        do_debug("Outdated ACK (n = %d while una = %d), Drop !\n", ack->ack_seqNo, state->seqNo_Una);
        free(ack->ack_dofs);
        free(ack);
        return;
    }
    
    // ~~ Estimate network parameters ~~
    gettimeofday(&(state->time_lastAck), NULL);
    struct timeval sentAt = sentAtTime(*(state->packetSentInfos), *(state->nPacketSent), ack->ack_seqNo);
    if(sentAt.tv_sec == 0){
        // The specified sequence number is unknown... better ignore this ACK !
        do_debug("Unknown/outdated sequence number, do not refresh parameters !\n");
        state->seqNo_Una = max(state->seqNo_Una, ack->ack_seqNo + 1);
        free(ack->ack_dofs);
        free(ack);
        return;
    }
    currentRTT = 1000000 * (state->time_lastAck.tv_sec - sentAt.tv_sec) + (state->time_lastAck.tv_usec - sentAt.tv_usec);
    //printf("RTT for current ACK = %d\n", currentRTT);
    
    // Actualize the RTT average
    if(state->longTermRttAverage != 0){
        state->longTermRttAverage = ((1 - SMOOTHING_FACTOR_LONG) * state->longTermRttAverage) + (SMOOTHING_FACTOR_LONG * currentRTT);
        state->shortTermRttAverage = ((1 - SMOOTHING_FACTOR_SHORT) * state->shortTermRttAverage) + (SMOOTHING_FACTOR_SHORT * currentRTT);
    } else { // We are initializing it
        state->longTermRttAverage = currentRTT;
        state->shortTermRttAverage = currentRTT;
    }
    
    //printf("Short-term RTT : %f, long-term :%f\n", state->shortTermRttAverage, state->longTermRttAverage);
    
    // Actualize the packet loss ratio
    state->p = 1.0 * ack->ack_loss / ack->ack_total;
    
    // ~~ Adjust current block ~~
    while(ack->ack_currBlock > state->currBlock){
        // Free acknowldeged blocks (and forget about packets sent for them !)
        blockFree(state->blocks[0]);
        for(i = 0; i < state->numBlock - 1; i++){
            (state->blocks)[i] = (state->blocks)[i+1];
        }
        state->blocks = realloc(state->blocks, (state->numBlock - 1) * sizeof(block));
        state->numBlock --;
        for(i = 0; i < *(state->nPacketSent); i++){
            uint32_t seqNo = (*(state->packetSentInfos))[i].seqNo;
            if(sentBlock(*(state->packetSentInfos), *(state->nPacketSent), seqNo) == state->currBlock){
                removeFromPacketSentInfos(state->packetSentInfos, state->nPacketSent, seqNo);
                i--;
            }
        }
        state->currBlock++;
    }
    for(i = 0; i < DOFS_LENGTH; i++){
        if(state->numBlock > i){
            state->blocks[i].dofs = max(state->blocks[i].dofs, ack->ack_dofs[i]);
        }
    }
    
    
    // ~~ Update Congestion window ~~
    do_debug("Congestion Window before actualizing = %f\n", state->congestionWindow);
    if(state->slowStartMode){
        state->congestionWindow += 1;
        if(state->congestionWindow > SS_THRESHOLD){
            state->slowStartMode = false;
        } 
    } else {// Congestion avoidance mode
        delta = 1 - (state->longTermRttAverage / state->shortTermRttAverage);
        if(delta < ALPHA){
            // Increase the window :
            state->congestionWindow += (INCREMENT / state->congestionWindow);
        } else if(delta > BETA) {
            // Decrease the window
            state->congestionWindow -= (INCREMENT / state->congestionWindow);
        }
        // If delta is in between, do not update the window
        
        // Avoid floating-point errors ; the window should never get lower than the BASE !
        if(state->congestionWindow < BASE_WINDOW){
            state->congestionWindow = BASE_WINDOW;
        }
    }
    do_debug("Congestion Window after actualizing = %f\n", state->congestionWindow);
    
    state->seqNo_Una = max(state->seqNo_Una, ack->ack_seqNo + 1);
    
    free(ack->ack_dofs);
    free(ack);
    
    if(state->congestionWindow > MAX_WINDOW){
        printf("Window reached maximum... DIE !\n");
        exit(1);
    }
    
    onWindowUpdate(state);
    
    // ~~ Set time for the next timeOut event ~~
    if(state->isOutstandingData){
        state->nextTimeout.tv_sec = state->time_lastAck.tv_sec;
        state->nextTimeout.tv_usec = state->time_lastAck.tv_usec;
        addUSec(&(state->nextTimeout), TIMEOUT_FACTOR * state->shortTermRttAverage);
    } else { // No data left to send... let the TO be infinite !
        state->nextTimeout.tv_sec = 0;
        state->nextTimeout.tv_usec = 0;
    }
    state->timeOutCounter = 0;
}

encoderstate* encoderStateInit(){
    encoderstate* ret = malloc(sizeof(encoderstate));
    
    ret->blocks = 0;
    ret->numBlock = 0;
    ret->packetSentInfos = malloc(sizeof(packetsentinfo*));
    *(ret->packetSentInfos) = 0;
    ret->nPacketSent = malloc(sizeof(int));
    *(ret->nPacketSent) = 0;
    ret->p = 0.0;
    ret->longTermRttAverage = 0;
    ret->shortTermRttAverage = 0;
    ret->seqNo_Next = 0;
    ret->seqNo_Una = 0;
    ret->congestionWindow = BASE_WINDOW;
    ret->currBlock = 0;
    ret->slowStartMode = true;
    ret->dataToSend = 0;
    ret->dataToSendSize = 0;
    ret->nDataToSend = 0;
    ret->time_lastAck.tv_sec = 0;
    ret->time_lastAck.tv_usec = 0;
    ret->nextTimeout.tv_sec = 0;
    ret->nextTimeout.tv_usec = 0;
    ret->isOutstandingData = false;
    ret->timeOutCounter = 0;
    
    return ret;
}

void encoderStateFree(encoderstate* state){
    int i;
    
    for(i = 0; i < state->numBlock; i++){
        mFree(state->blocks[i].dataMatrix);
    }
    if(state->numBlock > 0){
        free(state->blocks);
    }
    
    if(*(state->nPacketSent) > 0){
        free(*(state->packetSentInfos));
    }
    free(state->packetSentInfos);
    free(state->nPacketSent);
    
    for(i = 0; i < state->nDataToSend; i++){
        free(state->dataToSend[i]);
    }
    if(state->nDataToSend > 0){
        free(state->dataToSend);
        free(state->dataToSendSize);
    }
    
    free(state);
}

struct timeval sentAtTime(packetsentinfo* table, int nPacketSent, uint32_t seqNo){
    int i;
    for(i = 0; i<nPacketSent; i++){
        if(table[i].seqNo == seqNo){
            return table[i].sentAt;
        }
    }
    do_debug("sentAtTime queried for unknown seqNo : %u. *should not happen*\n", seqNo);
    struct timeval ret; // Note : this line will never be reached... Just for gcc not to complain
    ret.tv_sec = 0;
    ret.tv_usec = 0;
    return ret;
}

uint16_t sentBlock(packetsentinfo* table, int nPacketSent, uint32_t seqNo){
    int i;
    for(i = 0; i<nPacketSent; i++){
        if(table[i].seqNo == seqNo){
            return table[i].blockNo;
        }
    }
    do_debug("sentBlock queried for unexisting seqNo : %u. *should not happen*\n", seqNo);
    return 0;
}

void addToPacketSentInfos(packetsentinfo** table, int* nPacketSent, uint32_t seqNo, uint16_t blockNo, struct timeval sentAtTime){
    *table = realloc(*table, (*nPacketSent + 1) * sizeof(packetsentinfo));
    (*table)[*nPacketSent].seqNo = seqNo;
    (*table)[*nPacketSent].blockNo = blockNo;
    (*table)[*nPacketSent].sentAt.tv_sec = sentAtTime.tv_sec;
    (*table)[*nPacketSent].sentAt.tv_usec = sentAtTime.tv_usec;
    
    //printf("Added packet #%u fro block %d to the sent table\n", seqNo, blockNo);
    
    (*nPacketSent) ++;
}

void removeFromPacketSentInfos(packetsentinfo** table, int* nPacketSent, uint32_t seqNo){
    int indexOccurence, i;
    for(i = 0; i < *nPacketSent; i++){
        if((*table)[i].seqNo == seqNo){
            for(indexOccurence=i; indexOccurence < (*nPacketSent - 1); indexOccurence++){
                (*table)[indexOccurence].seqNo = (*table)[indexOccurence + 1].seqNo;
                (*table)[indexOccurence].blockNo = (*table)[indexOccurence + 1].blockNo;
                (*table)[indexOccurence].sentAt = (*table)[indexOccurence + 1].sentAt;
            }
            
            *table = realloc(*table, (*nPacketSent -1) * sizeof(packetsentinfo));
            (*nPacketSent) --;
            return;
        }
    }
    
    do_debug("removeFromPacketSent queried for unexisting seqNo. DIE.");
    exit(1);
}

block blockCreate(){
    block b;
    int i;
    b.dataMatrix = mCreate(BLKSIZE, PACKETSIZE);
    b.nPackets = 0;
    for(i = 0; i<BLKSIZE; i++){
        b.isSentPacket[i] = false;
    }
    
    b.dofs = 0;
    
    return b;
}

void blockFree(block b){
    mFree(b.dataMatrix);
}

void sendFromBlock(encoderstate* state, int blockNo){
    do_debug("in sendFromBlock\n");
    int i, bufLen;
    datapacket packet;
    uint16_t tmp16;
    uint8_t buffer[PACKETSIZE + 100];
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    
    // Actualize sent at & sent from block tables
    addToPacketSentInfos(state->packetSentInfos, state->nPacketSent, state->seqNo_Next, blockNo + state->currBlock, currentTime);
    
    // First, look for an unsent packet
    for(i = 0; i < state->blocks[blockNo].nPackets; i++){
        if( !(state->blocks[blockNo].isSentPacket[i])){
            // Generate the packet
            packet.blockNo = blockNo + state->currBlock;
            packet.packetNumber = (BITMASK_NO & i) | FLAG_CLEAR;
            packet.seqNo = state->seqNo_Next;
            memcpy(&tmp16, state->blocks[blockNo].dataMatrix->data[i], 2);
            packet.size = ntohs(tmp16) + 2;
            packet.payloadAndSize = malloc(packet.size * sizeof(uint8_t));
            memcpy(packet.payloadAndSize, state->blocks[blockNo].dataMatrix->data[i], packet.size);
            
            //printf("Data to send :\n");
            //dataPacketPrint(packet);
            
            // Marshall it
            dataPacketToBuffer(packet, buffer, &bufLen);
            
            // Append it to the data to send buffer
            state->dataToSend = realloc(state->dataToSend, (state->nDataToSend + 1) * sizeof(uint8_t*));
            state->dataToSendSize = realloc(state->dataToSendSize, (state->nDataToSend + 1) * sizeof(int));
            
            state->dataToSend[state->nDataToSend] = malloc(bufLen * sizeof(uint8_t));
            memcpy(state->dataToSend[state->nDataToSend], buffer, bufLen);
            state->dataToSendSize[state->nDataToSend] = bufLen;
            state->nDataToSend ++;
            
            state->blocks[blockNo].isSentPacket[i] = true;
            state->seqNo_Next ++;
            
            free(packet.payloadAndSize);
            
            return;
        }
    }
    
    // If not found, send an encoded packet, comprising every packet know in the block
    packet.blockNo = blockNo + state->currBlock;
    packet.packetNumber = (BITMASK_NO & (state->blocks[blockNo].nPackets)) | FLAG_CODED;
    packet.seqNo = state->seqNo_Next;
    generateEncodedPayload(*(state->blocks[blockNo].dataMatrix), state->blocks[blockNo].nPackets, state->seqNo_Next, buffer, &bufLen);
    packet.size = bufLen;
    packet.payloadAndSize = malloc(bufLen);
    memcpy(packet.payloadAndSize, buffer, bufLen);


    //printf("Data to send :\n");
    //dataPacketPrint(packet);
    
    // Marshall it
    dataPacketToBuffer(packet, buffer, &bufLen);

    // Append it to the data to send buffer
    state->dataToSend = realloc(state->dataToSend, (state->nDataToSend + 1) * sizeof(uint8_t*));
    state->dataToSendSize = realloc(state->dataToSendSize, (state->nDataToSend + 1) * sizeof(int));

    state->dataToSend[state->nDataToSend] = malloc(bufLen * sizeof(uint8_t));
    memcpy(state->dataToSend[state->nDataToSend], buffer, bufLen);
    state->dataToSendSize[state->nDataToSend] = bufLen;
    state->nDataToSend ++;

    state->seqNo_Next ++;
    free(packet.payloadAndSize);
}

void generateEncodedPayload(matrix data, int nPackets, uint32_t seed, uint8_t* buffer, int* bufLen){
    srandom(seed); // Initialize the PRNG with seed value
    matrix* coeffs = getRandomMatrix(1, nPackets);
    matrix* tmp = mCopy(data);
    tmp->nRows = nPackets;
    
    matrix* result = mMul(*coeffs, *tmp);
    
    memcpy(buffer, result->data[0], result->nColumns);
    *bufLen = result->nColumns;
    
    tmp->nRows = data.nRows;
    mFree(tmp);
    mFree(coeffs);
    mFree(result);
}

void encoderStatePrint(encoderstate state){
    printf("Encoder state : \n");
    printf("\tCurrent block = %u\n", state.currBlock);
    printf("\tNumber of blocks = %d\n", state.numBlock);
    printf("\tEncoded data to send = %d\n", state.nDataToSend);
    printf("\tCongestion window = %f\n", state.congestionWindow);
    printf("\tlong-term RTT = %f\n", state.longTermRttAverage);
    printf("\tshort-term RTT = %f\n", state.shortTermRttAverage);
    printf("\tLoss estimation = %f\n", state.p);
}
