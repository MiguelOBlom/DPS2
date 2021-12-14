#ifndef _BLOCKCHAIN_
#define _BLOCKCHAIN_

#include <iostream>
#include <vector>
#include <sha256.h>
#include "block.h"

template <typename T, typename H>
class Blockchain {
public:
	Blockchain<T, H>(H (* hash_func_) (T, H));
	//void AddBlock(Block<T, H>* b);
	void AddBlock(const T* data);
	void PopBlock();
	Block<T, H>* SneakPeekBlock(const T* data);
	Block<T, H>* GetBlockFromIndex(const size_t index);
	Block<T, H>* GetTopBlock();
	const std::vector<Block<T, H> > GetBlocks() const;
	H GetHashAtIndex(const size_t index) const;
	H GetHashFromBlock(const Block<T, H>* blockptr) const;
	H GetTopHash() const;
	size_t Size();

private:
	H (* hash_func) (T, H);
	std::vector<Block<T, H> > blocks;
};

template <typename T, typename H>
Blockchain<T, H>::Blockchain(H (* _hash_func) (T, H)) {
	blocks = std::vector<Block<T, H> >();
	hash_func = _hash_func;

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
/*
template <typename T, typename H>
void Blockchain<T, H>::AddBlock(Block<T, H>* b) {
	blocks.push_back(b);
}
*/
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
