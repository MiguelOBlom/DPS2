/*
	
	Implementation for a generic datatype Block where we can
	choose the type T of the data that is stored in the block
	and the type H of the hashes.

	Authors: Miguel Blom, Matt van den Nieuwenhuijzen

*/

#include <cstring>

#ifndef _BLOCK_
#define _BLOCK_

template <typename T, typename H>
class Block {
public:
	// Constructor for initializing all private members to zero
	Block<T, H>();
	// Constructor for initializing the private members given parameters
	Block<T, H>(const T* data, const H hash, const H prev_hash);
	
	// Setter for the private member data 
	void SetData(const T* _data);
	// Setter for the private member hash
	void SetHash(const H _hash);
	// Setter for the private member previous_hash
	void SetPrevHash(const H _hash);

	// Getter for the private member data as return value
	T* GetData() const;
	// Getter for the private member data as call by reference
	void GetData(T* & out) const;

	// Getter for the private member hash
	H GetHash() const;
	// Getter for the private member prev_hash
	H GetPrevHash() const;

private:
	// The data and hashes of the block
	T data;
	H hash;
	H prev_hash;
};

template <typename T, typename H>
Block<T, H>::Block() {
	T default_t = T();
	SetData(&default_t);
	SetHash(H());
	SetPrevHash(H());
}

template <typename T, typename H>
Block<T, H>::Block(const T* _data, const H _hash, const H _prev_hash){
	SetData(_data);
	SetHash(_hash);
	SetPrevHash(_prev_hash);
}

template <typename T, typename H>
void Block<T, H>::SetData(const T* _data) {
	memcpy(&data, _data, sizeof(T));
}

template <typename T, typename H>
void Block<T, H>::SetHash(const H _hash) {
	hash = _hash;
}

template <typename T, typename H>
void Block<T, H>::SetPrevHash(const H _hash) {
	prev_hash = _hash;
}

template <typename T, typename H>
T* Block<T, H>::GetData() const {
	T* retval = new T;
	memcpy(retval, &data, sizeof(T));
	return retval;
}

template <typename T, typename H>
void Block<T, H>::GetData(T* & out) const {
	out = GetData();
}

template <typename T, typename H>
H Block<T, H>::GetHash() const {
	return hash;
}

template <typename T, typename H>
H Block<T, H>::GetPrevHash() const {
	return prev_hash;
}

#endif
