#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "galois_field.h"
#include "matrix.h"

#define true 1==1
#define false 1==0


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

matrix* getIdentityMatrix(int rows){ // Returns square identity matrix
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
    free(m);
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
    matrix* resultMatrix = getIdentityMatrix(m.nRows);

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
