#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// Polynomial type
#define POLY_TYPE uint16_t

#ifndef _CRC_
#define _CRC_

// Polynomial
const POLY_TYPE POLY;

// Double the size of the POLY_TYPE
struct dpoly {
	POLY_TYPE h;
	POLY_TYPE l;
};

POLY_TYPE get_crc(const void * data, size_t data_len);
int check_crc(const void * data, size_t data_len, POLY_TYPE crc);

#endif