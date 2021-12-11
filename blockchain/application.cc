#include "blockchain.h"
#include "sha256.h"
#include "hashcash.h"

#include <string>
#include <iostream>
#include <vector>

#define DIFFICULTY 3
#define MAX_TRANSACTIONS 5
#define ID_TYPE char

template <typename ID>
struct Transaction {
	ID sender;
	ID receiver;
	unsigned int amount;
};

template <typename ID, unsigned int N>
struct Transactions {
	struct Transaction<ID> transaction[N];
};

/*
bool check_solution(std::string solution) {
	for (int i = 0; i < DIFFICULTY; ++i) {
		if (solution.at(i) != '0') {
			return false;
		}
	}
    return true;
}
*/


template <typename T>
std::string SHA256FromBlock(const Block<T, std::string>& b)
{
	T* data = b.GetData();
	std::string data_str = std::string((char*)data, sizeof(T));
	delete data;
	std::string full_str = data_str + b.GetHash() + b.GetPrevHash();
	return sha256(full_str);
}

template <typename T>
std::string SHA256FromDataAndHash(T data, std::string hash)
{
	std::string data_str = std::string((char*) &data, sizeof(T));
	std::cout << data_str << std::endl;
	std::string full_str = data_str + hash;
	return sha256(full_str);
}


int blockchaintest1() {
    //Blockchain<int, std::string> bc(simpleHash);
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> bc(SHA256FromDataAndHash);
	HashCash hc(32);
	Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> b = *bc.GetBlockFromIndex(0);
	std::string hash = SHA256FromBlock(b);
	std::string solution = hc.SolveProblem(&hash);
	std::cout << hc.CheckSolution(&hash, &solution) << std::endl;
	return 0;
}


int main () {
	/*
	const char * a = "Hell\0o world!";
	std::string str = std::string(a, 13);
	std::cout << str << std::endl;
	*/

	blockchaintest1();
	return 0;
}