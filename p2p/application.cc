#include "blockchain.h"
#include "block.h"
#include "sha256.h"
#include "hashcash.h"
#include "peer.h"
#include "iproofofwork.h"

#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <tuple>
#include <map>

#define DIFFICULTY 3
#define MAX_TRANSACTIONS 5
#define ID_TYPE char

enum BlockchainMessageType {
	BLOCK,
	REQUESTBLOCK,
	NOBLOCK,
	VOTE,
	ADDREQUEST,
};

struct BlockchainMessageHeader {
	BlockchainMessageType type;
};

struct BlockVote {
	struct BlockchainMessageHeader bmh;
	std::string block_hash;
	bool agree;  
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
std::string SHA256FromBlock(const Block<T, std::string>* b)
{
	T* data = b->GetData();
	std::string data_str = std::string((char*)data, sizeof(T));
	delete data;
	std::string full_str = data_str + b->GetHash() + b->GetPrevHash();
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
std::string SHA256FromDataAndHash(const Block<T, std::string>* b)
{
	T* data = b->GetData();
	std::string data_str = std::string((char*)data, sizeof(T));
	delete data;
	std::string full_str = data_str + b->GetHash();
	return sha256(full_str);
}


const int n_tries = 10; // number of tries before trying to request a block again


class Application {
private:
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> * bc;
	struct peer peer;
	IProofOfWork<std::string, std::string> * POWGroup;

	std::queue<std::tuple<sockaddr_in, void*, size_t> > inbox = std::queue<std::tuple<sockaddr_in, void*, size_t> >();
	std::vector<std::pair<sockaddr_in, struct BlockVote*> > votes;
public:

	Application (char* tracker_addr, char* tracker_port, char* addr, char* port) {
		bc = new Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>(SHA256FromDataAndHash);
		init_peer(&peer, tracker_addr, tracker_port, addr, port);
		POWGroup = new HashCash(32);
	}

	~Application () {
		delete bc;
		exit_peer(&peer);
	}


	bool checkMajority(Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* & largest_block, 
					   const size_t& i, 
					   const size_t& n_peers, 
					   const size_t& no_block, 
					   std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >& messages){
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >::iterator it;
		size_t largest;
		size_t second_largest;
		size_t temp;

		// There is a potential majority
		if (i >= n_peers/2) {
			largest_block = NULL;
			if (messages.size() > 0) {
				largest = no_block;
				it = messages.begin();
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
				// majority vote for NOBLOCK
				largest_block = NULL;
				return true;
			}
		}
		return false;
	}

	bool client_found(const sockaddr_in& clntaddr, std::vector<sockaddr_in>& clients_seen) {
		std::vector<sockaddr_in>::iterator it;
		for (it = clients_seen.begin(); it != clients_seen.end(); ++it){
			if (it->sin_family == clntaddr.sin_family && it->sin_port == clntaddr.sin_port &&
				it->sin_addr.s_addr == clntaddr.sin_addr. s_addr) {
				return true;
			}
		}
		return false;
	}

	bool RequestBlockchainBlock(Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* & largest_block) {

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> > messages; // Hash -> Block, Count (Block)
		std::vector<sockaddr_in> clients_seen = std::vector<sockaddr_in>();
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >::iterator it;
		size_t no_block = 0;

		struct BlockchainIndexHeader bih;
		bih.bmh.type = REQUESTBLOCK;
		bih.index = bc->Size();
		// Broadcast a request asking for a block at index blockchain.size()
		size_t n_peers = broadcast(&peer, &bih, sizeof(bih));

		// If majority sent LASTBLOCK message, we are done
		void * msg;
		size_t msg_len;
		struct sockaddr_in clntaddr;

		int tries;

		while(clients_seen.size() < n_peers){
			tries = 0;

			receive(&peer, &msg, &msg_len, &clntaddr);

			if (msg) {
				b = (Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*)((char*)msg + sizeof(struct BlockchainIndexHeader));

				if (((struct BlockchainMessageHeader*) msg)->type == NOBLOCK) {
					if (!client_found(clntaddr, clients_seen)){
						++no_block;
						clients_seen.push_back(clntaddr);
					}
				} else if (((struct BlockchainMessageHeader*) msg)->type == BLOCK) {
					// Check hash and prev_hash for the block

					if (!client_found(clntaddr, clients_seen)){
						if (SHA256FromDataAndHash(*(b->GetData()), b->GetPrevHash()) == b->GetHash() && b->GetPrevHash() == bc->GetTopHash()) {

							if (messages.find(b->GetHash()) != messages.end()) { // If hash is known
								++messages[b->GetHash()].second;
								free(msg);
							} else {
								messages.insert(std::make_pair(b->GetHash(), std::make_pair(b, 1)));
							}
						} else {
							free(msg);
						}
						clients_seen.push_back(clntaddr);
					}
				} else {
					// Add all messages that are not of type BLOCK or NOBLOCK to queue  
					inbox.push(std::make_tuple(clntaddr, msg, msg_len));
				}


				if(checkMajority(largest_block, clients_seen.size(), n_peers, no_block, messages)) {
					// If largest_block == null, there are no more blocks
					// Add the block with the most votes
					for(it = messages.begin(); it != messages.end(); ++it) {
						free(it->second.first);
					}

					return true;
				}
			} else {
				sleep(1);
				++tries;
			}

			// If not all peers responded, maybe ask again?
			if (tries >= 10) {
				for(it = messages.begin(); it != messages.end(); ++it) {
					free(it->second.first);
				}
				return false;
			}
		}

		for(it = messages.begin(); it != messages.end(); ++it) {
			free(it->second.first);
		}
		return false;
	}

	void RequestBlockchain (){
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;

		do {
			while(!RequestBlockchainBlock(b));
			std::cout << b << std::endl;
		} while (b != NULL);
	}


	void SendBlockchain(const struct BlockchainIndexHeader& bih, const struct sockaddr_in* clntaddr) {
		void * response;
		size_t response_len;
		struct BlockchainIndexHeader response_bih;
		response_bih.index = bih.index;

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b = bc->GetBlockFromIndex(bih.index);
		// If the block exists
		if (b) {
			// Send the nth block to the requester
			response_bih.bmh.type = BLOCK;
			
			response_len = sizeof(bih) + sizeof(*b);
			response = malloc(response_len);
			memcpy(response, &response_bih, sizeof(response_bih));
			memcpy((char *)response + sizeof(response_bih), b, sizeof(*b));

			respond(&peer, response, response_len, clntaddr);

			free(response);
		} else {
			response_bih.bmh.type = NOBLOCK;
			respond(&peer, &response_bih, sizeof(response_bih), clntaddr);
		}
	}

	bool HandleBlockAdditionRequest(void* request_message, size_t request_len) {
		struct BlockchainAdditionRequest* requestHeader = (struct BlockchainAdditionRequest*) request_message;
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * req_block = (Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > *) (((char*)request_message) + sizeof(requestHeader));
		BlockVote vote;
		vote.bmh.type = VOTE;
		vote.block_hash = req_block->GetHash();
		vote.agree = false;
		void * response;

		std::string hash = SHA256FromBlock(req_block);
		std::string key = requestHeader->pow_solution;

		// Check Previous Hash
		if (req_block->GetPrevHash() != bc->GetTopHash()) {
			std::cout << "Invalid previous hash." << std::endl;
			vote.agree = false;
		}

		// Check Block Hash
		else if (req_block->GetHash() != SHA256FromDataAndHash(req_block->GetData(), req_block->GetHash())) {
			std::cout << "Invalid block hash." << std::endl;
			vote.agree = false;
		}

		// Check Proof Of Work
		else if (!POWGroup->CheckSolution(&hash, &key)) {
			std:: cout << "POW invalid." << std::endl;
			vote.agree = false;
		}


		// Broadcast vote to all others
		size_t n_peers = broadcast(&peer, &vote, sizeof(vote));
		
		// Receive all votes
		return CheckVotesForBlock(n_peers, req_block);

		// If yes, add the block: how?

	}


	bool CheckVotesForBlock(size_t n_peers, Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * req_block) {
		int tries = 0;
		size_t no_block = 0;
		size_t n_agrees = 0;
		size_t n_disagrees = 0;
		std::vector<sockaddr_in> clients_seen;
		struct sockaddr_in clntaddr;
		void * msg;
		size_t msg_len;

		while (clients_seen.size() < n_peers) {
			receive(&peer, &msg, &msg_len, &clntaddr);
			if (msg) {
				if (((struct BlockchainMessageHeader*) msg)->type == NOBLOCK) {
					if (!client_found(clntaddr, clients_seen)){
						++no_block;
					}
				} else if (((struct BlockchainMessageHeader*) msg)->type == VOTE) {
					votes.push_back(std::make_pair(clntaddr, (struct BlockVote*) msg));
					while(votes.size() > 0) {
						if (!client_found(votes.back().first, clients_seen) && votes.back().second->block_hash == req_block->GetHash()) {
							votes.back().second->agree ? ++n_agrees : ++n_disagrees;
							clients_seen.push_back(votes.back().first);
						}
						votes.pop_back();
					}
				} else {
					// Add all messages that are not of type VOTE or NOBLOCK to queue  
					inbox.push(std::make_tuple(clntaddr, msg, msg_len));
				}

				if (n_agrees > n_peers / 2) {
					return true;
				} else if (n_disagrees > n_peers / 2) {
					return false;
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
	}

	void AddBlockToBlockchain (Transactions<ID_TYPE, MAX_TRANSACTIONS> * data) {
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * new_block;
		bc->AddBlock(data); // Blockchain handles the process of hash generation
		new_block = bc->GetTopBlock();
		std::string new_block_hash = SHA256FromBlock(new_block);

		// Compute proof of work
		BlockchainAdditionRequest request_bar;
		request_bar.bmh.type = ADDREQUEST;
		request_bar.pow_solution = POWGroup->SolveProblem(&new_block_hash);
		size_t request_len;
		void * request;

		request_len = sizeof(request_bar) + sizeof(*new_block);
		request = malloc(request_len);
		memcpy(request, &request_bar, sizeof(request_bar));
		memcpy((char *)request + sizeof(request_bar), new_block, sizeof(*new_block));

		size_t n_peers = broadcast(&peer, &request, request_len);
		free(request);

		bool accepted = CheckVotesForBlock(n_peers, new_block);

		// If rejected, pop newly added block
		if (!accepted) {
			bc->PopBlock();
		}

	}

	void Run() {
		bool transact = false;
		void * msg;
		size_t msg_len;
		sockaddr_in clntaddr;

		// If we were offline
		while(1) {
			if (should_refresh(&peer)) {
				// RequestBlockchain
				RequestBlockchain();
			} else {
				// If randomly add transactions
				if (transact) {
					// AddBlockToBlockchain
					//AddBlockToBlockchain();
				} else {
					msg = NULL;
					do {
						receive(&peer, &msg, &msg_len, &clntaddr);
						inbox.push(std::make_tuple(clntaddr, msg, msg_len));
					} while (msg);


					std::tuple<sockaddr_in, void*, size_t> inbox_item;
					

					//for (it = inbox.begin(); it != inbox.end(); ++it) {
					while(!inbox.empty()) {
						msg = std::get<1>(inbox_item);
						switch(((struct BlockchainMessageHeader*)msg)->type) {
								case REQUESTBLOCK:
									SendBlockchain(*((struct BlockchainIndexHeader*)msg), &std::get<0>(inbox_item));
									break;

								case ADDREQUEST:

									break;
								case VOTE:
									votes.push_back(std::make_pair(std::get<0>(inbox_item), (struct BlockVote*)std::get<1>(inbox_item)));
									break;
								case BLOCK:
								case NOBLOCK:
								default:
									break;
						}
						free(msg);

					}

					// Receive messages
					// ReceiveMessages(); // Receive all messages for some time

					// Remove all BLOCK and NOBLOCK messages

					// Then handle each other queued block accordingly
					// SendBlockchain
					// HandleBlockAdditionRequest
				}

			}
		}

	}

};



int blockchaintest1() {
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> bc(SHA256FromDataAndHash);
	HashCash hc(32);
	Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b = bc.GetBlockFromIndex(0);
	std::string hash = SHA256FromBlock(b);
	std::string solution = hc.SolveProblem(&hash);
	std::cout << hc.CheckSolution(&hash, &solution) << std::endl;
	return 0;
}


int main (int argc, char ** argv) {
	if (argc != 5) {
		printf("Usage: %s <tracker_addr> <tracker_port> <addr> <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	Application a(argv[1], argv[2], argv[3], argv[4]);
	a.Run();


	/*
	const char * a = "Hell\0o world!";
	std::string str = std::string(a, 13);
	std::cout << str << std::endl;
	*/

	blockchaintest1();
	return 0;
}
