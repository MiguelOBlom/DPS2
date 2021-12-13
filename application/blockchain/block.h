#include <cstring>

#ifndef _BLOCK_
#define _BLOCK_


template <typename T, typename H>
class Block {
public:
	Block<T, H>();
	Block<T, H>(const T* data, const H hash, const H prev_hash);
	void SetData(const T* _data);
	void SetHash(const H _hash);
	void SetPrevHash(const H _hash);

	T* GetData() const;
	void GetData(T* & out) const;
	H GetHash() const;
	H GetPrevHash() const;


private:
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
	//memset(&data, 0, sizeof(T));
	//memset(&hash, 0, sizeof(H));
	//memset(&prev_hash, 0, sizeof(H));
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