#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "utils.h"
#include "galois_field.h"
#include "matrix.h"
#include "packet.h"



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
    
    return isOk;
}

int packetTest(){
    int isOk = true;
    uint8_t data[2] = {0xFF, 0xFF};
    
    clearpacket* cp = clearPacketCreate(10, 2, data);
    encodedpacket* ep = malloc(sizeof(encodedpacket));
    ep->payload = payloadCreate(2, data);
    ep->coeffs = malloc(sizeof(coeffs));    
    
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


int main(int argc, char **argv){    
    if(galoisTest() && matrixTest() && packetTest() && maxMinTest()){
        printf("All test passed.\n");
        return 0;
    } else {
        printf("Testing failed.\n");
        return -1;
    }
}
