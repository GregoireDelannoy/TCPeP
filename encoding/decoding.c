#include "decoding.h"

int appendCodedPayload(decoderstate* state, uint8_t* coeffsVector, uint8_t* dataVector, int blockNo);
void extractData(decoderstate* state);

int isZeroAndOneAt(uint8_t* vector, int index, int size);

void handleInCoded(decoderstate* state, uint8_t* buffer, int size){
    printf("in handleInCoded\n");
    datapacket* packet = bufferToData(buffer, size);
    dataPacketPrint(*packet);
    matrix *coeffs, *tmp;
    int bufLen, i;
    uint8_t* dataVector;
    uint8_t* coeffVector;
    uint8_t ackBuffer[100];
    
    printf("p->packetNumber = %2x\n", packet->packetNumber);
    
    // ~~ Allocate blocks & coefficient matrix if necessary ~~
    while(state->currBlock + state->numBlock - 1 < packet->blockNo){
        printf("CurrBlock = %d, numBlock = %d, blockNo of received Data = %d\n", state->currBlock, state->numBlock, packet->blockNo);
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
            
            printf("Computed coefficient matrix for received packet : \n");
            mPrint(*coeffs);
            
            // ~~ Append to the matrix and eventually decode ~~
            memcpy(dataVector, packet->payloadAndSize, packet->size);
            memcpy(coeffVector, coeffs->data[0], BLKSIZE);
            
            mFree(coeffs);
            
            if(appendCodedPayload(state, coeffVector, dataVector, packet->blockNo - state->currBlock)){
                printf("Received an innovative packet\n");
            } else {
                printf("Received packet was not innovative. Drop.\n");
            }
            
            free(dataVector);
            free(coeffVector);
        } else {
            printf("Received packet has NO chance to be innovative. Drop.\n");
        }
    } else {
        printf("Packet received for an outdated block. Drop.\n");
    }
    
    // ~~ Try to decode ~~
    if((state->numBlock > 0) && (state->nPacketsInBlock[0] > 0)){
        extractData(state);
    }
    
    // ~~ Send an ACK back ~~
    ackpacket ack;
    ack.ack_seqNo = packet->seqNo;
    ack.ack_currBlock = state->currBlock;
    if(state->numBlock > 0){ // Note : we might not have a current block !
        ack.ack_currDof = state->nPacketsInBlock[0];
    } else {
        ack.ack_currDof = 0;
    }
    ackPacketPrint(ack);
    
    ackPacketToBuffer(ack, ackBuffer, &bufLen);
    
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
    decoderstate* ret = malloc(sizeof(decoderstate));
    
    ret->blocks = 0;
    ret->coefficients = 0;
    
    ret->currBlock = 0;
    ret->numBlock = 0;
    ret->nPacketsInBlock = 0;
    ret->isSentPacketInBlock = 0;
    
    ret->dataToSend = 0;
    ret->nDataToSend = 0;
    
    ret->ackToSend = 0;
    ret->ackToSendSize = 0;
    ret->nAckToSend = 0;

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
    
    free(state);
}

int appendCodedPayload(decoderstate* state, uint8_t* coeffsVector, uint8_t* dataVector, int blockNo){
    printf("in appendCodedPayload\n");
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
        mPrint(*(state->blocks[blockNo]));
        memcpy(state->coefficients[blockNo]->data[index], coeffsVector, BLKSIZE);
        state->nPacketsInBlock[blockNo] ++;
        return true;
    } else {
        // Try to eliminate
        factor = coeffsVector[index];
        mPrint(*(state->coefficients[blockNo]));
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
    printf("in extractData\n");
    // We only try to extract data on current block.
    
    //DEBUG :
    printf("CoeffMatrix : \n");
    mPrint(*(state->coefficients[0]));
    printf("dataMatrix : \n");
    mPrint(*(state->blocks[0]));
    
    
    int nPackets = state->nPacketsInBlock[0], i, firstNonDecoded = -1;
    uint8_t factor;
    uint16_t size;
    
    // Find the first non-decoded line :
    for(i = 0; i<nPackets; i++){
        // Look for decoded packets to send
        if((isZeroAndOneAt(state->coefficients[0]->data[i], i, BLKSIZE)) && !(state->isSentPacketInBlock[0][i])){
                memcpy(&size, state->blocks[0]->data[i], 2);
                size = ntohs(size);
                printf("Got a new decoded packet of size %u to send to the application ! o/\n", size);
                
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
    
    printf("FirstNonDecoded = %d\n", firstNonDecoded);
    
    if(firstNonDecoded == -1){
        if((nPackets == BLKSIZE) && (state->isSentPacketInBlock[0][BLKSIZE - 1])){
            // The entire block has been decoded AND sent 
            printf("An entire block has been decoded and sent, switch to next block.\n");
            
            mFree(state->blocks[0]);
            mFree(state->coefficients[0]);
            free(state->isSentPacketInBlock[0]);
            
            for(i = 0; i < state->numBlock - 1; i++){
                state->blocks[i] = state->blocks[i+1];
                state->coefficients[i] = state->coefficients[i+1];
                state->nPacketsInBlock[i] = state->nPacketsInBlock[i+1];
            }
            
            state->numBlock--;
            state->currBlock++;
            
            state->blocks = realloc(state->blocks, state->numBlock * sizeof(matrix*));
            state->coefficients = realloc(state->coefficients, state->numBlock * sizeof(matrix*));
            state->isSentPacketInBlock = realloc(state->isSentPacketInBlock, state->numBlock * sizeof(int*));
            state->nPacketsInBlock = realloc(state->nPacketsInBlock, state->numBlock * sizeof(int));
        }
        
        printf("Nothing new decoded this round.\n");
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
        printf("Something got decoded\n");
        extractData(state);
    }
}


void decoderStatePrint(decoderstate state){
    printf("Decoder state : \n");
    printf("\tCurrent block = %u\n", state.currBlock);
    printf("\tNumber of blocks = %d\n", state.numBlock);
    printf("\tBytes to send to the application = %d\n", state.nDataToSend);
    printf("\tACKs to send = %d\n", state.nAckToSend);
}
