#ifndef _MATRIX_
#define _MATRIX_

#define MAX_PRINT 20 // Do not print matrices if horiz dimension exceeds it

#include "utils.h"
#include "galois_field.h"

typedef struct matrix_t {
    uint8_t** data;
    int nRows;
    int nColumns;
} matrix;


matrix* mCreate(int rows, int columns);

matrix* getIdentityMatrix(int rows);

matrix* getRandomMatrix(int rows, int columns);

void mPrint(matrix m);

void mFree(matrix* m);

matrix* mMul(matrix a, matrix b);

matrix* mCopy(matrix orig);

matrix* mGauss(matrix m);

int mEqual(matrix a, matrix b);

void mAppendVector(matrix* m, uint8_t* v);

void rowReduce(uint8_t* row, uint8_t factor, int size);

void rowMulSub(uint8_t* a, uint8_t* b, uint8_t coeff, int size);

void mGrow(matrix* m, int newColumnSize);

#endif
