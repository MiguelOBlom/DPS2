#include "blockchain.h"
#include "block.h"
#include "sha256.h"
#include "hashcash.h"
#include "../p2p/peer.h"

#include <string>
#include <iostream>
#include <vector>

#define DIFFICULTY 3
#define MAX_TRANSACTIONS 5
#define ID_TYPE char

enum BlockChainMessageType {
	BLOCK,
	REQUESTBLOCK,
	LASTBLOCK
};

struct BlockChainMessageHeader{
	std::string header;
};

struct BlockchainAdditionRequest {
	struct BlockchainMessageHeader bmh;
	std::string pow_solution;
};

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

template <typename T>
std::string SHA256FromDataAndHash(const Block<T, std::string>& b)
{
	T* data = b.GetData();
	std::string data_str = std::string((char*)data, sizeof(T));
	delete data;
	std::string full_str = data_str + b.GetHash();
	return sha256(full_str);
}


class Application {
private:
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> * bc;
	struct peer peer;
public:

	Application (char* tracker_addr, char* tracker_port, char* addr, char* port) {
		bc = new Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>(SHA256FromDataAndHash);
		init_peer(&peer, tracker_addr, tracker_port, addr, port);
	}

	~Application () {
		delete bc;
		exit_peer(&peer);
	}

	void RequestBlockchain() {
		// Broadcast a request asking for a block at index blockchain.size()

		// If majority sent LASTBLOCK message, we are done

		// Check for which peers we have received this block
		// If not all peers responded, maybe ask again?

		// Check prev_hash for each block
		// Check hash for each block
		// Check how many blocks are equal 
		// 		add block that most peers agreed upon

	}


	void SendBlockchain() {
		// Send the nth block to the requester
	}

	void HandleBlockAdditionRequest(BlockchainAdditionRequest request) {
		Block<Transaction<ID_TYPE>, std::string > requestedBlock; // temporary, change when we know how the block will be received

		// Check Previous Hash
		if (requestedBlock.GetPrevHash() != bc->GetTopHash()) {
			std::cout << "Invalid previous hash." << std::endl;
			return; // Previous hash invalid.
		}

		// Check Block Hash
		if (requestedBlock.GetHash() != SHA256FromDataAndHash(requestedBlock)) {
			std::cout << "Invalid block hash." << std::endl;
			return; // Block hash invalid
		}

		// Check Proof Of Work
		if (!CheckSolution(SHA256FromBlock(requestedBlock), ) {
			std:: cout << "POW invalid," << std::endl;
			return;
		}


		// Broadcast vote to all others

		// Receive all votes and count majority

		// If yes, add the block

	}


	void AddBlockToBlockchain () {
		// Compute proof of work
		// Broadcast block addition request

	}

	void Run() {
		// If we were offline
			// RequestBlockchain
		// Else
			// If randomly add transactions
				// AddBlockToBlockchain
			// Else 
				// Receive messages
				// SendBlockchain
				// HandleBlockAdditionRequest


	}

};



int blockchaintest1() {
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