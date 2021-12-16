extern "C" {
	#include <peer.h>
}

#include <blockchain.h>
#include <block.h>
#include <sha256.h>
#include <hashcash.h>
#include <iproofofwork.h>

#include "transaction_reader.h"
#include "config.h"
#include "lock_vector.h"
#include "logger.h"

#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <tuple>
#include <map>


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
	size_t index;
};

struct BlockVote {
	struct BlockchainMessageHeader bmh;
	bool agree;
	size_t hash_size;
};

struct BlockchainAdditionRequest {
	struct BlockchainMessageHeader bmh;
	size_t pow_size;
	//std::string pow_solution;
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



void place_back(LockVector<std::tuple<void*, size_t> > & mailbox, std::vector<std::tuple<void*, size_t> >& messages) {
	if (mailbox.Lock()) {
		for (std::vector<std::tuple<void*, size_t> >::iterator it = messages.begin(); it != messages.end(); ++it) {
			mailbox.vec.push_back(*it);
		}

		mailbox.Unlock();
		messages.clear();
	}
	

}


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


void* inbox_thread(void * args) {
	//printf("Inbox thread starting!\n");
	sockaddr_in clntaddr;
	void * msg;
	size_t msg_len;

	LockVector<std::tuple<void*, size_t> >* inbox = ((struct inbox_thread_args*) args)->inbox;
	struct peer * peer = ((struct inbox_thread_args*) args)->peer;
	std::cout << &peer->own_sockfd << std::endl;
	free(args);
	while (1) {
		//printf("In the WHILE LOOP!\n");
		receive(peer, &msg, &msg_len, &clntaddr);

		if (msg) {
			//printf("Something...\n");
			if (inbox->Lock()) {
				inbox->vec.push_back(std::make_tuple(msg, msg_len));
				inbox->Unlock();
			}	
		} else {
			//printf("Nothing...\n");
			sleep(1);
		}
		
	}

}

class Application {
private:
	char * log_filename;
	Logger* log;
	Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string> * bc;
	struct peer * peer;
	IProofOfWork<std::string, std::string> * pow_group;

	LockVector<std::tuple<void*, size_t> > inbox = LockVector<std::tuple<void*, size_t> >();
	//std::queue<std::tuple<void*, size_t> > inbox = std::queue<std::tuple<void*, size_t> >();
	//std::vector<struct BlockVote*> votes = std::vector<struct BlockVote*>();
	pthread_t inbox_thread_id; 

	std::vector<Transactions<ID_TYPE, MAX_TRANSACTIONS> > transactions;

	std::vector<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*> addblock_candidates = std::vector<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*>();

public:

	void initialize() {
		while (!transactions.empty()) {
			size_t block_id = bc->Size();
			log->LogBlockAddedStart(block_id);
			Transactions<ID_TYPE, MAX_TRANSACTIONS> ts = transactions.front(); 
			bc->AddBlock(&ts);
			transactions.erase(transactions.begin());
			log->LogBlockAddedStop(block_id, true);
		}
		
		log->WriteBack(log_filename);
	}


	Application (char* tracker_addr, char* tracker_port, char* addr, char* port, std::vector<Transactions<ID_TYPE, MAX_TRANSACTIONS> > _transactions, char * _log_filename, char init) {
		std::cout << _log_filename << std::endl;
		log = new Logger();
		log_filename = _log_filename;

		transactions = _transactions;

		bc = new Blockchain<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>(SHA256FromDataAndHash);
		
		peer = (struct peer*) malloc(sizeof(struct peer));
		memset(peer, 0, sizeof(struct peer));
		init_peer(peer, tracker_addr, tracker_port, addr, port);
		pow_group = new HashCash(DIFFICULTY);

		if (init == '1') {
			initialize();
		}

		struct inbox_thread_args* it_args = (struct inbox_thread_args*)malloc(sizeof(struct inbox_thread_args));
		it_args->inbox = &inbox;
		it_args->peer = peer;

		std::cout << &peer->own_sockfd << std::endl;
		pthread_create(&inbox_thread_id, NULL, inbox_thread, it_args);
	}

	~Application () {
		delete bc;
		delete pow_group;
		exit_peer(peer);
		free(peer);
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
				second_largest = 0;
				it = messages.begin();
				if (it != messages.end()) {
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

	bool RequestBlockchainBlock(Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* & largest_block) {
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> > messages; // Hash -> Block, Count (Block)
		std::vector<struct peer_address> clients_seen = std::vector<struct peer_address>();
		std::map<std::string, std::pair<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*, size_t> >::iterator it;
		size_t no_block = 0;

		struct BlockchainMessageHeader bmh;
		memset(&bmh, 0, sizeof(bmh));
		bmh.type = REQUESTBLOCK;
		bmh.peer_address = peer->own_pa;
		bmh.index = bc->Size();
		std::cout << "[ RequestBlockchain ] Requesting block " << bmh.index << std::endl;

		// print_bytes(&bih, sizeof(bih));
		// Broadcast a request asking for a block at index blockchain.size()
		//std::cout << peer << std::endl; 

		//print_bytes(&bih, sizeof(bih));
		//sleep(1);

		//std::cout << "Bih: " << &bih << " (" << sizeof(bih) << ")" << std::endl;

		size_t n_peers = broadcast(peer, &bmh, sizeof(bmh));

		// If majority sent LASTBLOCK message, we are done
		void * msg;
		size_t msg_len;
		struct sockaddr_in clntaddr;

		size_t n_requesting = 0;

		int tries = 0;

		if(n_peers == 0) {
			std::cout << "[ RequestBlockchain ] No peers " << std::endl;
			largest_block = NULL;
			return true; // Nothing to be done
		}

		bool once = false;

		std::vector<std::tuple<void*, size_t> > read_messages = std::vector<std::tuple<void*, size_t> >();

		while(clients_seen.size() < n_peers){
			//std::cout << "begin while" << std::endl;
			
			if(inbox.Lock()) {
				if (!inbox.vec.empty()) {
					std::cout << "[ RequestBlockchain ] Message available! " << std::endl;
					std::tuple<void*, size_t> inbox_item = *(inbox.vec.begin());
					inbox.vec.erase(inbox.vec.begin());
					inbox.Unlock();

					msg = std::get<0>(inbox_item);
					msg_len = std::get<1>(inbox_item);
					
					std::cout << "[ RequestBlockchain ] Handling block..." << std::endl;
					if (((struct BlockchainMessageHeader*) msg)->type == NOBLOCK) {
						if (!client_found(&((struct BlockchainMessageHeader*)msg)->peer_address, clients_seen)){
							std::cout << "[ RequestBlockchain ] Received NOBLOCK" << std::endl;
							++no_block;
							clients_seen.push_back(((struct BlockchainMessageHeader*)msg)->peer_address);
						} else {
							std::cout << "[ RequestBlockchain ] Client that sent NOBLOCK already seen... " << std::endl;
						}
						free(msg);
					} else if (((struct BlockchainMessageHeader*) msg)->type == BLOCK) {
						void * block_data = ((char*)msg + sizeof(struct BlockchainMessageHeader));
						b = new Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>;
						BlockFromData(b, block_data);
						// Check hash and prev_hash for the block

						if (!client_found(&((struct BlockchainMessageHeader*)msg)->peer_address, clients_seen)){
							clients_seen.push_back(((struct BlockchainMessageHeader*)msg)->peer_address);
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
								std::cout << "[ RequestBlockchain ] Found " << b->GetHash() << " with count " << messages[b->GetHash()].second << std::endl;
							} else {
								std::cout << "[ RequestBlockchain ] Malformed BLOCK" << std::endl;
								free(msg);
							}
						} else {
							std::cout << "[ RequestBlockchain ] Client that sent BLOCK already seen... " << std::endl;
						}
					} else if (((struct BlockchainMessageHeader*) msg)->type == REQUESTBLOCK){
						if(((struct BlockchainMessageHeader*) msg)->index == bmh.index) {
							if (!client_found(&((struct BlockchainMessageHeader*)msg)->peer_address, clients_seen)){
								std::cout << "[ RequestBlockchain ] Peer is requesting same block" << std::endl;
								//++n_requesting;
								clients_seen.push_back(((struct BlockchainMessageHeader*)msg)->peer_address);
							} else {
								std::cout << "[ RequestBlockchain ] Client requesting same block and already seen... " << std::endl;
							}
							free(msg);
							// if the index of the requested block < our index, send it to the requester
						} else if (((struct BlockchainMessageHeader*) msg)->index < bmh.index) {
							SendBlockchain(*((struct BlockchainMessageHeader*)msg));
							free(msg);
						} else {
							read_messages.push_back(inbox_item);
						}
					} else {
						// Add all messages that are not of type BLOCK or NOBLOCK to queue 
						//if (!once) {
							std::cout << "[ RequestBlockchain ] Not NOBLOCK nor BLOCK nor REQUESTBLOCK" << std::endl;
							if (((struct BlockchainMessageHeader*) msg)->type == VOTE) {
								std::cout << "[ RequestBlockchain ] Is a vote for block " << ((struct BlockchainMessageHeader*) msg)->index << std::endl;
							} else if (((struct BlockchainMessageHeader*) msg)->type == ADDREQUEST) {
								std::cout << "[ RequestBlockchain ] Is an addblock request for block " << ((struct BlockchainMessageHeader*) msg)->index << std::endl;
							}


							//if (inbox.Lock()) {
							//	inbox.vec.push_back(inbox_item);
							//	inbox.Unlock();
							//}
						//	once = true;
						//}
						//Test:
						///////read_messages.push_back(inbox_item);
						free(msg);
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
						place_back(inbox, read_messages);
						return true;
					}

				} else {
					inbox.Unlock();
					sleep(1);
					++tries;
				}

				// If not all peers responded, maybe ask again?
				if (tries >= 10) {
					for(it = messages.begin(); it != messages.end(); ++it) {
						delete it->second.first;
					}
					std::cout << "[ RequestBlockchain ] Ran out of tries " << std::endl;
					place_back(inbox, read_messages);
					return false;
				}
					

				
			}
		}

		std::cout << "[ RequestBlockchain ] No majority " << std::endl;
		place_back(inbox, read_messages);
		return false; // No majority (probably something like a 50/50 vote)

	}

	void RequestBlockchain () {
		size_t block_id;
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b;

		bool success;

		


		do {
			block_id = bc->Size();
			do {
				log->LogRequestBlock(block_id);
				success = RequestBlockchainBlock(b);
				log->LogReceivedBlock(block_id, success);
			} while (!success);

			if (b) {
				bc->AddBlock(b->GetData()); 
				delete b;
			}
		} while (b != NULL);
	}


	void SendBlockchain(struct BlockchainMessageHeader& bmh) {
		void * response;
		size_t response_len;
		struct BlockchainMessageHeader response_bmh;
		memset(&response_bmh, 0, sizeof(response_bmh));
		response_bmh.index = bmh.index;
		response_bmh.peer_address = peer->own_pa;

		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* b = bc->GetBlockFromIndex(bmh.index);
		// If the block exists
		if (b) {
			std::cout << "[ SendBlockchain ] Block " << bmh.index << " is available " << std::endl;
			// Send the nth block to the requester

			void* block_data;
			size_t block_size = DataFromBlock(b, block_data);

			response_bmh.type = BLOCK;
			
			response_len = sizeof(bmh) + block_size;
			response = malloc(response_len);
			memcpy(response, &response_bmh, sizeof(response_bmh));
			memcpy((char *)response + sizeof(response_bmh), block_data, block_size);

			respond(peer, response, response_len, &bmh.peer_address);
			log->LogSendingBlock(bmh.index);
			std::cout << "[ SendBlockchain ] BLOCK sent! " << std::endl;
			free(block_data);
			free(response);
		} else {
			std::cout << "[ SendBlockchain ] Block does not exist " << std::endl;
			//std::cout << clntaddr->sin_family << " " << clntaddr->sin_port << " " << clntaddr->sin_addr.s_addr << std::endl;
			response_bmh.type = NOBLOCK;

			//sockaddr_in temp = *clntaddr;
			//temp.sin_port = htons(1235);
			//respond(&peer, &response_bih, sizeof(response_bih), &temp);
			respond(peer, &response_bmh, sizeof(response_bmh), &bmh.peer_address);
			log->LogSendingBlock(bmh.index);
			std::cout << "[ SendBlockchain ] NOBLOCK sent! " << std::endl;
		}
	}

	bool HandleBlockAdditionRequest(void* request_message, size_t request_len, bool allow_agree) {
		struct BlockchainAdditionRequest* request_bar = (struct BlockchainAdditionRequest*) request_message;
		//Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * req_block = (Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > *) (((char*)request_message) + sizeof(requestHeader) + requestHeader->pow_size);
		
		if (request_bar->bmh.index > bc->Size()) {
			if (inbox.Lock()) {
				inbox.vec.push_back(std::make_tuple(request_message, request_len));
				inbox.Unlock();
			}
			//RequestBlockchain();
			std::cout << "[ HandleBlockAdditionRequest ] BlockchainAdditionRequest too early" << std::endl;
			return false;
		} 
		

		void * block_data = ((char*)request_message) + sizeof(struct BlockchainAdditionRequest) + request_bar->pow_size;
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * b = new Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>;
		BlockFromData(b, block_data);
		std::string block_hash = b->GetHash();

		struct BlockVote vote;
		memset(&vote, 0, sizeof(struct BlockVote));
		vote.bmh.index = request_bar->bmh.index;
		vote.bmh.type = VOTE;
		vote.bmh.peer_address = peer->own_pa;
		vote.agree = allow_agree;
		vote.hash_size = block_hash.size();

		std::cout << "[ HandleBlockAdditionRequest ] Handling BlockchainAdditionRequest " << request_bar->bmh.index << " with hash " << b->GetHash() << std::endl;

		std::string hash = SHA256FromBlock(b);
		//std::string key = requestHeader->pow_solution;
		std::string key = std::string(((char*)request_message) + sizeof(struct BlockchainAdditionRequest), request_bar->pow_size);

		if (request_bar->bmh.index < bc->Size()) {
			std::cout << "[ HandleBlockAdditionRequest ] Outdated BlockchainAdditionRequest" << std::endl;
			vote.agree = false;
		}
		// Check Previous Hash
		else if (b->GetPrevHash() != bc->GetTopHash()) {
			std::cout << "[ HandleBlockAdditionRequest ] Invalid previous hash: have " << bc->GetTopHash() << " but requested has prev hash " << b->GetPrevHash() << std::endl;
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
		size_t n_peers = broadcast(peer, vote_msg, sizeof(vote) + vote.hash_size);
		
		std:: cout << "[ HandleBlockAdditionRequest ] Sent vote " << vote.agree << " to " << n_peers << " peers"<< std::endl;

		free(vote_msg);
		// Receive all votes
		free(request_message);
		if (allow_agree && CheckVotesForBlock(n_peers, b, vote.agree, 1)) {
			addblock_candidates.push_back(b);
			//bc->AddBlock(b->GetData()); 
			//delete b;
			std:: cout << "[ HandleBlockAdditionRequest ] Block added to candidates" << std::endl;
		} else {
			delete b;
			std:: cout << "[ HandleBlockAdditionRequest ] Block rejected!" << std::endl;
		}
		return vote.agree;
	}


	bool CheckVotesForBlock(size_t n_peers, Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * req_block, bool own_vote, int other) {
		/////
		other = 0;
		/////



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

		std::vector<std::tuple<void*, size_t> > read_messages = std::vector<std::tuple<void*, size_t> >();

		std::vector<struct peer_address> clients_seen = std::vector<struct peer_address>();
		struct sockaddr_in clntaddr;
		void * msg;
		size_t msg_len;

		if (n_peers == 0) {
			std::cout << "[ CheckVotesForBlock ] Only one peer! " << std::endl;
			return true;
		}

		// int unread_messages;
		// int expected_inbox_size;

		// if (inbox.Lock()) { 
		// 	expected_inbox_size = unread_messages = inbox.vec.size();
		// 	inbox.Unlock();
		// }


		bool once = false;

		while (clients_seen.size() + 1 + other < n_peers) {
			//receive(&peer, &msg, &msg_len, &clntaddr);

			if (inbox.Lock()) {
				if(!inbox.vec.empty()) {

					std::tuple<void*, size_t> inbox_item = *(inbox.vec.begin());
					inbox.vec.erase(inbox.vec.begin());
					inbox.Unlock();

					msg = std::get<0>(inbox_item);
					msg_len = std::get<1>(inbox_item);

					if (((struct BlockchainMessageHeader*) msg)->type == VOTE && ((struct BlockchainMessageHeader*) msg)->index <= bc->Size()) {
						//votes.push_back((struct BlockVote*) msg);
						//while(votes.size() > 0) {
						std::cout << "[ CheckVotesForBlock ] Got vote for block " << ((struct BlockchainMessageHeader*) msg)->index << std::endl;
						if (((struct BlockchainMessageHeader*) msg)->index == bc->Size()) {
							std::cout << "[ CheckVotesForBlock ] Handling vote " << std::endl;
							struct BlockVote* vote = (struct BlockVote*)msg; //votes.back();
							//if (!client_found(votes.back().first, clients_seen) && votes.back().second->block_hash == req_block->GetHash()) {
							//struct BlockVote* vote = (struct BlockVote*) msg;
							std::string block_hash = std::string((char*)msg + sizeof(struct BlockVote), vote->hash_size);
							if (!client_found(&vote->bmh.peer_address, clients_seen)) {
								if (block_hash == req_block->GetHash()) {
									vote->agree ? ++n_agrees : ++n_disagrees;
									clients_seen.push_back(vote->bmh.peer_address);
									std::cout << "[ CheckVotesForBlock ] Block hash matches!" << std::endl;
								} else {
									std::cout << "[ CheckVotesForBlock ] Block hash does not match" << std::endl;
								}
							} else {
								std::cout << "[ CheckVotesForBlock ] Client already seen" << std::endl;
							}
						} else {
							std::cout << "[ CheckVotesForBlock ] Outdated VOTE" << std::endl;
						}
						//votes.pop_back();
						free(msg);
						//}
					} else {
						// Add all messages that are not of type BLOCK or NOBLOCK to queue 
						//if (!once) {
							// TODO: If a vote is too early, then it would not make sense to add the block, right? It means that the rest of the network
							// is ahead of us 
							if (((struct BlockchainMessageHeader*) msg)->type == VOTE) {
								std::cout << "[ CheckVotesForBlock ] VOTE too early " << ((struct BlockchainMessageHeader*) msg)->index << " while we are ready for block with index " << bc->Size() << std::endl;
								read_messages.push_back(inbox_item);
								//RequestBlockchain();
							} else if (((struct BlockchainMessageHeader*) msg)->type == REQUESTBLOCK) {
								if (((struct BlockchainMessageHeader*) msg)->index <= bc->Size()) {
									SendBlockchain(*((struct BlockchainMessageHeader*)msg));
									free(msg);
								} else {
									read_messages.push_back(inbox_item);
								}
							} else  {
								std::cout << "[ CheckVotesForBlock ] Not a VOTE" << std::endl;
								std::string block_hash;
								void * block_data;
								Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * b;
								struct BlockchainAdditionRequest* request_bar;

								switch(((struct BlockchainMessageHeader*) msg)->type) {
									case ADDREQUEST:
										std::cout << "[ CheckVotesForBlock ] Is an add request" << std::endl;
										if (((struct BlockchainMessageHeader*) msg)->index == bc->Size()) {
											//read_messages.push_back(inbox_item);
											// If the hash of our block is smaller than that of the other block
											// Then reject our own block
											request_bar = (struct BlockchainAdditionRequest*) msg;
											block_data = ((char*)msg) + sizeof(struct BlockchainAdditionRequest) + request_bar->pow_size;
											b = new Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>;
											BlockFromData(b, block_data);
											block_hash = b->GetHash();

											delete b;

											// if the other hash is smaller than our hash, we should decline our block
											if(block_hash.compare(req_block->GetHash()) <= 0) {
												std::cout << "[ CheckVotesForBlock ] Our hash is larger" << std::endl;
												// Make sure we will handle this message next
												// if (inbox.Lock()) {
												// 	inbox.vec.insert(inbox.vec.begin(), inbox_item);
												// }

												if (HandleBlockAdditionRequest(msg, msg_len, true)) {
													return false;
												}

												//read_messages.push_back(inbox_item);
												//return false;
											} else {
												std::cout << "[ CheckVotesForBlock ] Our hash is smaller" << std::endl;
												HandleBlockAdditionRequest(msg, msg_len, false);
												//free(msg);
											}
										} else {
											read_messages.push_back(inbox_item);
										}

									break;
									case BLOCK:
									std::cout << "[ CheckVotesForBlock ] Is a block" << std::endl;
									free(msg);
									break;
									case NOBLOCK:
									std::cout << "[ CheckVotesForBlock ] Is a noblock" << std::endl;
									free(msg);
									break;
									case REQUESTBLOCK:
									std::cout << "[ CheckVotesForBlock ] Is a block request ... HOW?" << std::endl;
									read_messages.push_back(inbox_item);
									break;
									default:
									std::cout << "[ CheckVotesForBlock ] Not recognized" << std::endl;
									free(msg);
									break;
								}


								
							}
						//	once = true;
						//}
						//if (inbox.Lock()) {
							
							
							//inbox.vec.push_back(inbox_item);

							//expected_inbox_size++;
							//inbox.Unlock();
						//}

						
					}

					// We have read the message
					//unread_messages--;

					if (n_agrees > n_peers / 2) {
						std::cout << "[ CheckVotesForBlock ] Majority agrees! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
						place_back(inbox, read_messages);
						return true;
					} else if (n_disagrees > n_peers / 2) {
						std::cout << "[ CheckVotesForBlock ] Majority disagrees! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
						place_back(inbox, read_messages);
						return false;
					}

				} else {
					inbox.Unlock();
					sleep(1);
					++tries;
				}

				// If not all peers responded, maybe ask again?
				if (tries >= n_peers * 10) {
					std::cout << "[ CheckVotesForBlock ] Too many tries! Got " << n_agrees << " agrees and " << n_disagrees << " disagrees" << std::endl;
					place_back(inbox, read_messages);
					return false;
				}

				
			}
		}

		place_back(inbox, read_messages);
		if (n_agrees > n_disagrees + n_peers - (clients_seen.size() + 1 + other)) {
			std::cout << "[ CheckVotesForBlock ] Majority already agrees! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
			return true;
		}
		std::cout << "[ CheckVotesForBlock ] No majority (equal vote, etc.) ! " << n_agrees << " agree vs. " << n_disagrees << " disagree, out of " << n_peers << std::endl;
		return false;
	}

	Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > *  AddBlockToBlockchain (Transactions<ID_TYPE, MAX_TRANSACTIONS> * data) {
		
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * new_block = bc->SneakPeekBlock(data);
		//bc->AddBlock(data); // Blockchain handles the process of hash generation

		//new_block = bc->GetTopBlock();
		std::string new_block_hash = SHA256FromBlock(new_block);

		// Compute proof of work
		
		void * new_block_data;
		size_t new_block_size = DataFromBlock(new_block, new_block_data);


		std::string pow = pow_group->SolveProblem(&new_block_hash);


		BlockchainAdditionRequest request_bar;
		memset((void*)&request_bar, 0, sizeof(request_bar));
		request_bar.bmh.index = bc->Size();
		request_bar.bmh.type = ADDREQUEST;
		request_bar.bmh.peer_address = peer->own_pa;
		request_bar.pow_size = pow.size();
		//std::cout << "POW GROUP!!! >" << pow_group << "<" << std::endl;
		//request_bar.pow_solution = pow_group->SolveProblem(&new_block_hash);
		//std::cout << "POW GROUP!!! >" << pow_group << "<" << std::endl;

		std::cout << "[ AddBlockToBlockchain ] Trying to add block with hash " << new_block->GetHash() << request_bar.bmh.index << std::endl;

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

		size_t n_peers = broadcast(peer, request, request_len);
		free(request);
		free(new_block_data);

		bool accepted = CheckVotesForBlock(n_peers, new_block, true, 0);

		

		// Todo: make sendvote function
		struct BlockVote vote;
		memset(&vote, 0, sizeof(struct BlockVote));
		vote.bmh.index = request_bar.bmh.index;
		vote.bmh.type = VOTE;
		vote.bmh.peer_address = peer->own_pa;
		vote.agree = accepted;
		vote.hash_size = new_block->GetHash().size();
		// Broadcast vote to all others
		void * vote_msg = malloc(sizeof(vote) + vote.hash_size);
		memset(vote_msg, 0, sizeof(vote) + vote.hash_size);
		memcpy(vote_msg, &vote, sizeof(vote));
		memcpy(vote_msg + sizeof(vote), new_block->GetHash().c_str(), vote.hash_size);
		//size_t n_peers = broadcast(peer, vote_msg, sizeof(vote) + vote.hash_size);
		broadcast(peer, vote_msg, sizeof(vote) + vote.hash_size);

		

		// If rejected, pop newly added block
		if (!accepted) {
			std::cout << "[ AddBlockToBlockchain ] Not accepted " << std::endl;
			//bc->PopBlock();
			delete new_block;
			return NULL;
		}

		addblock_candidates.push_back(new_block);
		//bc->AddBlock(data);
		std::cout << "[ AddBlockToBlockchain ] Added as candidate " << std::endl;
		return new_block;
	}

	Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* HandleAllBlockAdditionRequests() {
		std::tuple<void*, size_t> inbox_item;
		std::vector<std::tuple<void*, size_t> > read_messages = std::vector<std::tuple<void*, size_t> >();
		std::vector<Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>*>::iterator it;
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* best = NULL;

		std::cout << "Handling all block addition requests..." << std::endl;

		while(1) {
			if (inbox.Lock()) {
				if(!inbox.vec.empty()) {
					inbox_item = *(inbox.vec.begin());
					inbox.vec.erase(inbox.vec.begin());
					inbox.Unlock();
				} else {
					inbox.Unlock();
					break;
				}

				void * msg = std::get<0>(inbox_item);

				if (((struct BlockchainMessageHeader*)msg)->type == ADDREQUEST && ((struct BlockchainMessageHeader*)msg)->index == bc->Size()) {
					std::cout << "performing HandleBlockAdditionRequest"  << std::endl;
					HandleBlockAdditionRequest(msg, std::get<1>(inbox_item), true);
				} else {
					read_messages.push_back(inbox_item);
				}
			}											
		}

		it = addblock_candidates.begin();
		if(it != addblock_candidates.end()) {
			best = *it;
			++it;
			// Get the one with the lowest hash
			for (; it != addblock_candidates.end(); ++it) {
				// if the hash of the other is smaller than our best hash, we should take the other instead
				if ((*it)->GetHash().compare(best->GetHash()) <= 0) {
					delete best;
					best = *it;
				} else {
					delete *it;
				}
			}
			addblock_candidates.clear();
		}

		

		// Place back messages
		place_back(inbox, read_messages);

		return best;
	}

	void Run() {
		Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string>* best;
		std::tuple<void*, size_t> inbox_item;
		void * msg;
		size_t msg_len;
		sockaddr_in clntaddr;

		//std::vector<std::tuple<void*, size_t> > read_messages = std::vector<std::tuple<void*, size_t> >();

		while(1) {
			if(peer->is_online) {
				// If we were offline
				if (should_refresh(peer)) {
					// RequestBlockchain
					std::cout << "[ RequestBlockchain ] --------- Start --------- " << std::endl;
					//place_back(inbox, read_messages);
					RequestBlockchain();
					std::cout << "[ RequestBlockchain ] --------- Stop  --------- " << std::endl << std::endl;
				} else {
					if(inbox.Lock()) {
						std::cout << "Blockchain is size " << bc->Size() << " with top hash " << bc->GetTopHash() << std::endl;
						if(!inbox.vec.empty()) {
							
							inbox_item = *(inbox.vec.begin());
							inbox.vec.erase(inbox.vec.begin());
							inbox.Unlock();

							msg = std::get<0>(inbox_item);

							switch(((struct BlockchainMessageHeader*)msg)->type) {
									case REQUESTBLOCK:
										std::cout << "[ SendBlockchain ] --------- Start --------- " << std::endl;
										//std::cout << std::get<0>(inbox_item).sin_family << " " << std::get<0>(inbox_item).sin_port << " " << std::get<0>(inbox_item).sin_addr.s_addr << std::endl;
										//place_back(inbox, read_messages);
										SendBlockchain(*((struct BlockchainMessageHeader*)msg)/*, &std::get<0>(inbox_item)*/);
										free(msg);
										std::cout << "[ SendBlockchain ] --------- Stop  --------- " << std::endl << std::endl;
										break;

									case ADDREQUEST:
										RequestBlockchain();
										std::cout << "[ HandleBlockAdditionRequest ] --------- Start --------- " << std::endl;
										//place_back(inbox, read_messages);
										HandleBlockAdditionRequest(msg, std::get<1>(inbox_item), true);
										best = HandleAllBlockAdditionRequests();
										if (best){
											bc->AddBlock(best->GetData());
											delete best;
											std::cout << "[ HandleBlockAdditionRequest ] Some block got added " << bc->Size() << std::endl << std::endl;
										} else {
											std::cout << "[ HandleBlockAdditionRequest ] No block got added " << bc->Size() << std::endl << std::endl;
										}
										std::cout << "[ HandleBlockAdditionRequest ] --------- Stop  --------- " << std::endl << std::endl;

										break;
									case VOTE:
										free(msg);
									/*
										std::cout << "[ BlockVote ] --------- Start --------- " << std::endl;
										if (((struct BlockchainMessageHeader*) msg)->index < bc->Size()) {
											std::cout << "[ BlockVote ] Outdated vote " << ((struct BlockchainMessageHeader*) msg)->index << " while the next block would have index " << bc->Size() << std::endl;
											free(msg);
										} else {
											//if(inbox.Lock()) {
											//	inbox.vec.push_back(inbox_item); // Put it back
											//	inbox.Unlock();
											//}
											read_messages.push_back(inbox_item);
										}
										std::cout << "[ BlockVote ] --------- stop --------- " << std::endl << std::endl;
									*/
										// We add votes to a buffer, in case other's votes are received before the request
										// due to messages being received out of order 
										//votes.push_back(((struct BlockVote*)std::get<0>(inbox_item)));
										break;
									case BLOCK:
									case NOBLOCK:
									default:
										free(msg);
										break;
							}
							

						} else {
							inbox.Unlock();

							if (!transactions.empty()) {
								size_t block_id = bc->Size();
								log->LogBlockAddedStart(block_id);
								Transactions<ID_TYPE, MAX_TRANSACTIONS> ts = transactions.front(); 
								RequestBlockchain();
								std::cout << "[ AddBlockToBlockchain ] --------- Start --------- " << std::endl;
								//place_back(inbox, read_messages);
								//if (AddBlockToBlockchain(&ts)) {
								//	transactions.erase(transactions.begin());
								//}
								Block<Transactions<ID_TYPE, MAX_TRANSACTIONS>, std::string > * new_block = AddBlockToBlockchain(&ts);
								best = HandleAllBlockAdditionRequests();
								if (best){
									bc->AddBlock(best->GetData());
									if (new_block == best) {
										std::cout << "[ AddBlockToBlockchain ] Our block got added " << bc->Size() << std::endl << std::endl;
										transactions.erase(transactions.begin());
										log->LogBlockAddedStop(block_id, true);
									} else {
										std::cout << "[ AddBlockToBlockchain ] Another block got added instead " << bc->Size() << std::endl << std::endl;
										log->LogBlockAddedStop(block_id, false);
									}
									delete best;
								} else {
									std::cout << "[ AddBlockToBlockchain ] No block got added ... for some reason " << bc->Size() << std::endl << std::endl;
									log->LogBlockAddedStop(block_id, false);
								}
								std::cout << "[ AddBlockToBlockchain ] --------- Stop --------- " << std::endl << std::endl;
							} else {
								log->WriteBack(log_filename);
								std::cout << "[ Run ] No messages... "<< std::endl;
								//place_back(inbox, read_messages);
								for (int i = 0; i < rand() % 5; i++) {
									sleep(1);
								}
							}
						}
					}

				}
			} else {
				printf("We are offline...\n");
				sleep(1);
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
	srand(time(NULL));

	if (argc != 8) {
		printf("Usage: %s <tracker_addr> <tracker_port> <addr> <port> <transaction_trace> <output_file> <0/1 for initialization>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	std::cout << argv[6] << std::endl;

	Application* a = new Application(argv[1], argv[2], argv[3], argv[4], TransactionReader<ID_TYPE, MAX_TRANSACTIONS>::ReadFile(argv[5]), argv[6], argv[7][0]);
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
