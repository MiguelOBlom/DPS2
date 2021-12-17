/*
	This code implements a vector using a mutex lock, similar to
	that of p2p/queue.h.

	Author: Miguel Blom
*/


#include <pthread.h>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef _LOCKVECTOR_
#define _LOCKVECTOR_

template <typename T>
class LockVector {
private:
	// Mutex lock used for locking this vector
	pthread_mutex_t lock;
public:
	// Vector in which items are stored
	std::vector<T> vec = std::vector<T>();

	// Constructor for this vector, initializes the mutex lock
	LockVector<T>();

	// Destructor, cleans up the mutex lock
	~LockVector<T>();

	// Tries to lock the mutex lock, and returns true
	// on success, otherwise the program exits
	bool Lock();

	// Tries to unlock the mutex lock, and returns true
	// on success, otherwise the program exits
	bool Unlock ();

};

template <typename T>
LockVector<T>::LockVector() {
	if (pthread_mutex_init(&lock, 0) != 0) {
		perror("Cannot initialize vector mutex lock");
		exit(EXIT_FAILURE);
	}
}

template <typename T>
LockVector<T>::~LockVector() {
	pthread_mutex_destroy(&lock);
}

template <typename T>
bool LockVector<T>::Lock() {
	if(pthread_mutex_lock(&lock) == 0) {
		return true;
	} else {
		perror("Error while locking vector mutex");
		exit(EXIT_FAILURE);
	}
	return false; // Redundant
}

template <typename T>
bool LockVector<T>::Unlock () {
	if (pthread_mutex_unlock(&lock) != 0) {
		perror("Error unlocking queue mutex");
		exit(EXIT_FAILURE);
		return false; // Redundant
	}
	return true;
}


#endif

