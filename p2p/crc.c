#include "crc.h"



void print_bits_of_byte(const unsigned char* c) {
	unsigned char m = 128;
	for (unsigned int i = 0; i < 8; ++i) {
		printf("%u", (*c & m)? 1 : 0);
		m >>= 1;
	}
}

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

	if (val == 0) {
		val = dp.l;
	} else {
		offset = sizeof(val) * 8;
	}

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
POLY_TYPE calc_crc(struct dpoly dp, void * data, size_t data_len, size_t n_blocks) {
	size_t poly_len = calc_poly_len(dp);
	sl(&dp, sizeof(dp) * 8 - poly_len);

	for (size_t q = 0; q < n_blocks - 1; ++q) {
		for (size_t i = sizeof(dp.l) * 8; i > 0; i--) {

			// for (size_t b = n_blocks; b > 0; --b) {
			// 	printf("[%lu] %p: ", b, &((POLY_TYPE*)data)[b - 1]);
			// 	print_bits(&((POLY_TYPE*)data)[b - 1], sizeof(POLY_TYPE));
			// 	printf("\n");
			// }
			// printf("\n");

			if (((POLY_TYPE*)data)[q] & 1 << (i - 1)) {
				((POLY_TYPE*)data)[q + 1] ^= dp.l;
				((POLY_TYPE*)data)[q] ^= dp.h;
			}
			sr(&dp, 1);
		}
		sl(&dp, sizeof(dp.l) * 8);
	}

	// for (size_t b = n_blocks; b > 0; --b) {
	// 	printf("[%lu] %p: ", b, &((POLY_TYPE*)data)[b - 1]);
	// 	print_bits(&((POLY_TYPE*)data)[b - 1], sizeof(POLY_TYPE));
	// 	printf("\n");
	// }
	
	return ((POLY_TYPE*)data)[n_blocks - 1];
}

POLY_TYPE get_crc(const void * data, size_t data_len) {
	size_t n_blocks = (data_len + sizeof(POLY_TYPE) - 1) / sizeof(POLY_TYPE) + 1; // We need an extra block for our CRC
	//printf("Data is size %lu, we have blocks of size %lu, so we need %lu block(s) for the data.\n", data_len, sizeof(POLY_TYPE), n_blocks);

	size_t temp_len = n_blocks * sizeof(POLY_TYPE);
	void * temp = malloc(temp_len);
	
	memset(temp, 0, temp_len);
	memcpy(temp, (POLY_TYPE*) data, data_len);
	//printf("temp is at: %p with size %lu\n", temp, temp_len);

	struct dpoly dp;
	dp.h = POLY;
	dp.l = 0;

	POLY_TYPE ret = calc_crc(dp, temp, temp_len, n_blocks);

	free(temp);

	return ret;
}

int check_crc(const void * data, size_t data_len, POLY_TYPE crc) {
	POLY_TYPE res;
	size_t temp_len = data_len + sizeof(crc);
	void * temp = malloc(temp_len);
	memcpy(temp, data, data_len);
	memcpy(temp + data_len, &crc, sizeof(crc));


	res = get_crc(temp, temp_len);

	free(temp);

	return !res;
}

/*
int main() {
	char * x = "Hi!!!\0";
	size_t len = strlen(x);
	size_t n_blocks = (len + sizeof(POLY_TYPE) - 1) / sizeof(POLY_TYPE) + 1; // We need an extra block for our CRC

	void * data = malloc(sizeof(POLY_TYPE) * n_blocks);
	size_t data_len = n_blocks * sizeof(POLY_TYPE);
	memcpy(data, (POLY_TYPE*) x, (n_blocks - 1) * sizeof(POLY_TYPE));

	struct dpoly dp;
	dp.h = POLY;
	dp.l = 0;

	size_t poly_len = calc_poly_len(dp);

	sl(&dp, sizeof(dp) * 8 - poly_len);
	sr(&dp, sizeof(dp) * 8 - poly_len);

	printf("%u\n", get_crc(dp, data, data_len));

	free(data);
}


uint16_t rotate(struct dpoly dp, void * data, size_t data_len) {
	size_t poly_len = calc_poly_len(dp);
	// printf("size: %lu\n", sizeof(dp) * 8  - poly_len);
	sl(&dp, sizeof(dp) * 8 - poly_len);

	for (size_t q = 0; q < data_len/sizeof(uint16_t); ++q) {
		for (size_t i = sizeof(dp.l) * 8; i > 0; i--) {
			//print_bits((void*) &dp, sizeof(struct dpoly));
			//printf("\n");
			//for (size_t b = data_len/sizeof(uint16_t); b > 0; --b) {
			//	print_bits(&((uint16_t*)data)[b - 1], sizeof(uint16_t));
			//}
			//print_bits(&((uint16_t*)data)[0], sizeof(uint16_t));
			//printf("\n");
			
			if (((uint16_t*)data)[q] & 1 << (i - 1)) {
				//printf("yes\n\n");
				((uint16_t*)data)[q + 1] ^= dp.l;
				((uint16_t*)data)[q] ^= dp.h;
			}// else {
				//printf("no\n\n");
			//}
			
			sr(&dp, 1);
		}
		sl(&dp, sizeof(dp.l) * 8);
	}

	//print_bits((void*) &dp, sizeof(struct dpoly));
	//printf("\n");
	//for (size_t b = data_len/sizeof(uint16_t); b > 0; --b) {
	//	print_bits(&((uint16_t*)data)[b - 1], sizeof(uint16_t));
	//}
	//print_bits(&((uint16_t*)data)[1], sizeof(uint16_t));
	//print_bits(&((uint16_t*)data)[0], sizeof(uint16_t));
	//printf("\n");

	return ((uint16_t*)data)[data_len/sizeof(uint16_t)];
}

int main() {
	char * x = "Hi!!!\0";
	//printf("%s %d\n", x, (int)x[0]);
	size_t len = strlen(x);
	
	
	//print_bits(x, len);
	//printf("\n");

	//printf("Data is size %lu, we have blocks of size %lu, so we need %lu block(s) for the data.\n", len, sizeof(uint16_t), (len + sizeof(uint16_t) - 1) / sizeof(uint16_t));

	size_t n_blocks = (len + sizeof(uint16_t) - 1) / sizeof(uint16_t) + 1; // We need an extra block for our CRC

	void * data = malloc(sizeof(uint16_t) * n_blocks);
	size_t data_len = n_blocks * sizeof(uint16_t);
	memcpy(data, (uint16_t*) x, (n_blocks - 1) * sizeof(uint16_t));

	//printf("data ");
	//print_bits(&((uint16_t*)data)[0], sizeof(uint16_t));
	//print_bits(&((uint16_t*)data)[1], sizeof(uint16_t));
	// printf("\n");
	// printf("data %s\n", (char*) data);

	struct dpoly dp;
	dp.h = POLY;
	dp.l = 0;

	size_t poly_len = calc_poly_len(dp);

	// printf("high bits %u\n", dp.h);
	// printf("low  bits %u\n", dp.l);

	// printf("size: %lu\n", sizeof(dp) * 8  - poly_len);

	sl(&dp, sizeof(dp) * 8 - poly_len);
	// printf("poly (l l h h) ");
	// print_bits((void*) &dp, sizeof(struct dpoly));
	// printf("\n");

	// printf("poly (l l h h) ");
	// print_bits((void*) &dp, sizeof(struct dpoly));
	// printf("\n");

	// printf("high bits %u\n", dp.h);
	// printf("low  bits %u\n", dp.l);

	sr(&dp, sizeof(dp) * 8 - poly_len);
	// printf("poly (l l h h) ");
	// print_bits((void*) &dp, sizeof(struct dpoly));
	// printf("\n");

	// printf("high bits %u\n", dp.h);
	// printf("low  bits %u\n", dp.l);


	rotate(dp, data, data_len);
}
*/
