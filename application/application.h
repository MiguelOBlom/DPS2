/*
	This class implements the main application of our project.
	It uses the peer-to-peer network and blockchain for 
	creating a transactional system.

	Authors: Miguel Blom and Matt van den Nieuwenhuijzen

*/


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

#ifndef _APPLICATION_
#define _APPLICATION_

// Transactions type
typedef Transactions<ID_TYPE, MAX_TRANSACTIONS> T_TYPE;
// Block type
typedef Block<T_TYPE, std::string> B_TYPE;

// Types of blockchain messages used by the application
enum BlockchainMessageType {
	BLOCK,
	REQUESTBLOCK,
	NOBLOCK,
	VOTE,
	ADDREQUEST,
};

// The message header for a blockchain message
struct BlockchainMessageHeader {
	BlockchainMessageType type;
	struct peer_address peer_address;
	size_t index;
};

// Message header when voting for a block
struct BlockVote {
	struct BlockchainMessageHeader bmh;
	bool agree;
	size_t hash_size;
};

// Message header when requesting the addition of a block
struct BlockchainAdditionRequest {
	struct BlockchainMessageHeader bmh;
	size_t pow_size;
};

// A header for the sizes of a sent block
struct BlockData {
	size_t hash_size;
	size_t prev_hash_size;
	size_t data_size;
};

class Application {
private:
	// Logger information
	// Whenever the log writes back the first time, it means that the peer 
	// has handled all its additions
	char * log_filename;
	Logger* log;

	// Blockchain information
	Blockchain<T_TYPE, std::string> * bc;
	IProofOfWork<std::string, std::string> * pow_group;

	// Mailbox information
	LockVector<std::tuple<void*, size_t> > inbox = LockVector<std::tuple<void*, size_t> >();
	pthread_t inbox_thread_id; 

	// Transactions to be processed
	std::vector<T_TYPE> transactions;

	// Storage for candidates when adding blocks
	std::vector<B_TYPE*> addblock_candidates = std::vector<B_TYPE*>();

	// Peer information
	struct peer * peer;
	
	// Initialize the blockchain with the transactions in our trace
	void initialize();

	// Check a majority of blocks voted for
	// Return whether there was a majority, the majority block is stored in largest_block
	bool checkMajority(B_TYPE* & largest_block, const size_t& i, const size_t& n_peers, const size_t& no_block, std::map<std::string, std::pair<B_TYPE*, size_t> >& messages);

	// Check if a peer is present in a vector of peers, return true if found, otherwise false
	bool client_found(const struct peer_address* clnt_pa, std::vector<struct peer_address>& clients_seen);

	// Broadcast a message requesting a block in the blockchain
	// and update the blockchain according to their answer
	// Return whether there were blocks available, the resulting
	// block is stored in largest_block
	bool RequestBlockchainBlock(B_TYPE* & largest_block);

	// Request the whole blockchain until fully updated with the rest of the network
	void RequestBlockchain ();

	// Send a block from our blockchain to a requester 
	// according to the info in the blockchain message header
	void SendBlockchain(struct BlockchainMessageHeader& bmh);

	// Vote for a block to be added to the blockchain
	// If there was a majority vote agreeing to add the block, returns true, otherwise false
	// If we do not allow to agree, we automatically let the voting fail, this feature is
	// used when we find that our hash has a smaller value than someone else's add block request
	bool HandleBlockAdditionRequest(void* request_message, size_t request_len, bool allow_agree);

	// Await votes and check whether a majority agrees
	// with adding the block 
	bool CheckVotesForBlock(size_t n_peers, B_TYPE* req_block, bool own_vote);

	// Broadcast an ADDBLOCK request of a new block with specified data
	// We return the block upon completion
	B_TYPE* AddBlockToBlockchain (T_TYPE* data);

	// Handle all block addition requests in the mailbox
	// This is to ensure the voting gets handled properly
	B_TYPE* HandleAllBlockAdditionRequests();

public:
	// Constructor
	Application (char* tracker_addr, char* tracker_port, char* addr, char* port, std::vector<T_TYPE> _transactions, char * _log_filename, char init);

	// Destructor
	~Application ();

	// Run the application!
	void Run();

};

#endif
