#include <iostream>
#include <vector>
#include <string>
#include "sha256.h"

/*
datastructuur
hashfunctie crypto.h?
referenties in code
proof of work: computing
alles wat je test laat je staan
*/

// create problem

// solve problem

// checken problem

// template <class T>
// struct DataHash {
// 	T data;
// 	std::string hash;
// };

// template <class T>
// struct simpleHash
// {
//     std::string operator()(DataHash const& DH) const noexcept
//     {
//         std::size_t h1 = std::hash<T>{}(DH.data);
//         std::string h2 = sha256(DH.hash);
// 		return sha256(std::to_string(h1) + h2);
//     }
// };

template <class T>
std::string simpleHash(T data, std::string hash)
{
	std::size_t h1 = std::hash<T>{}(data);
	std::string h2 = sha256(hash);
	return sha256(std::to_string(h1) + h2);
}


template <class T>
class Block {
public:
	Block<T>(T data, std::string prev_hash, std::string (* hash_func) (T, std::string));
	T data;
	std::string hash;
	std::string prev_hash;
};

template <class T>
Block<T>::Block(T data_, std::string prev_hash_, std::string (* hash_func) (T, std::string)) {
	data = data_;
	prev_hash = prev_hash_;
	hash = hash_func(data, prev_hash);
}

// ============================================

template <class T>
class Blockchain {
public:
	Blockchain<T>(std::string (*hash_func_) (T, std::string));

	void addBlock(T data);
	Block<T> * getBlock(size_t index);
	const std::vector<Block<T>> getBlocks() const;

	std::string getHash_index(size_t index);
	std::string getHash_block(Block<T> * blockptr);

private:
	std::string (*hash_func) (T, std::string);
	std::vector<Block<T>> blocks;

	// void setGenesisBlock();

};

template <class T>
Blockchain<T>::Blockchain(std::string (*hash_func_) (T, std::string)) {
	blocks = std::vector<Block<T>>();
	hash_func = hash_func_;
}

// First block added must contain data so this function is not necessary (?)
// template <class T>
// void Blockchain<T>::setGenesisBlock() {
// 	addBlock(0);
// }


template <class T>
void Blockchain<T>::addBlock(T data) {
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
const std::vector<Block<T>> Blockchain<T>::getBlocks() const {
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

int blockchaintest1() {
	Blockchain<int> bc(simpleHash);
	bc.addBlock(12);
	bc.addBlock(12);
	bc.addBlock(23);
	std::cout << bc.getBlock(0)->data << std::endl;
	std::cout << bc.getHash_index(1) << std::endl;
	std::cout << bc.getBlock(1)->data << std::endl;
	std::cout << bc.getHash_index(2) << std::endl;
	std::cout << bc.getBlock(2)->data << std::endl;
	std::cout << bc.getHash_index(3) << std::endl;

	Blockchain<std::string> bc2(simpleHash);
	bc2.addBlock("I receive 10 bitcoins from you.");
	bc2.addBlock("I receive 10 bitcoins from you.");
	std::cout << "yo" << std::endl;
	std::cout << bc2.getBlock(0)->data << std::endl;
	std::cout << bc2.getHash_index(0) << std::endl;
	std::cout << bc2.getBlock(1)->data << std::endl;
	std::cout << bc2.getHash_index(1) << std::endl;
	return 0;
}

int main () {
	blockchaintest1();
	return 0;
}




// Tests creation of blockchain and block
// Adding a block to the chain
// Retrieving the reference to a block
// Setting the data of a retrieved block
// Recalling the reassigned data of the retrieved block
// Return value should be 12
int test1(){
	/*
	Blockchain<int> bc;
	Block<int> b;
	Block<int> * bp;
	b.data = 42;
	bc.addBlock(b);
	bp = bc.getBlock(0);
	bp->data = 12;
	bp = bc.getBlock(0);
	return bp->data; 
	*/
}

