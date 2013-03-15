#include "coding.h"

void printInfosTable(clearinfos* table, int n){
    int i;
    printf("Info table @%lu, containing %d entries :\n", (uint64_t)table, n);
    printf("|id : start : size : hdrSize|\n");
    for(i=0; i<n; i++){
        printf(" |%3d : %5u : %5u : %2u|\n", i, table[i].start, table[i].size, table[i].hdrSize);
    }
}

int biggestSizeTable(clearinfos* table, int n){
    int ret = 0, i;
    for(i=0; i<n; i++){
        ret = max(ret, table[i].size);
    }
    
    return ret;
}

int indexInTable(clearinfos* table, int n, uint32_t start){
    int i;
    for(i=0; i<n; i++){
        if(table[i].start == start){
            return i;
        }
    }
    
    printf("indexInTable : unable to find ! DIE.\n");
    exit(-1);
}

void addToInfosTable(clearinfos** table, int* n, encodedpacket packet){
    uint32_t currentStart;
    int i, j, isFound;
    
    currentStart = packet.coeffs->start1;
    for(i=0; i<packet.coeffs->n; i++){ // Loop into the coeffs (= loop into clear packets)
        isFound = false;
        currentStart += packet.coeffs->start[i];
        for(j=0; j<(*n) && !isFound; j++){ // Search in the table if already known
            if(currentStart == (*table)[j].start){
                isFound= true;
                
                // Test to ensure no repacketization
                if((*table)[j].size != packet.coeffs->size[i]){
                    printf("handleInCoded: REPACKETIZATION OCURRED ! BAD. WILL PROBABLY DIE.\n");
                }
            }
        }
        if(!isFound){ // Clear packet reference is new ; add it to the table
            (*table) = realloc((*table), (*n + 1) * sizeof(clearinfos));
            (*table)[*n].start = currentStart;
            (*table)[*n].size = packet.coeffs->size[i];
            (*table)[*n].hdrSize = packet.coeffs->hdrSize[i];
            *n = *n + 1;
        }
    }
}

int addIfInnovative(matrix* rrefCoeffs, matrix* invertedCoeffs, matrix* codedData, encodedpacket packet, clearinfos* infosTable, int decodingWindow){
    // Note : decodingWindow = table width
    uint8_t* rrefVector;
    uint8_t* invertedVector;
    uint8_t* factorVector;
    uint8_t* dataVector;
    int i, currentPacketNumber = rrefCoeffs->nRows, currentTableIndex;
    int nullVector;
    uint8_t factor;
    int returnValue = true;
    uint32_t currentStart;

    if(currentPacketNumber == 0){ // First packet HAS to be added
        rrefCoeffs->nRows = 1;
        rrefCoeffs->nColumns = decodingWindow;
        rrefCoeffs->data = malloc(sizeof(uint8_t*));
        rrefCoeffs->data[0] = malloc(decodingWindow * sizeof(uint8_t));
        memset(rrefCoeffs->data[0], 0x00, decodingWindow);
        
        invertedCoeffs->nRows = 1;
        invertedCoeffs->nColumns = decodingWindow;
        invertedCoeffs->data = malloc(sizeof(uint8_t*));
        invertedCoeffs->data[0] = malloc(decodingWindow * sizeof(uint8_t));
        memset(invertedCoeffs->data[0], 0x00, decodingWindow);
        
        codedData->nRows = 1;
        codedData->nColumns = biggestSizeTable(infosTable, decodingWindow);
        codedData->data = malloc(sizeof(uint8_t*));
        codedData->data[0] = malloc(biggestSizeTable(infosTable, decodingWindow) * sizeof(uint8_t));
        memset(codedData->data[0], 0x00, biggestSizeTable(infosTable, decodingWindow));
        
        currentStart = packet.coeffs->start1;
        for(i = 0; i<packet.coeffs->n; i++){ // Fill the Matrix
            currentStart += packet.coeffs->start[i];
            currentTableIndex = indexInTable(infosTable, decodingWindow, currentStart);
            
            rrefCoeffs->data[0][currentTableIndex] = packet.coeffs->alpha[i];
            memcpy(codedData->data[0], packet.payload->data, packet.payload->size);
        }
        invertedCoeffs->data[0][0] = 1; // First packet

        factor = rrefCoeffs->data[0][0];
        rowReduce(rrefCoeffs->data[0],factor , decodingWindow);
        rowReduce(invertedCoeffs->data[0],factor, decodingWindow);
        rowReduce(codedData->data[0],factor, codedData->nColumns);
    } else {
        rrefVector = calloc(decodingWindow, sizeof(uint8_t));
        factorVector = calloc(decodingWindow, sizeof(uint8_t));
        
        // Packet is innovative iff it can not be reduced to a row of zero with previous coefficients
        currentStart = packet.coeffs->start1;
        for(i = 0; i<packet.coeffs->n; i++){
            currentStart += packet.coeffs->start[i];
            currentTableIndex = indexInTable(infosTable, decodingWindow, currentStart);
            
            rrefVector[currentTableIndex] = packet.coeffs->alpha[i];
        }

        for(i=0; i<currentPacketNumber; i++){ // Eliminate
            factorVector[i] = rrefVector[i];
            rowMulSub(rrefVector, rrefCoeffs->data[i], rrefVector[i], min(decodingWindow, rrefCoeffs->nColumns));
        }
        
        nullVector = true;
        for(i=0; i<decodingWindow; i++){
            if(rrefVector[i] != 0x00){
                nullVector = false;
            }
        }
        if(!nullVector && (rrefVector[currentPacketNumber] != 0x00)){ // Packet is innovative. Add to the pool
            //printf("AddIf: Packet is innovative ! Let's add it.\n");
            
            // Compute inverted vector (basically, a unity vector to which we apply the same reduction operations)
            invertedVector = calloc(decodingWindow, sizeof(uint8_t));
            dataVector = calloc(max(codedData->nColumns, packet.payload->size) , sizeof(uint8_t));
            memcpy(dataVector, packet.payload->data, packet.payload->size);
            
            invertedVector[currentPacketNumber] = 1;
            for(i=0; i<currentPacketNumber; i++){ // Eliminate
                rowMulSub(invertedVector, invertedCoeffs->data[i], factorVector[i], min(decodingWindow, invertedCoeffs->nColumns));
                rowMulSub(dataVector, codedData->data[i], factorVector[i], codedData->nColumns);
            }

            // Reduce rref to 1 (and apply to inverted as well)
            factor = rrefVector[currentPacketNumber];
            rowReduce(rrefVector, factor, decodingWindow);
            rowReduce(invertedVector, factor, decodingWindow);
            rowReduce(dataVector, factor, codedData->nColumns);

            // Append to the pool of encoded packets => Need to correct the matrices size
            mGrow(rrefCoeffs, decodingWindow);
            mAppendVector(rrefCoeffs, rrefVector);
            mGrow(invertedCoeffs, decodingWindow);
            mAppendVector(invertedCoeffs, invertedVector);
            mGrow(codedData, max(codedData->nColumns, packet.payload->size));
            mAppendVector(codedData, dataVector);
        } else {
            //printf("AddIf: Packet is not innovative. Dropped.\n");
            free(rrefVector);
            returnValue = false;
        }
        free(factorVector);
    }
    
    return returnValue;
}

void extractClear(matrix* rrefCoeffs, matrix* invertedCoeffs, matrix* codedData,clearinfos* infosTable, int decodingWindow, uint32_t lastByteSent, clearpacketarray* clearPackets){
    // For each packet, try to reduce its coeff to "0..0 1 0.. 0" ; if done, we can decode.
    uint8_t factor;
    int i, j, isReduced;
    clearpacket* currentClearPacket;

    for(i=0; i < rrefCoeffs->nRows; i++){        
        // Eliminate, using the other packets/coefficients
        for(j=0; j < rrefCoeffs->nRows; j++){
            if(i!=j){
                factor = rrefCoeffs->data[i][j];
                if(factor != 0x00){
                    rowMulSub(rrefCoeffs->data[i], rrefCoeffs->data[j], factor, rrefCoeffs->nColumns);
                    rowMulSub(invertedCoeffs->data[i], invertedCoeffs->data[j], factor, invertedCoeffs->nColumns);
                    rowMulSub(codedData->data[i], codedData->data[j], factor, codedData->nColumns);
                }
            }
        }
        
        // Test if we really have reduced it :
        isReduced = true;
        if(rrefCoeffs->data[i][i] != 0x01){
            isReduced = false;
        }
        for(j=0; (j < rrefCoeffs->nColumns) && isReduced; j++){
            if(i != j){
                if(rrefCoeffs->data[i][j] != 0x00){
                    isReduced = false;
                }
            }
        }
        
        if(isReduced && (infosTable[i].start + (infosTable[i].size - infosTable[i].hdrSize) > lastByteSent)){
            // If isReduced, we got a vector like that : 0...0 1 0...0 in the rref matrix => The i-th clear packet has been decoded (and is actually in the codedData matrix)
            currentClearPacket = clearPacketCreate(infosTable[i].start, infosTable[i].size, infosTable[i].hdrSize, codedData->data[i]);
            
            // Add it to the array of decoded packets
            clearArrayAppend(clearPackets, currentClearPacket);
        }
    }
}

encodedpacketarray* handleInClear(clearpacket clearPacket, encoderstate state){ // Handle a new incoming clear packet and return array of coded packets to send
    int i, j, isFound, currentNCodedPackets, currentBiggestPacket, currentIndex;
    encodedpacketarray* ret = malloc(sizeof(encodedpacketarray));
    ret->nPackets = 0;
    ret->packets = 0;
    
    // Actualize the state according to the last acked byte (= we don't have to hold those packets anymore)
    printf("HiCl: size of the buffer before removing ACKed packets : %d\n", state.buffer->nPackets);
    printf("HiCl: state.lastByteAcked = %u\n", state.lastByteAcked);
    for(i=0; i<state.buffer->nPackets; i++){
        uint32_t currentLastByte = state.buffer->packets[i]->indexStart + state.buffer->packets[i]->payload->size - state.buffer->packets[i]->hdrSize;
        if(currentLastByte < state.lastByteAcked){
            printf("HiCl: Removing packet #%d from buffer (has been acked)\n", i);
            clearArrayRemove(state.buffer, i);
            i = 0;
        }
    }
    printf("HiCl: after : %d\n", state.buffer->nPackets);
    
    // If the packet is not already in the coding buffer, add it. Suppose no repacketization for the moment
    isFound = false;
    for(i=0; i < state.buffer->nPackets && !isFound; i++){
        if(state.buffer->packets[i]->indexStart == clearPacket.indexStart){
            isFound = true;
        }
    }
    if(!isFound){
        //printf("Handle Clear : Adding clear packet #%d to the buffer.\n", clearPacket.indexStart);
        clearArrayAppend(state.buffer, clearPacketCreate(clearPacket.indexStart, clearPacket.payload->size, clearPacket.hdrSize, clearPacket.payload->data));
    }
    
    *(state.NUM) = *(state.NUM) + REDUNDANCY_FACTOR;
    
    for(i=0; i < (int)(*(state.NUM)); i++){
        currentNCodedPackets = min(CODING_WINDOW, state.buffer->nPackets);
        // Generate a random combination of packets in the coding window
        matrix* currentCoefficents = getRandomMatrix(1, currentNCodedPackets);
        
        // Find the biggest packet in the buffer
        currentBiggestPacket = 0;
        for(j=0; j<currentNCodedPackets; j++){
            currentBiggestPacket = max(currentBiggestPacket, state.buffer->packets[j]->payload->size);
        }
        
        // Compute the encoded payload, using matrices            
        matrix* payloads = mCreate(currentNCodedPackets, currentBiggestPacket);
        
        // Fill the Matrix with data from packets
        for(j=0; j<currentNCodedPackets; j++){
            memcpy(payloads->data[j], state.buffer->packets[j]->payload->data, state.buffer->packets[j]->payload->size);
            // Padd it with zeroes
            memset(payloads->data[j] + state.buffer->packets[j]->payload->size, 0x00, currentBiggestPacket - state.buffer->packets[j]->payload->size);
        }
        
        // Compute the encoded payload
        matrix* encodedPayload = mMul(*currentCoefficents, *payloads);
        
        encodedpacket* currentEncodedPacket = malloc(sizeof(encodedpacket));
        currentEncodedPacket->payload = payloadCreate(currentBiggestPacket, encodedPayload->data[0]);
        
        // Add headers
        currentEncodedPacket->coeffs = malloc(sizeof(coeffs));
        currentEncodedPacket->coeffs->n = currentNCodedPackets;
        currentEncodedPacket->coeffs->start1 = state.buffer->packets[0]->indexStart;
        currentIndex = state.buffer->packets[0]->indexStart;
        currentEncodedPacket->coeffs->start = malloc(currentNCodedPackets * sizeof(uint16_t));
        currentEncodedPacket->coeffs->size = malloc(currentNCodedPackets * sizeof(uint16_t));
        currentEncodedPacket->coeffs->hdrSize = malloc(currentNCodedPackets * sizeof(uint8_t));
        currentEncodedPacket->coeffs->alpha = malloc(currentNCodedPackets * sizeof(uint8_t));
        
        for(j=0; j<currentNCodedPackets; j++){
            currentEncodedPacket->coeffs->start[j] = (uint16_t) (state.buffer->packets[j]->indexStart - currentIndex);                
            currentEncodedPacket->coeffs->size[j] = state.buffer->packets[j]->payload->size;
            currentEncodedPacket->coeffs->hdrSize[j] = state.buffer->packets[j]->hdrSize;
            currentEncodedPacket->coeffs->alpha[j] = currentCoefficents->data[0][j];
            currentIndex = state.buffer->packets[j]->indexStart;
        }
        
        // Add the packet to the return array
        encodedArrayAppend(ret, currentEncodedPacket);
        
        mFree(currentCoefficents);
        mFree(payloads);
        mFree(encodedPayload);
    }
    
    // Keep only the fractional part
    *(state.NUM) = *(state.NUM) - (int)(*(state.NUM));
    
    return ret;
}

clearpacketarray* handleInCoded(encodedpacket codedPacket, decoderstate state){ // Handle a new incoming encoded packet and return the array of (eventually !) decoded clear packets
    clearpacketarray* ret = malloc(sizeof(clearpacketarray));
    ret->nPackets = 0;
    ret->packets = 0;
    
    printf("Received packet : \n");
    encodedPacketPrint(codedPacket);
    // Add the currently received packet in the table
    addToInfosTable(state.clearInfosTable, state.nClearPackets, codedPacket);
    printInfosTable(*(state.clearInfosTable), *(state.nClearPackets));
    
    if(addIfInnovative(state.rrefCoeffs, state.invertedCoeffs, state.codedData, codedPacket, *(state.clearInfosTable), *(state.nClearPackets))){ // Then on the newly received packet
        printf("HiCo: Packet was innovative.\n");
        extractClear(state.rrefCoeffs, state.invertedCoeffs, state.codedData, *(state.clearInfosTable), *(state.nClearPackets), state.lastByteSent, ret);
        
        // Because it was innovative, add it to the buffer
        encodedArrayAppend(state.buffer, encodedPacketCopy(codedPacket));
    } else {
        printf("HiCo: Packet not innovative ; drop\n");
    }
    
    printf("HiCo: lastByteSent = %u\n", state.lastByteSent);
    //printf("Matrices states after adding all known packets.\nrref :\n");
    //mPrint(*state.rrefCoeffs);
    //printf("inv :\n");
    //mPrint(*state.invertedCoeffs);
    //printInfosTable(*(state.clearInfosTable), *(state.nClearPackets));
    //printf("codedData :\n");
    //mPrint(*state.codedData);
    return ret;
}

decoderstate* decoderStateInit(){
    decoderstate* ret = malloc(sizeof(decoderstate));
    
    ret->buffer = malloc(sizeof(encodedpacketarray));
    ret->buffer->nPackets = 0;
    ret->buffer->packets = 0;
    
    ret->lastByteSent = 0;
    
    // Initialize matrices
    ret->rrefCoeffs = mCreate(0,0);
    ret->invertedCoeffs = mCreate(0,0);
    ret->codedData = mCreate(0,0);
    ret->clearInfosTable = malloc(sizeof(clearinfos*));
    *(ret->clearInfosTable) = 0;
    
    ret->nClearPackets = malloc(sizeof(int));
    *(ret->nClearPackets) = 0;
    
    return ret;
}

void decoderStateFree(decoderstate* state){
    encodedArrayFree(state->buffer);
    mFree(state->rrefCoeffs);
    mFree(state->invertedCoeffs);
    mFree(state->codedData);
    
    free(*(state->clearInfosTable));
    free(state->clearInfosTable);
    
    free(state->nClearPackets);
    
    free(state);
}

encoderstate* encoderStateInit(){
    encoderstate* ret = malloc(sizeof(encoderstate));
    ret->NUM = malloc(sizeof(float));
    *(ret->NUM) = 0.0;
    ret->buffer = malloc(sizeof(clearpacketarray));
    ret->buffer->nPackets = 0;
    ret->buffer->packets = 0;
    ret->lastByteAcked = 0;
    
    return ret;
}

void encoderStateFree(encoderstate* state){
    free(state->NUM);
    clearArrayFree(state->buffer);
    free(state);
}
