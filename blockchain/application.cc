#include "blockchain.h"
#include "sha256.h"
#include <string>
#include <iostream>
#include <vector>

#define DIFFICULTY 3
#define MAX_TRANSACTIONS 5
#define ID_TYPE char
/*
template <class ID>
struct Transaction {
	ID sender;
	ID receiver;
	int amount;
};

bool check_solution(std::string solution) {
	for (int i = 0; i < DIFFICULTY; ++i) {
		if (solution.at(i) != '0') {
			return false;
		}
	}
    return true;
}

template <class T>
std::string simpleHash(T data, std::string hash)
{
	// std::size_t h1 = std::hash<T>{}(data);
	std::size_t h1 = 1;
    std::string h2 = sha256(hash);
	return sha256(std::to_string(h1) + h2);
}


int blockchaintest1() {
    Blockchain<int, std::string> bc(simpleHash);
	// Blockchain<Transaction<ID_TYPE> > bc(simpleHash);
	return 0;
}
*/

int main () {
//	blockchaintest1();
	return 0;
}