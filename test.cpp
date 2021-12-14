extern "C" {
	#include <peer.h>
}

#include <blockchain.h>
#include <block.h>
#include <sha256.h>
#include <hashcash.h>
#include <iproofofwork.h>

#include "lock_vector.h"

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
	struct peer_address peer_address;
};

struct BlockVote {
	struct BlockchainMessageHeader bmh;
	bool agree;
	size_t hash_size;
};

struct BlockchainIndexHeader {
	struct BlockchainMessageHeader bmh;
	size_t index;
};

struct BlockchainAdditionRequest {
	struct BlockchainMessageHeader bmh;
	size_t pow_size;
	//std::string pow_solution;
};

template <typename ID>
struct Transaction {
	ID sender;
	ID receiver;
	unsigned int amount;
};


struct BlockData {
	size_t hash_size;
	size_t prev_hash_size;
	size_t data_size;
};

template <typename T>
size_t DataFromBlock(const Block<T, std::string>* b, void* & retval){
	struct BlockData bd;
	memset(&bd, 0, sizeof(bd));
	std::string hash = b->GetHash();
	std::string prev_hash = b->GetPrevHash();

	bd.hash_size = hash.size();
	bd.prev_hash_size = prev_hash.size();
	bd.data_size = sizeof(T);

	void * block_data = b->GetData();

	size_t revtal_size = sizeof(bd) + bd.hash_size + bd.prev_hash_size + bd.data_size;
	retval = malloc(revtal_size);
	memset(retval, 0, revtal_size);

	memcpy(((char*)retval), &bd, sizeof(bd));
	memcpy(((char*)retval) + sizeof(bd), hash.c_str(), bd.hash_size);
	memcpy(((char*)retval) + sizeof(bd) + bd.hash_size, prev_hash.c_str(), bd.prev_hash_size);
	memcpy(((char*)retval) + sizeof(bd) + bd.hash_size + bd.prev_hash_size, block_data, bd.data_size);

	//return retval;
	return revtal_size;
}

template <typename T>
void BlockFromData(Block<T, std::string>* & b, void * data){
	struct BlockData* bd = (struct BlockData*) data;

	std::string hash = std::string((char*)data + sizeof(struct BlockData), bd->hash_size);
	std::string prev_hash = std::string((char*)data + sizeof(struct BlockData) + bd->hash_size, bd->prev_hash_size);
	void * block_data = (char*)data + sizeof(BlockData) + bd->hash_size + bd->prev_hash_size;

	b->SetHash(hash);
	b->SetPrevHash(prev_hash);
	b->SetData((T*) block_data);
}




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
	//std::cout << data_str << std::endl;
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

struct inbox_thread_args {
	LockVector<std::tuple<void*, size_t> >* inbox;
	struct peer* peer;


};


class Application {
private:
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> * bc;
	struct peer peer;
	IProofOfWork<std::string, std::string> * pow_group;

	LockVector<std::tuple<void*, size_t> > inbox = LockVector<std::tuple<void*, size_t> >();
	//std::queue<std::tuple<void*, size_t> > inbox = std::queue<std::tuple<void*, size_t> >();
	//std::vector<struct BlockVote*> votes = std::vector<struct BlockVote*>();
	pthread_t inbox_thread_id; 

	void* inbox_thread(void * args) {
		sockaddr_in clntaddr;
		void * msg;
		size_t msg_len;

		LockVector<std::tuple<void*, size_t> >* inbox = ((struct inbox_thread_args*) args)->inbox;
		struct peer * peer = ((struct inbox_thread_args*) args)->peer;

		while (1) {
			receive(peer, &msg, &msg_len, &clntaddr);

			if (msg) {
				if (inbox->Lock()) {
					inbox->vec.push(std::make_tuple(msg, msg_len));
					inbox->Unlock();
				}	
			} else {
				sleep(1);
			}
			
		}

	}


public:

	Application (char* tracker_addr, char* tracker_port, char* addr, char* port) {
		bc = new Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>(SHA256FromDataAndHash);
		init_peer(&peer, tracker_addr, tracker_port, addr, port);
		pow_group = new HashCash(24);

		struct inbox_thread_args it_args;
		it_args.inbox = &inbox;
		it_args.peer = &peer;

		pthread_create(&inbox_thread_id, NULL, &it_args, &inbox);
	}

	~Application () {
		delete bc;
		delete pow_group;
		exit_peer(&peer);
		pthread_cancel(inbox_thread_id);
		pthread_join(inbox_thread_id, NULL);
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
				++it;
				for (; it != messages.end(); ++it){
					if (it->second.second > largest){
						largest_block = it->second.first;
						second_largest = largest;
						largest = it->second.second;
					} else if (it->second.second > second_largest){
						second_largest = it->second.second;
					}
				}
				//std::cout << largest << " " << second_largest << " " << n_peers  << " " << i << std::endl;

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

	bool client_found(const struct peer_address* clnt_pa, std::vector<struct peer_address>& clients_seen) {
		std::vector<struct peer_address>::iterator it;
		std::cout << clnt_pa->family << " " << clnt_pa->port << " " << clnt_pa->addr << std::endl;
		for (it = clients_seen.begin(); it != clients_seen.end(); ++it){
			if (cmp_peer_address(clnt_pa, &(*(it)))) {
				return true;
			}
		}
		return false;
	}

	bool RequestBlockchainBlock(struct peer* p, Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* & largest_block) {

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> > messages; // Hash -> Block, Count (Block)
		std::vector<struct peer_address> clients_seen = std::vector<struct peer_address>();
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >::iterator it;
		size_t no_block = 0;

		struct BlockchainIndexHeader bih;
		memset(&bih, 0, sizeof(bih));
		bih.bmh.type = REQUESTBLOCK;
		bih.bmh.peer_address = p->own_pa;
		bih.index = bc->Size();
		std::cout << "[ RequestBlockchain ] Requesting block " << bih.index << std::endl;

		// print_bytes(&bih, sizeof(bih));
		// Broadcast a request asking for a block at index blockchain.size()
		//std::cout << peer << std::endl; 

		//print_bytes(&bih, sizeof(bih));
		//sleep(1);

		//std::cout << "Bih: " << &bih << " (" << sizeof(bih) << ")" << std::endl;

		size_t n_peers = broadcast(p, &bih, sizeof(bih));

		// If majority sent LASTBLOCK message, we are done
		void * msg;
		size_t msg_len;
		struct sockaddr_in clntaddr;

		int tries = 0;

		if(n_peers == 0) {
			std::cout << "[ RequestBlockchain ] No peers " << std::endl;
			largest_block = NULL;
			return true; // Nothing to be done
		}

		while(clients_seen.size() < n_peers){
			//std::cout << "begin while" << std::endl;
			

			receive(p, &msg, &msg_len, &clntaddr);



			std::cout << "[ RequestBlockchain ] " << msg << std::endl;

			if (msg) {
				//std::cout << clntaddr.sin_family << " " << clntaddr.sin_port << " " << clntaddr.sin_addr.s_addr << std::endl;
				//b = (Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*)((char*)msg + sizeof(struct BlockchainIndexHeader));
				std::cout << "[ RequestBlockchain ] Handling block..." << std::endl;
				if (((struct BlockchainMessageHeader*) msg)->type == NOBLOCK) {
					if (!client_found(&((struct BlockchainIndexHeader*)msg)->bmh.peer_address, clients_seen)){
						std::cout << "[ RequestBlockchain ] Received NOBLOCK" << std::endl;
						++no_block;
						clients_seen.push_back(((struct BlockchainIndexHeader*)msg)->bmh.peer_address);
					} else {
						std::cout << "[ RequestBlockchain ] Client that sent NOBLOCK already seen... " << std::endl;
					}
				} else if (((struct BlockchainMessageHeader*) msg)->type == BLOCK) {
					void * block_data = ((char*)msg + sizeof(struct BlockchainIndexHeader));
					b = new Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>;
					BlockFromData(b, block_data);
					// Check hash and prev_hash for the block

					if (!client_found(&((struct BlockchainIndexHeader*)msg)->bmh.peer_address, clients_seen)){
						clients_seen.push_back(((struct BlockchainIndexHeader*)msg)->bmh.peer_address);
						std::cout << "[ RequestBlockchain ] Received BLOCK " << std::endl;
						if (SHA256FromDataAndHash(*(b->GetData()), b->GetPrevHash()) == b->GetHash() && b->GetPrevHash() == bc->GetTopHash()) {

							if (messages.find(b->GetHash()) != messages.end()) { // If hash is known
								std::cout << "[ RequestBlockchain ] known BLOCK" << std::endl;
								++messages[b->GetHash()].second;
								free(msg);
							} else {
								std::cout << "[ RequestBlockchain ] BLOCK is new" << std::endl;
								messages.insert(std::make_pair(b->GetHash(), std::make_pair(b, 1)));
							}
						} else {
							std::cout << "[ RequestBlockchain ] Malformed BLOCK" << std::endl;
							free(msg);
						}
					} else {
						std::cout << "[ RequestBlockchain ] Client that sent BLOCK already seen... " << std::endl;
					}
				} else {
					// Add all messages that are not of type BLOCK or NOBLOCK to queue  
					std::cout << "[ RequestBlockchain ] Not NOBLOCK nor BLOCK" << std::endl;
					inbox.push(std::make_tuple(msg, msg_len));
				}


				if(checkMajority(largest_block, clients_seen.size(), n_peers, no_block, messages)) {
					// If largest_block == null, there are no more blocks
					// Add the block with the most votes
					for(it = messages.begin(); it != messages.end(); ++it) {
						if (it->second.first != largest_block) {
							delete it->second.first;
						}
					}
					std::cout << "[ RequestBlockchain ] Majority found" << std::endl;
					return true;
				}
			} else {
				sleep(1);
				++tries;
			}

			// If not all peers responded, maybe ask again?
			if (tries >= 10) {
				for(it = messages.begin(); it != messages.end(); ++it) {
					delete it->second.first;
				}
				std::cout << "[ RequestBlockchain ] Ran out of tries " << std::endl;
				return false;
			}
		}

		for(it = messages.begin(); it != messages.end(); ++it) {
			delete it->second.first;
		}

		std::cout << "[ RequestBlockchain ] No majority " << std::endl;
		return false; // No majority (probably something like a 50/50 vote)
	}

	void RequestBlockchain (struct peer* p) {
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;

		do {
			while(!RequestBlockchainBlock(p, b)) {
				//std::cout << "err" << std::endl;
			}
			//std::cout << b << std::endl;
			if (b) {
				bc->AddBlock(b->GetData()); 
				delete b;
			}
		} while (b != NULL);
	}


	void SendBlockchain(struct peer* p, struct BlockchainIndexHeader& bih) {
		void * response;
		size_t response_len;
		struct BlockchainIndexHeader response_bih;
		memset(&response_bih, 0, sizeof(response_bih));
		response_bih.index = bih.index;
		response_bih.bmh.peer_address = p->own_pa;

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b = bc->GetBlockFromIndex(bih.index);
		// If the block exists
		if (b) {
			std::cout << "[ SendBlockchain ] Block is available " << std::endl;
			// Send the nth block to the requester

			void* block_data;
			size_t block_size = DataFromBlock(b, block_data);

			response_bih.bmh.type = BLOCK;
			
			response_len = sizeof(bih) + block_size;
			response = malloc(response_len);
			memcpy(response, &response_bih, sizeof(response_bih));
			memcpy((char *)response + sizeof(response_bih), block_data, block_size);

			respond(&peer, response, response_len, &bih.bmh.peer_address);
			std::cout << "[ SendBlockchain ] BLOCK sent! " << std::endl;
			free(block_data);
			free(response);
		} else {
			std::cout << "[ SendBlockchain ] Block does not exist " << std::endl;
			//std::cout << clntaddr->sin_family << " " << clntaddr->sin_port << " " << clntaddr->sin_addr.s_addr << std::endl;
			response_bih.bmh.type = NOBLOCK;

			//sockaddr_in temp = *clntaddr;
			//temp.sin_port = htons(1235);
			//respond(&peer, &response_bih, sizeof(response_bih), &temp);
			respond(&peer, &response_bih, sizeof(response_bih), &bih.bmh.peer_address);
			std::cout << "[ SendBlockchain ] NOBLOCK sent! " << std::endl;
		}
	}

	bool HandleBlockAdditionRequest(struct peer* p, void* request_message, size_t request_len) {
		struct BlockchainAdditionRequest* request_bar = (struct BlockchainAdditionRequest*) request_message;
		//Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * req_block = (Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > *) (((char*)request_message) + sizeof(requestHeader) + requestHeader->pow_size);
		

		

		void * block_data = ((char*)request_message) + sizeof(struct BlockchainAdditionRequest) + request_bar->pow_size;
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * b = new Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>;
		BlockFromData(b, block_data);
		std::string block_hash = b->GetHash();

		struct BlockVote vote;
		memset(&vote, 0, sizeof(struct BlockVote));
		vote.bmh.type = VOTE;
		vote.bmh.peer_address = p->own_pa;
		vote.agree = true;
		vote.hash_size = block_hash.size();

		std::string hash = SHA256FromBlock(b);
		//std::string key = requestHeader->pow_solution;
		std::string key = std::string(((char*)request_message) + sizeof(struct BlockchainAdditionRequest), request_bar->pow_size);

		// Check Previous Hash
		if (b->GetPrevHash() != bc->GetTopHash()) {
			std::cout << "[ HandleBlockAdditionRequest ] Invalid previous hash" << std::endl;
			vote.agree = false;
		}

		// Check Block Hash
		else if (b->GetHash() != SHA256FromDataAndHash(*(b->GetData()), b->GetPrevHash())) {
			std::cout << "[ HandleBlockAdditionRequest ] Invalid block hash" << std::endl;
			vote.agree = false;
		}

		// Check Proof Of Work
		else if (!pow_group->CheckSolution(&hash, &key)) {
			std:: cout << "[ HandleBlockAdditionRequest ] Invalid Proof Of Work" << std::endl;
			vote.agree = false;
		}


		// Broadcast vote to all others
		void * vote_msg = malloc(sizeof(vote) + vote.hash_size);
		memset(vote_msg, 0, sizeof(vote) + vote.hash_size);
		memcpy(vote_msg, &vote, sizeof(vote));
		memcpy(vote_msg + sizeof(vote), block_hash.c_str(), vote.hash_size);
		size_t n_peers = broadcast(&peer, vote_msg, sizeof(vote) + vote.hash_size);
		
		std:: cout << "[ HandleBlockAdditionRequest ] Sent vote " << vote.agree << " to " << n_peers << " peers"<< std::endl;

		free(vote_msg);
		// Receive all votes
		if (CheckVotesForBlock(n_peers, b, vote.agree, 1)) {
			bc->AddBlock(b->GetData()); 
			delete b;
			std:: cout << "[ HandleBlockAdditionRequest ] Block added!" << std::endl;
		} else {
			std:: cout << "[ HandleBlockAdditionRequest ] Block rejected!" << std::endl;
		}

		// If yes, add the block: how?
	}


	bool CheckVotesForBlock(size_t n_peers, Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * req_block, bool own_vote, int other) {
		
		int tries = 0;
		//size_t no_block = 0;
		size_t n_agrees = 0;
		size_t n_disagrees = 0;

		if (own_vote) {
			++n_agrees;
		} else {
			++n_disagrees;
		}

		n_peers += 1;

		if (other) {
			++n_agrees;
		}


		std::vector<struct peer_address> clients_seen = std::vector<struct peer_address>();
		struct sockaddr_in clntaddr;
		void * msg;
		size_t msg_len;

		if (n_peers == 0) {
			std::cout << "[ CheckVotesForBlock ] Only one peer! " << std::endl;
			return true;
		}

		while (clients_seen.size() + 1 + other < n_peers) {
			receive(&peer, &msg, &msg_len, &clntaddr);
			std::cout << "There are also " << votes.size() << " pending votes!" << std::endl;
			std::cout << msg << std::endl;
			if (msg) {
				// if (((struct BlockchainMessageHeader*) msg)->type == NOBLOCK) {
				// 	if (!client_found(&((struct BlockchainIndexHeader*)msg)->bmh.peer_address, clients_seen)){
				// 		++no_block;
				// 	}
				// } else if (((struct BlockchainMessageHeader*) msg)->type == VOTE) {
				
				if (((struct BlockchainMessageHeader*) msg)->type == VOTE) {

					votes.push_back((struct BlockVote*) msg);
					while(votes.size() > 0) {
						std::cout << "[ CheckVotesForBlock ] Handling vote... " << std::endl;
						struct BlockVote* vote = votes.back();
						//if (!client_found(votes.back().first, clients_seen) && votes.back().second->block_hash == req_block->GetHash()) {
						//struct BlockVote* vote = (struct BlockVote*) msg;
						std::string block_hash = std::string((char*)msg + sizeof(struct BlockVote), vote->hash_size);
						if (!client_found(&vote->bmh.peer_address, clients_seen) && block_hash == req_block->GetHash()) {
							vote->agree ? ++n_agrees : ++n_disagrees;
							clients_seen.push_back(vote->bmh.peer_address);
						}
						votes.pop_back();
						free(vote);
					}
				} else {
					// Add all messages that are not of type VOTE or NOBLOCK to queue  
					inbox.push(std::make_tuple(msg, msg_len));
				}

				if (n_agrees > n_peers / 2) {
					std::cout << "[ CheckVotesForBlock ] Majority agrees! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
					return true;
				} else if (n_disagrees > n_peers / 2) {
					std::cout << "[ CheckVotesForBlock ] Majority disagrees! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
					return false;
				}
			} else {
				sleep(1);
				++tries;
			}

			// If not all peers responded, maybe ask again?
			if (tries >= 10) {
				std::cout << "[ CheckVotesForBlock ] Too many tries! Got " << n_agrees << " agrees and " << n_disagrees << " disagrees" << std::endl;
				return false;
			}
		}

		if (n_agrees > n_disagrees + n_peers - (clients_seen.size() + 1 + other)) {
			std::cout << "[ CheckVotesForBlock ] Majority agrees! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
			return true;
		}
		std::cout << "[ CheckVotesForBlock ] No majority! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
		return false;
	}

	void AddBlockToBlockchain (struct peer* p, Transactions<ID_TYPE, MAX_TRANSACTIONS> * data) {
		
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * new_block;
		bc->AddBlock(data); // Blockchain handles the process of hash generation

		new_block = bc->GetTopBlock();
		std::string new_block_hash = SHA256FromBlock(new_block);

		// Compute proof of work
		
		void * new_block_data;
		size_t new_block_size = DataFromBlock(new_block, new_block_data);


		std::string pow = pow_group->SolveProblem(&new_block_hash);


		BlockchainAdditionRequest request_bar;
		memset((void*)&request_bar, 0, sizeof(request_bar));
		request_bar.bmh.type = ADDREQUEST;
		request_bar.bmh.peer_address = p->own_pa;
		request_bar.pow_size = pow.size();
		//std::cout << "POW GROUP!!! >" << pow_group << "<" << std::endl;
		//request_bar.pow_solution = pow_group->SolveProblem(&new_block_hash);
		//std::cout << "POW GROUP!!! >" << pow_group << "<" << std::endl;



		size_t request_len;
		void * request;

		request_len = sizeof(request_bar) + request_bar.pow_size + new_block_size;
		request = malloc(request_len);
		memset(((char*)request), 0, request_len);
		memcpy(((char*)request), &request_bar, sizeof(request_bar));
		memcpy(((char*)request) + sizeof(request_bar), pow.c_str(), request_bar.pow_size);
		memcpy(((char*)request) + sizeof(request_bar) + request_bar.pow_size, new_block_data, new_block_size);
		//std::cout << "pow.size() " << pow.size() << " " << pow << std::endl;
		//memcpy((char *)request + sizeof(request_bar) + request_bar.pow_size, new_block, sizeof(*new_block));

		size_t n_peers = broadcast(p, request, request_len);
		free(request);
		free(new_block_data);

		bool accepted = CheckVotesForBlock(n_peers, new_block, true, 0);

		// If rejected, pop newly added block
		if (!accepted) {
			std::cout << "[ AddBlockToBlockchain ] Not accepted " << std::endl;
			bc->PopBlock();
			return;
		}
		std::cout << "[ AddBlockToBlockchain ] Accepted, blockchain size: " << bc->Size() << std::endl;
		
	}

	void Run() {
		std::tuple<void*, size_t> inbox_item;
		bool transact = true;
		void * msg;
		size_t msg_len;
		sockaddr_in clntaddr;

		// If we were offline
		while(1) {
			if (should_refresh(&peer)) {
				// RequestBlockchain
				std::cout << "[ RequestBlockchain ] --------- Start --------- " << std::endl;
				RequestBlockchain(&peer);
				std::cout << "[ RequestBlockchain ] --------- Stop  --------- " << std::endl;
			} else {
				if(inbox.Lock() && !inbox.empty()) { 
					
					inbox_item = *(inbox.vec.begin());
					inbox.erase(inbox.vec.begin());
					inbox.Unlock();

					msg = std::get<0>(inbox_item);

					switch(((struct BlockchainMessageHeader*)msg)->type) {
							case REQUESTBLOCK:
								std::cout << "[ SendBlockchain ] --------- Start --------- " << std::endl;
								//std::cout << std::get<0>(inbox_item).sin_family << " " << std::get<0>(inbox_item).sin_port << " " << std::get<0>(inbox_item).sin_addr.s_addr << std::endl;
								SendBlockchain(&peer, *((struct BlockchainIndexHeader*)msg)/*, &std::get<0>(inbox_item)*/);
								std::cout << "[ SendBlockchain ] --------- Stop  --------- " << std::endl;
								break;

							case ADDREQUEST:
								std::cout << "[ HandleBlockAdditionRequest ] --------- Start --------- " << std::endl;
								HandleBlockAdditionRequest(&peer, msg, std::get<1>(inbox_item));
								std::cout << "[ HandleBlockAdditionRequest ] --------- Stop  --------- " << std::endl;
								break;
							case VOTE:
								if(inbox.Lock()) {
									inbox.vec.push_back(inbox_item); // Put it back
									inbox.Unlock();
								}

								// We add votes to a buffer, in case other's votes are received before the request
								// due to messages being received out of order 
								//votes.push_back(((struct BlockVote*)std::get<0>(inbox_item)));
								break;
							case BLOCK:
							case NOBLOCK:
							default:
								break;
					}
					
					free(msg);
				} else if (transact) {
					Transactions<ID_TYPE, MAX_TRANSACTIONS> ts; 
					memset(&ts, 0, sizeof(ts));
					std::cout << "[ AddBlockToBlockchain ] --------- Start --------- " << std::endl;
					AddBlockToBlockchain(&peer, &ts);
					std::cout << "[ AddBlockToBlockchain ] --------- Stop --------- " << std::endl;
					transact = false;
				} else {
					sleep(1);
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

	Application* a = new Application(argv[1], argv[2], argv[3], argv[4]);
	sleep(5);
	a->Run();


	/*
	const char * a = "Hell\0o world!";
	std::string str = std::string(a, 13);
	std::cout << str << std::endl;
	*/

	//blockchaintest1();
	delete a;

	return 0;
}
