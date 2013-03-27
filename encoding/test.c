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


#define PACKET_LENGTH 50
#define CLEAR_PACKETS 10
#define LOSS 0.05


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

int maxMinTest(){
    if((max(1,2) == 2) && (min(2,1) == 1)){
        return true;
    } else {
        printf("Min/Max failed\n");
        return false;
    }
}

int codingTest(){
    encoderstate* encState = encoderStateInit();
    decoderstate* decState = decoderStateInit();
    uint8_t buffer[PACKETSIZE], buf1[2 * PACKETSIZE], buf2[2 * PACKETSIZE], type;
    int totalBytesSent = 0, totalBytesReceived = 0, totalAckSent = 0, totalAckReceived = 0, totalDataPacketReceived = 0, totalDataPacketSent = 0;
    int i, j, buf1Len, buf2Len;
    muxstate mState;
    mState.sport = 10; mState.dport = 10; mState.remote_ip = 10;
    int nRounds = 100;
    
    matrix* randomMatrix = getRandomMatrix(1, PACKETSIZE);
    mPrint(*randomMatrix);
    memcpy(buffer, randomMatrix->data[0], PACKETSIZE);
    mFree(randomMatrix);
    
    for(i = 0; i<nRounds; i++){
        printf("\n~~~~Starting round %d~~~~~\n", i);
        encoderStatePrint(*encState);
        decoderStatePrint(*decState);
        printf("~~~~~~~~~\n");
        
        handleInClear(encState, buffer, PACKETSIZE - 7);
        totalBytesReceived += PACKETSIZE - 7;

        // Send ACKs
        for(j = 0; j < decState->nAckToSend; j++){
            bufferToMuxed(decState->ackToSend[j], buf1, decState->ackToSendSize[j], &buf1Len, mState, TYPE_ACK);
            muxedToBuffer(buf1, buf2, buf1Len, &buf2Len, &mState, &type);
            totalAckSent++;
            if(((1.0 * random())/RAND_MAX) > LOSS){
                onAck(encState, buf2, buf2Len);
                printf("Sent an ACK\n");
                totalAckReceived++;
            } else {
                printf("Lost an ACK\n");
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
            totalDataPacketSent++;
            if(((1.0 * random())/RAND_MAX) > LOSS){
                handleInCoded(decState, buf2, buf2Len);
                printf("Sent a DATA packet\n");
                totalDataPacketReceived++;
            } else {
                printf("Lost a data packet\n");
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
            printf("Sent %d decoded bytes to the application\n", decState->nDataToSend);
            totalBytesSent += decState->nDataToSend;
            free(decState->dataToSend);
            decState->dataToSend = 0;
            decState->nDataToSend = 0;
        }
        
        printf("\n~~~~End of round~~~~~\n");
        encoderStatePrint(*encState);
        decoderStatePrint(*decState);
        printf("~~~~~~~~~\n");
        
        //usleep(100000);
    }
    
    printf("During the %d rounds, %d bytes has been received by the encoder ; %d has been sent to the application.\n%d Data Packets has been sent, %d received ( * %d = %d).\n%d Ack has been sent, %d received.\n Loss rate = %f.\n", nRounds, totalBytesReceived, totalBytesSent, totalDataPacketSent, totalDataPacketReceived, PACKETSIZE, PACKETSIZE * totalDataPacketReceived,totalAckSent, totalAckReceived, LOSS);
    
    encoderStateFree(encState);
    decoderStateFree(decState);
    return true;
}

int main(int argc, char **argv){
    if(galoisTest() && matrixTest() && maxMinTest() && codingTest()){
        printf("All test passed.\n");
        return 0;
    } else {
        printf("Testing failed.\n");
        return -1;
    }
}
