/* Copyright 2013 Gregoire Delannoy
 * 
 * This file is a part of TCPep.
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

int mEqual(matrix a, matrix b);

void mAppendVector(matrix* m, uint8_t* v);

void rowReduce(uint8_t* row, uint8_t factor, int size);

void rowMulSub(uint8_t* a, uint8_t* b, uint8_t coeff, int size);
#endif
