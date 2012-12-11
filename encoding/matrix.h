#ifndef _MATRIX_
#define _MATRIX_
#include <stdint.h>

typedef struct matrix_t {
	uint8_t** data;
	int nRows;
	int nColumns;
} matrix;

#endif
