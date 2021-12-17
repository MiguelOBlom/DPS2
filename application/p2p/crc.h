/*
	The CRC is calculated as a checksum for a message in our communication.
	We can compute a CRC over data and check a CRC of corresponding data, 
	using these files.

	Author: Miguel Blom
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "../config.h"

#ifndef _CRC_
#define _CRC_

// Double the size of the POLY_TYPE
// This is necessary for shifting the polynomial by some bits
// While performing the computation, since we would be shifting
// bits out of the POLY_TYPE
struct dpoly {
	POLY_TYPE h; // high bits
	POLY_TYPE l; // low bits
};

// Compute the CRC for our data
POLY_TYPE get_crc(const void * data, size_t data_len);

// Check the CRC for our data
int check_crc(const void * data, size_t data_len, POLY_TYPE crc);

#endif
