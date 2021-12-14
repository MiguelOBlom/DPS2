#include "iproofofwork.h"
#include <sha256.h>
#include <iostream>

class HashCash : public IProofOfWork <std::string, std::string> {
	public:
		HashCash(size_t difficulty);
		~HashCash();
		std::string SolveProblem(std::string* seed) override;
		bool CheckSolution(std::string* seed, std::string* solution) override;
	private:
		size_t difficulty;

		bool count_zeroes(std::string hash);
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
	 	if (j % 8 == 0) {
	 		mask = 0x80;
	 	}

 		if ((hash[j/8] & mask) == 0) {
 			//std::cout << hash[j/8] << " at " << (0xFF & (unsigned int) mask) << std::endl;
 			++zeroes;

	 		if (zeroes >= difficulty) {
 				return true; // We found a solution!
 			}
 			
 			mask >>= 1;
 		} else {
 			//std::cout << "breaking!" << std::endl;
 			break;
 		}
 	}
 	return false;
}

std::string HashCash::xor_strings(std::string s1, std::string s2) {
	for(size_t j = 0; j < s1.size(); j++) {
		s1[j] = s1[j] ^ s2[j];
	}
	return s1;
}

std::string HashCash::SolveProblem(std::string* seed) {
	std::string key;
	std::string hash;
	key += '\0';


	while (1) {
		// Add char and reset all
		if ((0xFF & (unsigned int) key[key.size() - 1]) == 0xFF) {
			if (key.size() == 16) {
				break; // We were not able to find a solution
			}

			key += '\0';

			for (size_t j = 0; j < key.size(); ++j) {
				key[j] = 0;
			}
		}

		// Add one to the current string and carry over
	 	for (size_t j = 0; j < key.size(); ++j) {
	 		if ((0xFF & (unsigned int) key[j]) == 0xFF) {
	 			key[j] = 0;
	 		} else {
	 			key[j]++;
	 			break;
	 		}
	 	}

	 	hash = xor_strings(sha256(*seed + key), *seed);

		///std::cout << "hash: ";
		///for (size_t j = 0; j < hash.size(); ++j) {
		///	std::cout << (0xFF & (unsigned int) hash[j]) << " ";
		///}
		///std::cout << std::endl;
		
		if (count_zeroes(hash)) {
			return key;
		}


	 	/*
	 	std::cout << "key: ";
		for (size_t j = 0; j < key.size(); ++j) {
			std::cout << (0xFF & (unsigned int) key[j]) << " ";
		}
		std::cout << std::endl;
		*/

	}

	return key;
}

bool HashCash::CheckSolution(std::string* seed, std::string* solution){
	std::string hash = xor_strings(sha256(*seed + *solution), *seed);
	return count_zeroes(hash);
}
