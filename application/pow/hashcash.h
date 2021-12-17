/*
	Class HashCash implements the IProofOfWork interface.

	The data is of a generic type and so is the solution 
	the the problem. This is considered for modularity.

	Author: Miguel Blom
*/


#include "iproofofwork.h"
#include <sha256.h>
#include <iostream>

#ifndef _HASHCASH_
#define _HASHCASH_

class HashCash : public IProofOfWork <std::string, std::string> {
	public:
		HashCash(size_t difficulty);
		~HashCash();
		std::string SolveProblem(std::string* seed) override;
		bool CheckSolution(std::string* seed, std::string* solution) override;
	private:
		// Difficulty, i.e. number of leading zeroes required to solve the problem
		size_t difficulty;
		// Count the number of leading zero bits of the string hash and return
		// true if it satisfies the difficulty requirement
		bool count_zeroes(std::string hash);
		// Perform a bitwise xor over the two provided strings of data
		std::string xor_strings(std::string s1, std::string s2);
};

HashCash::HashCash(size_t difficulty) : difficulty(difficulty) {

}

HashCash::~HashCash() {

}

// Count zero bits from beginning of string
bool HashCash::count_zeroes(std::string hash) {
	char mask;
	size_t zeroes;
	
 	zeroes = 0;
 	for (size_t j = 0; j < hash.size() * 8; j++) {
 		// Used to seperate the bits
	 	if (j % 8 == 0) {
	 		mask = 0x80; // Byte starting with a single leading 1 bit 
	 					 // and the rest is all zeroes
	 	}

	 	// If the current bit is a zero
 		if ((hash[j/8] & mask) == 0) {
 			// We found another zero
 			++zeroes;

 			// Check requirement
	 		if (zeroes >= difficulty) {
 				return true; // We found a solution!
 			}
 			
 			// Shift the bit to mask the next bit
 			mask >>= 1;
 		} else {
 			break;
 		}
 	}
 	return false;
}

std::string HashCash::xor_strings(std::string s1, std::string s2) {
	// For each character in the string, perform a xor
	for(size_t j = 0; j < s1.size(); j++) {
		s1[j] = s1[j] ^ s2[j];
	}
	return s1;
}

std::string HashCash::SolveProblem(std::string* seed) {
	std::string key;
	std::string hash;
	// Start with a single 0 character
	key += '\0';


	while (1) {
		// If we reached the last byte and it is maxed out
		if ((0xFF & (unsigned int) key[key.size() - 1]) == 0xFF) {
			// Add another char and reset all others to zero
			if (key.size() == 16) { // If our hash would outgrow the used hash size
				// Should be infeasable get to this point
				break; // We were not able to find a solution
			}

			key += '\0';

			for (size_t j = 0; j < key.size(); ++j) {
				key[j] = 0;
			}
		}

		// Increment the character's value and carry over
	 	for (size_t j = 0; j < key.size(); ++j) {
	 		if ((0xFF & (unsigned int) key[j]) == 0xFF) {
	 			key[j] = 0;
	 		} else {
	 			key[j]++;
	 			break;
	 		}
	 	}

	 	// We XOR with the original seed, since SHA256 only seems to provide
	 	// aphanumerical values, thus making leading zeroes impossible
	 	hash = xor_strings(sha256(*seed + key), *seed);
	
		// Check the solution
		if (count_zeroes(hash)) {
			return key;
		}
	}

	return key;
}

bool HashCash::CheckSolution(std::string* seed, std::string* solution){
	// Use the key that was found earlier to check whether the number of
	// leading zeroes checks out.
	std::string hash = xor_strings(sha256(*seed + *solution), *seed);
	return count_zeroes(hash);
}

#endif
