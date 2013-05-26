/* Copyright 2013 Gregoire Delannoy
 * 
 * This file is a part of TCPeP.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "utils.h"
#include "galois_field.h"
#include "matrix.h"
#include "packet.h"
#include "encoding.h"
#include "decoding.h"
#include "protocol.h"


#define CLEAR_PACKETS 1000
#define LOSS 0.1
#define INPUT_LENGTH 3000


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
    matrix* a, *b;
    
    // Create & Destroy Matrices
    a = getRandomMatrix(1000,1000);
    b = getRandomMatrix(1000,1000);
    mFree(a);
    mFree(b);
    
    mFree(mCreate(0,0));
    
    return true;
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
    struct timeval startTime, endTime;
    encoderstate* encState = encoderStateInit();
    decoderstate* decState = decoderStateInit();
    uint8_t inputBuffer[INPUT_LENGTH], buf1[2 * PACKETSIZE], buf2[2 * PACKETSIZE], type;
    int totalBytesSent = 0, totalBytesReceived = 0, totalAckSent = 0, totalAckReceived = 0, totalDataPacketReceived = 0, totalDataPacketSent = 0, nDataPacketSent = 0;
    int i, j, buf1Len, buf2Len;
    muxstate mState;
    mState.sport = 10; mState.dport = 10; mState.remote_ip = 10;
    int nRounds = CLEAR_PACKETS, sendSize;
    float timeElapsed;
    
    matrix* randomMatrix = getRandomMatrix(1, INPUT_LENGTH);
    memcpy(inputBuffer, randomMatrix->data[0], INPUT_LENGTH);
    mFree(randomMatrix);
    
    gettimeofday(&startTime, NULL);
    for(i = 0; i<nRounds; i++){
        //if(regulator()){
            //printf("\n~~~~Starting round %d~~~~~\n", i);
            //encoderStatePrint(*encState);
            //decoderStatePrint(*decState);
            //printf("~~~~~~~~~\n");
        //}

        //sendSize = (int)(((0.8 + 0.2 *random())/RAND_MAX) * INPUT_LENGTH);
        sendSize = PACKETSIZE - 20;
        //printf("Adding %d to the encoder\n", sendSize);
        handleInClear(encState, inputBuffer, sendSize);
        totalBytesReceived += sendSize;

        // Send ACKs
        for(j = 0; j < decState->nAckToSend; j++){
            bufferToMuxed(decState->ackToSend[j], buf1, decState->ackToSendSize[j], &buf1Len, mState, TYPE_ACK);
            muxedToBuffer(buf1, buf2, buf1Len, &buf2Len, &mState, &type);
            totalAckSent++;
            if(((1.0 * random())/RAND_MAX) > LOSS){
                onAck(encState, buf2, buf2Len);
                //printf("Sent an ACK\n");
                totalAckReceived++;
            } else {
                //printf("Lost an ACK\n");
            }
        }
        // Free
        for(j = 0; j< decState->nAckToSend;j++){
            free(decState->ackToSend[j]);
        }
        free(decState->ackToSend);
        decState->ackToSend = 0;
        free(decState->ackToSendSize);
        decState->ackToSendSize = 0;
        decState->nAckToSend = 0;

        // Send coded data packets from the encoder
        for(j = 0; j < encState->nDataToSend; j++){
            bufferToMuxed(encState->dataToSend[j], buf1, encState->dataToSendSize[j], &buf1Len, mState, TYPE_DATA);
            muxedToBuffer(buf1, buf2, buf1Len, &buf2Len, &mState, &type);
            totalDataPacketSent += buf2Len;
            nDataPacketSent++;
            if(((1.0 * random())/RAND_MAX) > LOSS){
                handleInCoded(decState, buf2, buf2Len);
                //printf("Sent a DATA packet from buf1:%d to buf2:%d\n", buf1Len, buf2Len);
                totalDataPacketReceived += buf2Len;
            } else {
                //printf("Lost a data packet\n");
            }
            
        }
        // Free
        for(j = 0; j< encState->nDataToSend;j++){
            free(encState->dataToSend[j]);
        }
        free(encState->dataToSend);
        encState->dataToSend=0;
        free(encState->dataToSendSize);
        encState->dataToSendSize = 0;
        encState->nDataToSend = 0;
        
        if(decState->nDataToSend > 0){
            //printf("Sent %d decoded bytes to the application\n", decState->nDataToSend);
            totalBytesSent += decState->nDataToSend;
            free(decState->dataToSend);
            decState->dataToSend = 0;
            decState->nDataToSend = 0;
        }
        
        if(regulator()){
            printf("\n~~~~End of round %d~~~~~\n", i);
            encoderStatePrint(*encState);
            decoderStatePrint(*decState);
            printf("~~~~~~~~~\n");
        }
        
        //usleep(10000 + (1.0 * random() /RAND_MAX) * 1000);
    }
    gettimeofday(&endTime, NULL);
    timeElapsed = 1.0 * (endTime.tv_sec - startTime.tv_sec) + ((endTime.tv_usec - startTime.tv_usec) / 1000000.0);
    
    encoderStatePrint(*encState);
    decoderStatePrint(*decState);
    
    printf("During the %d rounds and %f s, %d bytes has been received by the encoder ; %d has been sent to the application.\n%d bytes of Data Packets has been sent, %d received.\n%d Ack has been sent, %d received.\n Simulated loss rate = %f %%. Transmission efficiency = %f %%. Transmission speed = %f MB/s. Data packet per Rounds = %f\n", nRounds, timeElapsed, totalBytesReceived, totalBytesSent, totalDataPacketSent, totalDataPacketReceived, totalAckSent, totalAckReceived, LOSS, 1.0 * totalBytesSent / totalDataPacketReceived, totalBytesSent / (1024 * 1024 * timeElapsed), 1.0 * nDataPacketSent / nRounds);
    
    encoderStateFree(encState);
    decoderStateFree(decState);
    return true;
}

int statesTest(){
    muxstate** muxTable = malloc(sizeof(muxstate*));
    *muxTable = 0;
    int tableLength = 0;
    struct sockaddr_in udpRemoteAddr;
    memset(&udpRemoteAddr, 0, sizeof(udpRemoteAddr));
    
    assignMux((uint16_t)random(), (uint16_t)random(), (uint32_t)random(), (uint16_t)random(), 0, muxTable, &tableLength, udpRemoteAddr);
    printMux((*muxTable)[0]);
    removeMux(0, muxTable, &tableLength);
    
    free(muxTable);
    return true;
}

int main(int argc, char **argv){
    if(codingTest()){
    //if(statesTest()){
    //if(galoisTest() && matrixTest() && maxMinTest() && codingTest() && statesTest()){
        printf("All test passed.\n");
        return 0;
    } else {
        printf("Testing failed.\n");
        return -1;
    }
}
