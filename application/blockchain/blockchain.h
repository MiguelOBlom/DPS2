/*
	Implementation for a generic datatype Blockchain for handling
	the storage of Blocks. The storage is done in a linear fashion
	allowing for a single chain of blocks.

	Authors: Miguel Blom, Matt van den Nieuwenhuijzen

*/


#include <iostream>
#include <vector>
#include <sha256.h>
#include "block.h"

#ifndef _BLOCKCHAIN_
#define _BLOCKCHAIN_

template <typename T, typename H>
class Blockchain {
public:
	// Constructor for the blockchain
	// Creates a genesis block by calling the default ctor of Block
	// The callback function is a hash function used for computing the
	// hash of a block
	Blockchain<T, H>(H (* hash_func_) (T, H));

	// Adds a block of generic type T data to the chain
	// Will also handle the configuration of the hash and previous hash
	void AddBlock(const T* data);

	// Removes a single block from the end of the chain
	void PopBlock();

	// Performs the same configuration as AddBlock, but returns it
	// rather than adding it to the chain, this is used to 
	// get a block as how it would be like when adding it to the chain
	Block<T, H>* SneakPeekBlock(const T* data);

	// Fetches a block at a certain index from the chain and returns its address
	Block<T, H>* GetBlockFromIndex(const size_t index);

	// Get the top block of the chain, this is the same as calling
	// Blockchain::GetBlockFromIndex(Blockchain::Size() - 1)
	Block<T, H>* GetTopBlock();

	// Get the blockchain as a vector of blocks
	const std::vector<Block<T, H> > GetBlocks() const;

	// Get the hash of a block at a certain index in the chain, this is the same as calling
	// Blockchain::GetBlockFromIndex(index).GetHash()
	H GetHashAtIndex(const size_t index) const;

	// Fetches the hash of a block
	H GetHashFromBlock(const Block<T, H>* blockptr) const;

	// Get the hash at the top of the chain, this is the same as calling
	// Blockchain::GetBlockFromIndex(Blockchain::Size() - 1).GetHash()
	H GetTopHash() const;

	// Returns the size of the blockchain
	size_t Size();

private:
	// Hash function that computes a hash of generic type H from generic type T data
	H (* hash_func) (T, H);

	// The vector of blocks
	std::vector<Block<T, H> > blocks;
};

template <typename T, typename H>
Blockchain<T, H>::Blockchain(H (* _hash_func) (T, H)) {
	blocks = std::vector<Block<T, H> >();
	hash_func = _hash_func;

	// Add the first block (genesis block) to the chain
	Block<T, H> genesis;
	T* data = genesis.GetData();
	H prev_hash = genesis.GetPrevHash();
	genesis.SetHash(hash_func(*data, prev_hash));
	delete data;

	blocks.push_back(genesis);
}

template <typename T, typename H>
Block<T, H>* Blockchain<T, H>::SneakPeekBlock(const T* data) {
	std::string prev_hash = blocks.back().GetHash();
	return new Block<T, H>(data, hash_func(*data, prev_hash), prev_hash);
}

template <typename T, typename H>
void Blockchain<T, H>::AddBlock(const T* data) {
	std::string prev_hash = blocks.back().GetHash();
	Block<T, H> b(data, hash_func(*data, prev_hash), prev_hash);
	blocks.push_back(b);
}

template <typename T, typename H>
void Blockchain<T, H>::PopBlock() {
	if (blocks.size() > 0)
	{
		blocks.pop_back();
	}
}

template <typename T, typename H>
Block<T, H>* Blockchain<T, H>::GetBlockFromIndex(const size_t index) {
	if (index < blocks.size()) {
		return &blocks[index];
	} else {
		return NULL;
	}
}

template <typename T, typename H>
Block<T, H>* Blockchain<T, H>::GetTopBlock() {
	return &blocks.back();
}

template <typename T, typename H>
const std::vector<Block<T, H> > Blockchain<T, H>::GetBlocks() const {
	return blocks;
}

template <typename T, typename H>
H Blockchain<T, H>::GetHashAtIndex(const size_t index) const {
	Block<T, H>* b = GetBlockFromIndex(index);
	if (b) {
		return b->GetHash();
	}
}

template <typename T, typename H>
H Blockchain<T, H>::GetHashFromBlock(const Block<T, H>* blockptr) const {
	return blockptr->hash;
}

template <typename T, typename H>
size_t Blockchain<T, H>::Size() {
	return blocks.size();
}

template <typename T, typename H>
H Blockchain<T, H>::GetTopHash() const {
	return blocks.back().GetHash();
}

#endif
