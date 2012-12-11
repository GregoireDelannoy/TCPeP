#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "log_tables.h"
#include "matrix.h"

#define true 1==1
#define false 1==0

#define PACKET_LENGTH 1
#define CLEAR_PACKETS 3
#define ENCODED_PACKETS 5
#define LOSS 0.1

#define MAX_PRINT 20 // Do not print matrices if one dimension exceeds it

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
        printf("Code tried to divide by 0 !");
        exit(1);
    } else {
        result = ltable[a] - ltable[b]; // Note : this is a normal addition, not gadd
        result = (result + 255) % 255;
        /* Get the antilog */
        result = atable[result];
    }
	return result;
}

uint8_t getRandom(){
    int n = random(); // Returns a number between 0 and RAND_MAX
    return (uint8_t)n; // Casting will result in %255
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
    if((m.nRows > MAX_PRINT) || (m.nColumns > MAX_PRINT)){
        return;
    }
    
    for(i = 0; i < m.nRows; i++){
        printf("");
        for(j = 0; j < m.nColumns; j++){
            printf("& %2x ", m.data[i][j]);
        }
        printf("\\\\\n");
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
    // Returns m^-1 or crash if not invertible. Just to test the algo
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
    
    return resultMatrix;
}

void loss(matrix* A, matrix* C){
    int indexOrig, indexNew, i, j;
    
    if(A->nRows != C->nRows){
        printf("Error in loss ; matrices should have the same number of rows");
        exit(1);
    }
    
    matrix* newA = mcreate(A->nRows, A->nColumns);
    matrix* newC = mcreate(C->nRows, C->nColumns);
    
    indexNew = 0;
    for(indexOrig = 0; indexOrig < A->nRows; indexOrig++){
        // "roll the dice"
        if(random() > (LOSS * RAND_MAX)){
            for(j = 0; j < A->nColumns; j++){
                newA->data[indexNew][j] = A->data[indexOrig][j];
            }
            for(j = 0; j < C->nColumns; j++){
                newC->data[indexNew][j] = C->data[indexOrig][j];
            }
            indexNew++;
        }
    }
    
    // Adjust new matrix sizes
    for(i = A->nRows - 1; i > indexNew; i--){
        free(newA->data[i]);
        free(newC->data[i]);
    }
    newA->nRows = indexNew;
    newC->nRows = indexNew;
    
    // Free old matrices
    mfree(A);
    mfree(C);
    
    *A = *newA;
    *C = *newC;
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

int main(int argc, char **argv){
    /* Sender side : Generate clear packets, coefficients, and encode data. */
    matrix* Ps = getRandomMatrix(CLEAR_PACKETS, PACKET_LENGTH);
    printf("Packet Matrix P (sender) :\n");
    mprint(*Ps);
    
    matrix* As = getRandomMatrix(ENCODED_PACKETS, CLEAR_PACKETS);
    printf("\nCoefficient Matrix A (sender) :\n");
    mprint(*As);
    
    matrix* Cs = mmul(*As, *Ps);
    printf("\nEncoded Packets C (sender) :\n");
    mprint(*Cs);
    
    /* Simulate a lossy channel */
    matrix* Ar = mcopy(*As);
    matrix* Cr = mcopy(*Cs);
    loss(Ar, Cr);
    
    /* Receiver side : try to recover information */
    printf("\nCoefficient Matrix A (receiver) :\n");
    mprint(*Ar);
    
    printf("\nEncoded Packets C (receiver) :\n");
    mprint(*Cr);
    
    printf("\nWe received %d encoded packets\n", Ar->nRows);
    if(Ar->nRows >= Ar->nColumns){
         printf("Trying to recover original data\n");
        
        // Forget un-needed packets. That is NOT good. Correct way is to append packets iff they increase rank.
        Ar->nRows = Ar->nColumns;
        Cr->nRows = Ar->nColumns;
        
        printf("\nAfter packet drop :");
        printf("\nCoefficient Matrix A (receiver) :\n");
        mprint(*Ar);
        
        printf("\nEncoded Packets C (receiver) :\n");
        mprint(*Cr);
        
        matrix* ArInv = mgauss(*Ar);
        printf("\nCoefficient Matrix inverted (receiver) :\n");
        mprint(*ArInv);
        
        matrix* Pr = mmul(*ArInv, *Cr);
        printf("\nDecoded packets (receiver) :\n");
        mprint(*Pr);
        
        if(mEqual(*Pr, *Ps)){
            printf("\nSUCCESS : Information has been entirely recovered\n");
        } else {
            printf("ERROR : Decoded information differs from sent informations.\n");
        }
    } else {
        printf("\nERROR : Too much packet loss ; unable to recover data.\n");
    }
    
    return 0;
}

