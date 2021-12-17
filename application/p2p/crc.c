// Author: Miguel Blom

#include "crc.h"

// Prints all bits in a byte seperately
void print_bits_of_byte(const unsigned char* c) {
	unsigned char m = 128;
	for (unsigned int i = 0; i < 8; ++i) {
		printf("%u", (*c & m)? 1 : 0);
		m >>= 1;
	}
}

// Prints the data in bits
void print_bits(void * data, size_t len) {
	for (size_t i = len; i > 0; --i) {
		print_bits_of_byte(&(((unsigned char*) data)[i - 1]));
		printf(" ");
	}
}

// Calculate the length of the polynomial
// i.e. index of highest bit
size_t calc_poly_len(struct dpoly dp) {
	size_t poly_len;
	size_t offset = 0;

	POLY_TYPE val = dp.h;

	// Do we have 1's in the upper half?
	if (val == 0) {
		// If not, use lower half
		val = dp.l;
	} else {
		// Otherwise we do not have to 
		// count the length of the lower half
		offset = sizeof(val) * 8;
	}

	// Keep track of the last 1 to get to the length of the polynomial
	for (size_t i = 0; i < sizeof(val) * 8; i++) {
		if (val & 1) {
			poly_len = offset + i + 1;
		}
		val >>= 1;
	}

	return poly_len;
}

// Shift dp right by some amount n
void sl(struct dpoly* dp, size_t n) {
	POLY_TYPE carry;

	for (size_t i = 0; i < n; i++) {
			// Calculate carry to right adjacent byte
			carry = ((dp->l >> (sizeof(dp->l) * 8 - 1)) & 1);
			dp->l <<= 1;
			dp->h <<= 1;
			dp->h |= carry;
	}

}

// Shift dp right by some amount n
void sr(struct dpoly* dp, size_t n) {
	POLY_TYPE carry;
	for (size_t i = 0; i < n; i++) {
		// Calculate carry to right adjacent byte
		carry = (dp->h & 1) << (sizeof(dp->l) * 8 - 1);
		dp->l >>= 1;
		dp->h >>= 1;
		dp->l |= carry;
	}
}

// Compute the crc using polynom dp and data of size data_len 
POLY_TYPE calc_crc(struct dpoly dp, void * data, size_t n_blocks) {
	size_t poly_len = calc_poly_len(dp);
	sl(&dp, sizeof(dp) * 8 - poly_len);

	// For each byte
	for (size_t q = 0; q < n_blocks - 1; ++q) {
		// For each bit in the byte
		for (size_t i = sizeof(dp.l) * 8; i > 0; i--) {
			// If there is a 1 bit at this point in the data
			if (((POLY_TYPE*)data)[q] & 1 << (i - 1)) {
				// Perform the XOR
				((POLY_TYPE*)data)[q + 1] ^= dp.l;
				((POLY_TYPE*)data)[q] ^= dp.h;
			}
			// Shift the polynom by one
			sr(&dp, 1);
		}
		// Restore polynom
		sl(&dp, sizeof(dp.l) * 8);
	}
	
	return ((POLY_TYPE*)data)[n_blocks - 1];
}

POLY_TYPE get_crc(const void * data, size_t data_len) {
	// Get the required number of blocks for this problem
	size_t n_blocks = (data_len + sizeof(POLY_TYPE) - 1) / sizeof(POLY_TYPE) + 1; // We need an extra block for our CRC
	// Create a temporary buffer with the required size
	size_t temp_len = n_blocks * sizeof(POLY_TYPE);
	void * temp = malloc(temp_len);
	
	// Get the data in the buffer
	memset(temp, 0, temp_len);
	memcpy(temp, (POLY_TYPE*) data, data_len);

	// Construct the polynom
	struct dpoly dp;
	dp.h = POLY;
	dp.l = 0;

	// Calculate the CRC, clean up and return
	POLY_TYPE ret = calc_crc(dp, temp, n_blocks);
	free(temp);
	return ret;
}

int check_crc(const void * data, size_t data_len, POLY_TYPE crc) {
	POLY_TYPE res;
	// Append the CRC with the data
	size_t temp_len = data_len + sizeof(crc);
	void * temp = malloc(temp_len);
	memcpy(temp, data, data_len);
	memcpy(temp + data_len, &crc, sizeof(crc));

	// Compute the CRC
	res = get_crc(temp, temp_len);
	free(temp);

	// Return true if zero, otherwise false
	return !res;
}
