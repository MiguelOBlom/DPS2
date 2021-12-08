#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <iostream>
#include <vector>
#include <string>
#include "sha256.h"



template <class T>
class Block {
public:
	Block<T>(T * data, std::string prev_hash, std::string (* hash_func) (T, std::string));
	T data;
	std::string hash;
	std::string prev_hash;
	int nonce;
};

template <class T>
class Blockchain {
public:
	Blockchain<T>(std::string (*hash_func_) (T, std::string));

	void addBlock(T * data);
	Block<T> * getBlock(size_t index);
	const std::vector<Block<T> > getBlocks() const;

	std::string getHash_index(size_t index);
	std::string getHash_block(Block<T> * blockptr);

private:
	std::string (*hash_func) (T, std::string);
	std::vector<Block<T>> blocks;
};

template <class T>
Block<T>::Block(T * data_, std::string prev_hash_, std::string (* hash_func) (T, std::string)) {
	memcpy(&data, data_, sizeof(T));
	prev_hash = prev_hash_;
	hash = hash_func(data, prev_hash);
}


template <class T>
Blockchain<T>::Blockchain(std::string (*hash_func_) (T, std::string)) {
	blocks = std::vector<Block<T> >();
	hash_func = hash_func_;
}


template <class T>
void Blockchain<T>::addBlock(T * data) {
	std::string prev_hash = (blocks.size() > 0) ? blocks.back().hash : "\0";

	Block<T> b(data, prev_hash, hash_func);

	blocks.push_back(b);
}

template <class T>
Block<T> * Blockchain<T>::getBlock(size_t index) {
	if (index < blocks.size()) {
		return &(blocks[index]);
	}
	return NULL;
}

template <class T>
const std::vector<Block<T> > Blockchain<T>::getBlocks() const {
	return blocks;
}

template <class T>
std::string Blockchain<T>::getHash_index(size_t index) {
	Block<T> * b = getBlock(index);
	if (b) {
		return b->hash;
	}
}

template <class T>
std::string Blockchain<T>::getHash_block(Block<T> * blockptr) {
	return blockptr->hash;
}

#endif