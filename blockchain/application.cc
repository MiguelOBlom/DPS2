#include "blockchain.h"
#include "sha256.h"
#include "hashcash.h"
#include "../p2p/peer.h"

#include <string>
#include <iostream>
#include <vector>

#define DIFFICULTY 3
#define MAX_TRANSACTIONS 5
#define ID_TYPE char

enum BlockchainMessageType {
	BLOCK,
	REQUESTBLOCK,
	NOBLOCK
};

struct BlockchainMessageHeader {
	BlockchainMessageType type;
}

struct BlockchainIndexHeader {
	struct BlockchainMessageHeader bmh;
	size_t index;
}

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


class Application {
private:
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> * bc;
	struct peer peer;

	queue<sockaddr_in, message>...
public:

	Application (char* tracker_addr, char* tracker_port, char* addr, char* port) {
		bc = new Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>(SHA256FromDataAndHash);
		init_peer(&peer, tracker_addr, tracker_port, addr, port);
	}

	~Application () {
		delete bc;
		exit_peer(&peer);
	}

	void ReceiveMessage() {

	}

	void RequestBlockchain() {
		std::vector<void *> messages;
		struct BlockchainIndexHeader bih;
		bih.bmh.type = REQUESTBLOCK;
		bih.index = bc.Size();
		// Broadcast a request asking for a block at index blockchain.size()
		size_t n_peers = broadcast(&peer, &bih, sizeof(bih));

		// If majority sent LASTBLOCK message, we are done
		sleep(5);
		void * msg;
		size_t msg_len;
		struct sockaddr_in clntaddr;
		receive(&peer, &msg, &msg_len, &clntaddr);

		// Add all messages that are not of type BLOCK or NOBLOCK to queue  
		
		while() {
			// Check for which peers we have received this block
			// If not all peers responded, maybe ask again?

			// Check prev_hash for each block
			// Check hash for each block
			// Check how many blocks are equal 
			// 		add block that most peers agreed upon
		}


	}


	void SendBlockchain(const struct BlockchainIndexHeader& bih, const struct sockaddr_in* clntaddr) {
		struct BlockchainIndexHeader response_bih;
		response_bih.index = bih.index;

		size_t response_len;
		void * response;
		// Send the nth block to the requester
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b = bc.GetBlockFromIndex(bih.index);
		if (b) {
			response_bih.bmh.type = BLOCK;
			
			response_len = sizeof(bih) + sizeof(*b);
			response = malloc(response_len);
			memcpy(response, &response_bih, sizeof(response_bih));
			memcpy(response + sizeof(response_bih), b, sizeof(*b));

			respond(&peer, response, response_len, clntaddr);

			free(response);
		} else {
			response_bih.bmh.type = NOBLOCK;

			respond(&peer, &response_bih, sizeof(response_bih), clntaddr);
		}

	}

	void HandleBlockAdditionRequest() {
		// Check Previous Hash
		// Check Block Hash
		// Check Proof Of Work

		// Broadcast vote to all others

		// Receive all votes and count majority

		// If yes, add the block

	}


	void AddBlockToBlockchain () {
		// Compute proof of work
		// Broadcast block addition request

	}

	void Run() {
		bool transact;
		// If we were offline
		if (should_refresh(&peer)) {
			// RequestBlockchain
			RequestBlockchain();
		} else {
			// If randomly add transactions
			if (transact) {
				// AddBlockToBlockchain
				AddBlockToBlockchain();
			} else {
				// Receive messages
				ReceiveMessages(); // Receive all messages for some time

				// Remove all BLOCK and NOBLOCK messages

				// Then handle each other queued block accordingly
				// SendBlockchain
				// HandleBlockAdditionRequest
			}
		}

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