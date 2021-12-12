#include "blockchain.h"
#include "block.h"
#include "sha256.h"
#include "hashcash.h"
#include "../p2p/peer.h"
#include "iproofofwork.h"

#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <tuple>

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
};

struct BlockchainIndexHeader {
	struct BlockchainMessageHeader bmh;
	size_t index;
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

<<<<<<< Updated upstream
template <typename T>
std::string SHA256FromDataAndHash(const Block<T, std::string>& b)
{
	T* data = b.GetData();
	std::string data_str = std::string((char*)data, sizeof(T));
	delete data;
	std::string full_str = data_str + b.GetHash();
	return sha256(full_str);
}

=======
const int n_tries = 10; // number of tries before trying to request a block again
>>>>>>> Stashed changes

class Application {
private:
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> * bc;
	struct peer peer;
	IProofOfWork<std::string, std::string> * PowGroup;

	std::queue<std::tuple<sockaddr_in, void*, size_t> > inbox = std::queue<std::tuple<sockaddr_in, void*, size_t> >();
public:

	Application (char* tracker_addr, char* tracker_port, char* addr, char* port) {
		bc = new Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>(SHA256FromDataAndHash);
		init_peer(&peer, tracker_addr, tracker_port, addr, port);
		PowGroup = new HashCash(32);
	}

	~Application () {
		delete bc;
		exit_peer(&peer);
	}

	bool checkMajority(Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* & largest_block, const size_t& i, const size_t& n_peers, const size_t& no_block, const std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >& messages){
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >::iterator it;
		size_t largest;
		size_t second_largest;
		size_t temp;

		// There is a potential majority
		if (i >= n_peers/2) {
			largest_block = NULL;
			if (messages.size() > 0) {
				largest = no_block;
				it = messages.begin()
				second_largest = it->second.second;
				if (second_largest == largest) {
					// No consensus
				} else if (second_largest > largest) {
					temp = second_largest;
					second_largest = largest;
					largest = temp;
					largest_block = it->second.first;
				}

				for (; it != messages.end(); ++it){
					if (it->second.second > largest){
						largest_block = it->second.first;
						second_largest = largest;
						largest = it->second.second;
					} else if (it->second.second > second_largest){
						second_largest = it->second.second;
					}
				}

				if (largest > second_largest + (n_peers - i)) { // If the largest has more elements than the sum of the second largest and the unknown part, then we can safely assume we have found all information for a majority vote
					// majority for largest
					// if largest_block == NULL: majority for no_block
					// otherwise for block pointed to
					return true;
				}
			} else {
				//majority vote for NOBLOCK
				largest_block = NULL;
				return true;
			}
		}
		return false;
	}

	bool client_found(const sockaddr_in& clntaddr, const std::vector<sockaddr_in>& clients_seen) {
		std::vector<sockaddr_in> clients_seen::iterator it;
		for (it = clients_seen.begin; it != clients_seen.end(); ++it){
			if (it->sin_family == clntaddr.sin_family && it->sin_port == clntaddr.sin_port &&
				it->sin_addr.s_addr == clntaddr.sin_addr. s_addr) {
				return true;
			}
		}
		return false;
	}

	bool RequestBlockchainBlock(Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* largest_block) {

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> > messages; // Hash -> Block, Count (Block)
		std::vector<sockaddr_in> clients_seen;
		size_t no_block = 0;

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

		int tries;

		for (size_t i = 0; i < n_peers; ++i) {
			tries = 0;

			receive(&peer, &msg, &msg_len, &clntaddr);

			if (msg) {
				b = msg + sizeof(struct BlockchainIndexHeader);

				if (((struct BlockchainMessageHeader*) msg)->type == NOBLOCK) {
					if (!clnt_found(clntaddr, clients_seen)){
						++no_block;
					}
				} else if (((struct BlockchainMessageHeader*) msg)->type == BLOCK) {
					// Check hash and prev_hash for the block
					if (!clnt_found(clntaddr, clients_seen)){
						if (SHA256FromDataAndHash(*(b->GetData(), b->GetPrevHash()) && b->GetPrevHash() == bc.GetTopHash())) {
							if (messages.find(b->GetHash()) != messages.end()) { // If hash is known
								++messages[b->GetHash()].second;
							} else {
								messages.insert(std::make_pair(b->GetHash(), std::make_pair(b, 1)));
							}
						} else {
							free(msg);
						}
					}
				} else {
					// Add all messages that are not of type BLOCK or NOBLOCK to queue  
					inbox.push(std::make_tuple(clntaddr, msg, msg_len));
				}

				if(checkMajority(largest_block, i, n_peers, no_block, messages)) {
					// If largest_block == null, there are no more blocks
					// Add the block with the most votes
					return true;
				}
			} else {
				sleep(1);
				++tries;
			}

			// If not all peers responded, maybe ask again?
			if (tries >= 10) {
				return false;
			}
		}

		return false;
	}

	void RequestBlockchain (){
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;

		do {

		} while (b != NULL);
	}


	void SendBlockchain(const struct BlockchainIndexHeader& bih, const struct sockaddr_in* clntaddr) {
		void * response;
		size_t response_len;
		struct BlockchainIndexHeader response_bih;
		response_bih.index = bih.index;

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b = bc.GetBlockFromIndex(bih.index);
		// If the block exists
		if (b) {
			// Send the nth block to the requester
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

	void HandleBlockAdditionRequest(BlockchainAdditionRequest requestHeader) {
		Block<Transaction<ID_TYPE>, std::string > requested_block; // temporary, change when we know how the block will be received
		std::string hash;
		std::string key;

		// Check Previous Hash
		if (requested_block.GetPrevHash() != bc->GetTopHash()) {
			std::cout << "Invalid previous hash." << std::endl;
			return; // Previous hash invalid.
		}

		// Check Block Hash
		if (requested_block.GetHash() != SHA256FromDataAndHash(requested_block)) {
			std::cout << "Invalid block hash." << std::endl;
			return; // Block hash invalid
		}

		hash = SHA256FromBlock(requested_block);
		key = requestHeader.pow_solution;

		// Check Proof Of Work
		if (!PowGroup->CheckSolution(&hash, &key)) {
			std:: cout << "POW invalid." << std::endl;
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