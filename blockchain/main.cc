#include <iostream>
#include <vector>

template <class T>
char * simpleHash(void * data) {
	char * bytes = data;
	// Compute simple hash 
}

template <class T>
class Block {
public:
	Block<T>(T data, char * prev_hash /*hashing function*/);
	T data;
	char * hash;
	char * prev_hash;
};

template <class T>
Block<T>::Block(T data_, char * prev_hash_, char (*hash_funct) (T, char*))  {
	data = data_;
	prev_hash = prev_hash_;
	hash = hash_funct (data, prev_hash);
}


template <class T>
class Blockchain {
public:
	Blockchain<T>(/*hashing function*/);

	void addBlock(T data);
	Block<T> * getBlock(size_t index);
	const std::vector<Block<T> > getBlocks() const;

	char * getHash(size_t index);
	char * getHash(Block<T> * blockptr);

private:
	void setGenesisBlock();
	std::vector<Block<T> > blocks;

};

template <class T>
Blockchain<T>::Blockchain() {
	blocks = std::vector<Block<T> >();
	setGenesisBlock();
}

template <class T>
void Blockchain<T>::setGenesisBlock() {
	addBlock(0);
}


template <class T>
void Blockchain<T>::addBlock(T data) {
	char * prev_hash = (blocks.size() > 0)? blocks.back().prev_hash: "\0";

	Block <T> b(data, prev_hash);

	blocks.push_back(block);
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


int main () {

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