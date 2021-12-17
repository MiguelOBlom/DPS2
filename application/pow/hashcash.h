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


#endif
