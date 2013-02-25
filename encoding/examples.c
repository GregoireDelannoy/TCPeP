#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "log_tables.h"
#include "structures.h"

#define true 1==1
#define false 1==0
#define max(a, b) a<b?b:a
#define min(a, b) a<b?a:b

#define PACKET_LENGTH 1
#define CLEAR_PACKETS 20
#define ENCODED_PACKETS 30
#define LOSS 0.2

#define MAX_PRINT 20 // Do not print matrices if horiz dimension exceeds it


uint8_t gadd(uint8_t a, uint8_t b) {
    return a ^ b;
}

uint8_t gsub(uint8_t a, uint8_t b) {
    return a ^ b;
}

uint8_t gmul(uint8_t a, uint8_t b) {
    int result;
    if((a == 0) || (b == 0)){
        result = 0;
    } else {
        result = ltable[a] + ltable[b]; // Note : this is a normal addition, not gadd
        result %= 255;
        /* Get the antilog */
        result = atable[result];
    }
    return result;
}

uint8_t gdiv(uint8_t a, uint8_t b) {
    int result;
    if(a == 0){
        result = 0;
    } else if(b == 0){
        printf("Code tried to divide %x by %x !\n", a, b);
        exit(1);
    } else {
        result = ltable[a] - ltable[b]; // Note : this is a normal addition, not gadd
        result = (result + 255) % 255;
        /* Get the antilog */
        result = atable[result];
        if(result == 0){
            printf("Division returned 0 while a = %x", a);
        }
    }
    return result;
}

uint8_t getRandom(){
    int n = random(); // Returns a number between 0 and RAND_MAX
    return (uint8_t)n; 
}

matrix* mcreate(int rows, int columns){
    int i;
    matrix* resultMatrix;
    resultMatrix = malloc(sizeof(matrix));
    resultMatrix->data = malloc(rows * sizeof(uint8_t**));
    resultMatrix->nRows = rows;
    resultMatrix->nColumns = columns;
    for(i = 0; i < resultMatrix->nRows; i++){
        resultMatrix->data[i] = malloc(resultMatrix->nColumns * sizeof(uint8_t*));
    }
    return resultMatrix;
}

matrix* identity(int rows){ // Returns square identity matrix
    int i, j;
    matrix* resultMatrix = mcreate(rows, rows);
    for(i = 0; i < resultMatrix->nRows; i++){
        for(j = 0; j < resultMatrix->nColumns; j++){
            if(i==j){
                resultMatrix->data[i][j] = 1;
            } else {
                resultMatrix->data[i][j] = 0;
            }
        }
    }
    return resultMatrix;
}

matrix* getRandomMatrix(int rows, int columns){
    int i, j;
    matrix* resultMatrix = mcreate(rows, columns);

    for(i = 0; i < resultMatrix->nRows; i++){
        for(j = 0; j < resultMatrix->nColumns; j++){
            resultMatrix->data[i][j] = getRandom();
        }
    }

    return resultMatrix;
}

void mprint(matrix m){
    int i, j;
    printf("rows = %d, columns = %d\n", m.nRows, m.nColumns);
    if(m.nColumns > MAX_PRINT){
        printf("Yo Matrix so fat I not ev' gonna print it.\n");
        return;
    }

    for(i = 0; i < m.nRows; i++){
        printf("|");
        for(j = 0; j < m.nColumns; j++){
            printf(" %2x ", m.data[i][j]);
        }
        printf("|\n");
    }
}

void mfree(matrix* m){
int i;

    for(i = 0; i < m->nRows; i++){
        free(m->data[i]);
    }
    free(m->data);
}

matrix* mmul(matrix a, matrix b){
    int i, j, k;
    matrix* resultMatrix;
    uint8_t tmp;

    // Check dimension correctness
    if(a.nColumns != b.nRows){
        printf("Error in Matrix dimensions. Cannot continue");
        exit(1);
    }

    resultMatrix = mcreate(a.nRows, b.nColumns);

    for(i = 0; i < resultMatrix->nRows; i++){
        for(j = 0; j < resultMatrix->nColumns; j++){
            tmp = 0;
            for(k = 0; k < a.nColumns; k++){
                tmp = gadd(tmp, gmul(a.data[i][k], b.data[k][j]));
            } 
            resultMatrix->data[i][j] = tmp;
        }
    }
    return resultMatrix;
}

matrix* mcopy(matrix orig){
    int i,j;
    matrix* resultMatrix = mcreate(orig.nRows, orig.nColumns);

    for(i = 0; i < resultMatrix->nRows; i++){
        for(j = 0; j < resultMatrix->nColumns; j++){
            resultMatrix->data[i][j] = orig.data[i][j];
        }
    }

    return resultMatrix;
}

matrix* mgauss(matrix m){
    int h,i, j;
    uint8_t temp;
    matrix* origMatrix = mcopy(m);
    matrix* resultMatrix = identity(m.nRows);

    if(m.nRows != m.nColumns){
        printf("Should give square matrix ! DIE.\n");
        exit(1);
    }

    for(h = 0; h < m.nRows ; h++){
        // Make the first number 1
        temp = origMatrix->data[h][h];
        for(j = 0 ; j < m.nRows; j++){
            origMatrix->data[h][j] = gdiv(origMatrix->data[h][j], temp);
            resultMatrix->data[h][j] = gdiv(resultMatrix->data[h][j], temp);
        }

        // Eliminate others
        for(i = 0; i < m.nRows; i++){
            if(i != h){ // We do not want to eliminate ourselves !
                temp = origMatrix->data[i][h];
                for(j = 0 ; j < m.nRows; j++){
                    origMatrix->data[i][j] = gsub(origMatrix->data[i][j], gmul(temp, origMatrix->data[h][j]));
                    resultMatrix->data[i][j] = gsub(resultMatrix->data[i][j], gmul(temp, resultMatrix->data[h][j]));
                }
            }
        }
    }

    mfree(origMatrix);
    free(origMatrix);
    return resultMatrix;
}

int mEqual(matrix a, matrix b){
    int i,j;
    // Test dimensions
    if((a.nRows != b.nRows) || (a.nColumns != b.nColumns)){
    return false;
    }

    // Test each cell
    for(i = 0; i < a.nRows; i++){
        for(j = 0; j < a.nColumns; j++){
            if(a.data[i][j] != b.data[i][j]){
                return false;
            }
        }
    }

    // No difference has been found, return true.
    return true;
}

void mAppendVector(matrix* m, uint8_t* v){
    // Append a vector v at the bottom of a matrix.
    m->data = realloc(m->data, sizeof(uint8_t*) * (m->nRows + 1));
    m->data[m->nRows] = v;
    m->nRows++; 
}

void packetPrint(encodedpacket packet){
    int i;
    // Print coefficients
    printf("Coeffs  : ");
    for(i = 0; i < packet.nCoeffs ; i++){
        printf("%2x|", packet.coeffs[i]);
    } 
    printf("\n");
    // Print payload
    printf("Payload : ");
    for(i = 0; i < packet.payload.size ; i++){
        printf("%2x|", packet.payload.data[i]);
    } 
    printf("\n");
}

void poolPrint(encodedpacketpool pool){
    int i;
    printf("%d packets in pool : \n", pool.nPackets);
    printf("~~~~~~~~\n");
    for(i = 0; i < pool.nPackets; i++){
        printf("Packet %d : \n", i);
        packetPrint(pool.packets[i]);
    } 
    printf("~~~~~~~~\n");
    printf("Coeffs :\n");
    mprint(*(pool.coeffs));
    printf("~~~~~~~~\n");
    printf("Reduced coefficients :\n");
    mprint(*(pool.rrefCoeffs));
    printf("Inverted coefficients :\n");
    mprint(*(pool.invertedCoeffs));
}

void rowReduce(uint8_t* row, uint8_t factor, int size){
    // Reduce a row st row[i] = row[i] / factor
    int i;
    for(i = 0; i < size ; i++){
        row[i] = gdiv(row[i], factor);
    }
}

void rowMulSub(uint8_t* a, uint8_t* b, uint8_t coeff, int size){
    // a = a - b*c
    int i;
    for(i = 0; i < size ; i++){
        a[i] = gsub(a[i], gmul(coeff, b[i]));
    }
}

int addIfInnovative(encodedpacketpool* pool, encodedpacket packet){
    uint8_t* rrefVector;
    uint8_t* invertedVector;
    int i;
    int nullVector;
    uint8_t factor;
    int returnValue = true;

    if(pool->nPackets == 0){ // First packet HAS to be added (plus, we're doing some mallocs here)
        pool->nPackets = 1;
        pool->packets = malloc(sizeof(encodedpacket));
        pool->packets[0] = packet;  
        pool->coeffs = mcreate(1, packet.nCoeffs);
        pool->invertedCoeffs = mcreate(1, packet.nCoeffs);
        for(i = 0; i<packet.nCoeffs; i++){
            pool->coeffs->data[0][i] = packet.coeffs[i];
            pool->invertedCoeffs->data[0][i] = 0;
        }
        pool->invertedCoeffs->data[0][0] = 1; // First packet
        pool->rrefCoeffs = mcopy(*(pool->coeffs));

        factor = pool->rrefCoeffs->data[0][0];
        rowReduce(pool->rrefCoeffs->data[0],factor , packet.nCoeffs);
        rowReduce(pool->invertedCoeffs->data[0],factor, packet.nCoeffs);
    } else {
        rrefVector = malloc(packet.nCoeffs * sizeof(uint8_t));
        invertedVector = malloc(packet.nCoeffs * sizeof(uint8_t));
        // Packet is innovative iff it can not be reduced to a row of zero with previous coefficients
        for(i=0; i<packet.nCoeffs; i++){ // Fill vectors
            rrefVector[i] = packet.coeffs[i];
            invertedVector[i] = 0;
        }
        invertedVector[min(pool->nPackets, packet.nCoeffs)] = 1;

        for(i=0; i<pool->nPackets; i++){ // Eliminate
            factor = rrefVector[i];
            rowMulSub(rrefVector, pool->rrefCoeffs->data[i], factor, packet.nCoeffs);
            rowMulSub(invertedVector, pool->invertedCoeffs->data[i], factor, packet.nCoeffs);
        }
        nullVector = true;
        for(i=0; i<packet.nCoeffs; i++){
            if(rrefVector[i] != 0x00){
                nullVector = false;
            }
        }
        if(!nullVector && (rrefVector[pool->nPackets] != 0x00)){ // Packet is innovative. Add to the pool
            printf("Packet is innovative ! Let's add it.\n");

            // Reduce rref to 1
            factor = rrefVector[pool->nPackets];
            rowReduce(rrefVector, factor, packet.nCoeffs);
            rowReduce(invertedVector, factor, packet.nCoeffs);

            pool->packets = realloc(pool->packets, sizeof(encodedpacket) * (pool->nPackets + 1));
            pool->packets[pool->nPackets] = packet;
            mAppendVector(pool->coeffs, packet.coeffs);
            mAppendVector(pool->rrefCoeffs, rrefVector);
            mAppendVector(pool->invertedCoeffs, invertedVector);
            pool->nPackets++;
        } else {
            printf("Packet is not innovative. Dropped.\n");
            free(rrefVector);
            free(invertedVector);
            returnValue = false;
        }
    }
    
    return returnValue;
}

clearpacketarray* extractPacket(encodedpacketpool pool){
    // For each packet, try to reduce its coeff to "0..0 1 0.. 0" ; if done, we can decode.
    encodedpacket currentCodedPacket = pool.packets[0];
    uint8_t* tempRrefVector;
    uint8_t* tempInvertedVector;
    uint8_t factor, temp;
    int i, j, k, isReduced;
    clearpacket* currentClearPacket;
    clearpacketarray* clearPackets = malloc(sizeof(clearpacketarray));
    clearPackets->nPackets = 0;
    clearPackets->packets = 0;

    for(i=0; i < pool.nPackets; i++){
        tempRrefVector = malloc(currentCodedPacket.nCoeffs * sizeof(uint8_t));
        tempInvertedVector = malloc(currentCodedPacket.nCoeffs * sizeof(uint8_t));
    
        currentCodedPacket = pool.packets[i];
        for(j=0; j < currentCodedPacket.nCoeffs; j++){
            tempRrefVector[j] = pool.rrefCoeffs->data[i][j];
            tempInvertedVector[j] = pool.invertedCoeffs->data[i][j];
        }
        
        for(j=0; j < pool.nPackets; j++){
            if(i!=j){
                factor = tempRrefVector[j];
                rowMulSub(tempRrefVector, pool.rrefCoeffs->data[j], factor, currentCodedPacket.nCoeffs);
                rowMulSub(tempInvertedVector, pool.invertedCoeffs->data[j], factor, currentCodedPacket.nCoeffs);
            }
        }
        
        // Test if we really have reduced it :
        isReduced = true;
        if(tempRrefVector[i] != 0x01){
            isReduced = false;
        }
        for(j=0; (j < currentCodedPacket.nCoeffs) && isReduced; j++){
            if(i != j){
                if(tempRrefVector[j] != 0x00){
                    isReduced = false;
                }
            }
        }
        
        if(isReduced){
            currentClearPacket = malloc(sizeof(clearpacket));
            currentClearPacket->payload.size = currentCodedPacket.payload.size;
            currentClearPacket->payload.data = malloc(currentClearPacket->payload.size * sizeof(uint8_t));
            
            for(j=0; j < currentClearPacket->payload.size; j++){
                temp = 0x00;
                for(k=0; k < pool.nPackets; k++){
                    temp = gadd(temp, gmul(pool.packets[k].payload.data[j], tempInvertedVector[k]));
                }
                currentClearPacket->payload.data[j] = temp;
            }
            
            // Add it to the array of decoded packets
            clearPackets->packets = realloc(clearPackets->packets, (clearPackets->nPackets + 1) * sizeof(clearpacket));
            clearPackets->packets[clearPackets->nPackets] = *currentClearPacket;
            clearPackets->nPackets++;
            
            // Modify the encoded packet pool according to our last calculations
            free(pool.rrefCoeffs->data[i]);
            free(pool.invertedCoeffs->data[i]);
            pool.rrefCoeffs->data[i] = tempRrefVector;
            pool.invertedCoeffs->data[i] = tempInvertedVector;            
        } else {
            free(tempRrefVector);
            free(tempInvertedVector);
        }
    }
    
    
    
    return clearPackets;
}

int main(int argc, char **argv){
    // Create Encoded packets (sender side)
    // Gen clear packets
    matrix* Ps = getRandomMatrix(CLEAR_PACKETS, PACKET_LENGTH);
    printf("Original Packets : \n");
    mprint(*Ps);

    // Gen Encoded Packets
    encodedpacket** encodedData = malloc(ENCODED_PACKETS * (sizeof (encodedpacket*)));
    int i;
    for(i = 0; i < ENCODED_PACKETS; i++){
        //printf("Generating encoded packet #%d\n", i);
        encodedData[i] = malloc(sizeof(encodedpacket)); 
        encodedData[i]->payload.size = PACKET_LENGTH;
        encodedData[i]->nCoeffs = CLEAR_PACKETS;
        // Generate Coefficients
        matrix* currentCoeffs = getRandomMatrix(1, CLEAR_PACKETS);
        //printf("Coeffs :\n");
        //mprint(*currentCoeffs);
        encodedData[i]->coeffs = currentCoeffs->data[0];
        // Compute encoded packet
        matrix* encodedPacket = mmul(*currentCoeffs, *Ps);  
        //printf("Encoded packet #%d:\n", i);
        //mprint(*encodedPacket);
        encodedData[i]->payload.data = encodedPacket->data[0];
    }

    // Add in pool (receiver side ) ; yeld when found
    encodedpacketpool* pool = malloc(sizeof(encodedpacketpool));
    pool->nPackets = 0;


    clearpacketarray* clearArray;
    int j,k;
    
    for(i = 0; i < ENCODED_PACKETS; i++){
        if(((1.0 * random())/RAND_MAX) < LOSS){
            printf("\nEncoded packet #%d has been lost\n", i);
        } else {
            printf("\nReceived encoded packet #%d\n", i);
            if(addIfInnovative(pool, *(encodedData[i]))){
                printf("Innovative combination ; packet added to the pool :\n");
                //poolPrint(*pool);
                clearArray = extractPacket(*pool);
                if(clearArray->nPackets > 0){
                    printf("%d packets decoded in this round\n", clearArray->nPackets);
                    //for(j=0; j < clearArray->nPackets; j++){
                        //printf("Decoded packet #%d: \n", j);
                        //for(k=0; k < clearArray->packets[j].payload.size; k++){
                            //printf(" %2x ", clearArray->packets[j].payload.data[k]);
                        //}
                        //printf("\n");
                    //}
                }
            }
        }
    }
    
    return 0;
}

