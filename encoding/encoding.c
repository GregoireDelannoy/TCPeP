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
    do_debug("in onWindowUpdate\n");
    int sentInThisRound = true, totalInFlight = 0, i;
    int* nPacketsInFlight = calloc(state->numBlock, sizeof(int));
    struct timeval timeOfArrival, currentTime;
    uint16_t sentBlockNumber;
    
    // ~~ Count packets in flight (total, and for each block) ~~
    gettimeofday(&currentTime, NULL);
    do_debug("Current Time : (%d,%d)\n", (int)currentTime.tv_sec, (int)currentTime.tv_usec);
    for(i = state->seqNo_Una; i < state->seqNo_Next; i++){
        timeOfArrival = sentAtTime(*(state->packetSentInfos), *(state->nPacketSent), i);
        if(timeOfArrival.tv_sec == 0){
            continue;
        }
        
        if(state->RTT != 0){
            addUSec(&timeOfArrival, (state->RTT * INFLIGHT_FACTOR) + COMPUTING_DELAY);
        } else {
            addUSec(&timeOfArrival, 10000000); // Add 10 seconds, if RTT = 0
        }
        
        if(isSooner(currentTime, timeOfArrival)){ // All packets
            do_debug("Packet might still be in flight\n");
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
    do_debug("Before for statement, totalInFlight = %d, congWin = %f , floor-ed = %d\n", totalInFlight, state->congestionWindow, (int)floor(state->congestionWindow));
    while((totalInFlight < floor(state->congestionWindow)) && (sentInThisRound)){
        sentInThisRound = false;
        for(i = 0; i < state->numBlock; i++){
            printf("Block %d should receive ~%f packets\n", i, (1 - state->p) * nPacketsInFlight[i]);
            if(( i == 0) && (roundf(((1 - state->p) * nPacketsInFlight[i])) < (state->blocks[i].nPackets - state->currDof))){
                // ~~ Send from the first block ~~
                printf("Sending from first block with state->p = %f, npackets in flight = %d, blocks.npacket = %d, currDof = %d\n", state->p, nPacketsInFlight[0], state->blocks[0].nPackets, state->currDof);
                sendFromBlock(state, i);
                totalInFlight ++;
                nPacketsInFlight[i]++;
                sentInThisRound = true;
                break;
            } else if((i != 0) && ((1 - state->p) * nPacketsInFlight[i]) < state->blocks[i].nPackets){
                // ~~ Send from the i-th block ~~
                printf("Sending from block %d with state->p = %f, npackets in flight = %d, blocks.npacket = %d\n",i, state->p, nPacketsInFlight[i], state->blocks[i].nPackets);
                sendFromBlock(state, i);
                totalInFlight ++;
                nPacketsInFlight[i]++;
                sentInThisRound = true;
                break;
            }
        }
        if(totalInFlight == 0){
            do_debug("All available data has been transfered to the other side\n");
            state->isOutstandingData = false;
        }
    }
    
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
    do_debug("in onTimeOut\n");
    state->slowStartMode = true;
    state->congestionWindow = BASE_WINDOW;
    
    // ~~ Set time for the next timeOut event ~~
    gettimeofday(&(state->nextTimeout), NULL);
    addUSec(&(state->nextTimeout), TIMEOUT_FACTOR * state->RTT);
    
    onWindowUpdate(state);
}

void onAck(encoderstate* state, uint8_t* buffer, int size){
    do_debug("in onAck :\n");
    ackpacket* ack = bufferToAck(buffer, size);
    int losses, i, currentRTT;
    
    // Arbitrary idea : if ACK < seqNo_una (= sequence already ACKed somehow) => Do not treat it
    if(ack->ack_seqNo < state->seqNo_Una){
        printf("Outdated ACK, Drop !\n");
        free(ack);
        return;
    }
    
    // ~~ Estimate network parameters ~~
    gettimeofday(&(state->time_lastAck), NULL);
    struct timeval sentAt = sentAtTime(*(state->packetSentInfos), *(state->nPacketSent), ack->ack_seqNo);
    if(sentAt.tv_sec == 0){
        // The specified sequence number is unknown... better to ignore this ACK !
        do_debug("Unknown/outdated sequence number, do not refresh parameters !\n");
        free(ack);
        return;
    }
    
    currentRTT = 1000000 * (state->time_lastAck.tv_sec - sentAt.tv_sec) + (state->time_lastAck.tv_usec - sentAt.tv_usec);
    do_debug("RTT for current ACK = %d\n", currentRTT);
    
    state->RTT = floor(((1 - SMOOTHING_FACTOR) * state->RTT) + (SMOOTHING_FACTOR * currentRTT));
    
    if(state->RTTmin != 0){
        state->RTTmin = min(state->RTTmin, state->RTT);
    } else {
        state->RTTmin = 10 * currentRTT;   // On start, RTTmin = 0 => Set it to the first RTT value encountered
    }
    
    if(ack->ack_seqNo >= state->seqNo_Una){
        losses = ack->ack_seqNo - state->seqNo_Una;
        printf("%d losses, p = %f\n", losses, state->p);
        
        if(losses == 0){
            state->p = state->p * (1.0 - 5 * SMOOTHING_FACTOR);
        } else {
            state->p = state->p * powf(1.0 - SMOOTHING_FACTOR, losses + 1.0) + (1 - powf(1.0 - SMOOTHING_FACTOR, (float)losses));
        }
        
        // Arbitrary idea : If there's no losses, reconciliate RTT & RTTmin.
        if(losses == 0){
            state->RTTmin = floor(((1 - SMOOTHING_FACTOR) * state->RTTmin) + (SMOOTHING_FACTOR * state->RTT));
        }
    }
    
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
            }
        }
        state->currBlock++;
        state->currDof = ack->ack_currDof;
    }
    
    state->currDof = max(state->currDof, ack->ack_currDof);
    
    // ~~ Update Congestion window ~~
    do_debug("Congestion Window before actualizing = %f\n", state->congestionWindow);
    if(state->slowStartMode){
        state->congestionWindow += 1.0;
        if(state->congestionWindow > SS_THRESHOLD){
            state->slowStartMode = false;
        }
    } else { // Congestion avoidance mode
        if(ack->ack_seqNo > state->seqNo_Una){
            do_debug("Multiplicative backoff with RTT = %lu & RTTmin = %lu\n", state->RTT, state->RTTmin);
            // Arbitrary rule : do NEVER shrink the window by more than 2/3
            state->congestionWindow = (max(0.66, ((1.0 * state->RTTmin) / (1.0 * state->RTT)))) * state->congestionWindow;
        } else {
            state->congestionWindow = state->congestionWindow + (1 / state->congestionWindow);
        }
        
        // Arbitrary rule, not from MIT's algos : congestion Win >= SS_THRESHOLD when in cong avoidance mode :
        state->congestionWindow = max(state->congestionWindow, SS_THRESHOLD);
    }
    do_debug("Congestion Window after actualizing = %f\n", state->congestionWindow);
    
    state->seqNo_Una = max(state->seqNo_Una, ack->ack_seqNo + 1);
    
    // ~~ Set time for the next timeOut event ~~
    gettimeofday(&(state->nextTimeout), NULL);
    addUSec(&(state->nextTimeout), TIMEOUT_FACTOR * state->RTT);
        
    free(ack);
    
    onWindowUpdate(state);
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
    ret->RTT = 0;
    ret->RTTmin = 0;
    ret->seqNo_Next = 0;
    ret->seqNo_Una = 0;
    ret->congestionWindow = BASE_WINDOW;
    ret->currBlock = 0;
    ret->currDof = 0;
    ret->slowStartMode = true;
    ret->dataToSend = 0;
    ret->dataToSendSize = 0;
    ret->nDataToSend = 0;
    ret->time_lastAck.tv_sec = 0;
    ret->time_lastAck.tv_usec = 0;
    ret->nextTimeout.tv_sec = 0;
    ret->nextTimeout.tv_usec = 0;
    ret->isOutstandingData = false;
    
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
        do_debug(" i = %d\n", i);
        if( !(state->blocks[blockNo].isSentPacket[i])){
            // Generate the packet
            packet.blockNo = blockNo + state->currBlock;
            packet.packetNumber = (BITMASK_NO & i) | FLAG_CLEAR;
            packet.seqNo = state->seqNo_Next;
            memcpy(&tmp16, state->blocks[blockNo].dataMatrix->data[i], 2);
            packet.size = ntohs(tmp16) + 2;
            packet.payloadAndSize = malloc(packet.size * sizeof(uint8_t));
            memcpy(packet.payloadAndSize, state->blocks[blockNo].dataMatrix->data[i], packet.size);
            
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
    printf("\tCurrent Degrees of Freedom = %u\n", state.currDof);
    printf("\tCongestion window = %f\n", state.congestionWindow);
    printf("\tRTT = %ld\n", state.RTT);
    printf("\tRTT min = %ld\n", state.RTTmin);
    printf("\tLoss estimation = %f\n", state.p);
}
