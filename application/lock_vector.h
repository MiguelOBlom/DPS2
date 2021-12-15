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
	pthread_mutex_t lock;
public:
	std::vector<T> vec = std::vector<T>();

	LockVector<T>() {
		if (pthread_mutex_init(&lock, 0) != 0) {
			perror("Cannot initialize vector mutex lock");
			exit(EXIT_FAILURE);
		}
	}

	~LockVector<T>() {
		pthread_mutex_destroy(&lock);
	}

	bool Lock() {
		if(pthread_mutex_lock(&lock) == 0) {
			return true;
		} else {
			perror("Error while locking vector mutex");
			exit(EXIT_FAILURE);
		}
		return false; // Redundant
	}


	bool Unlock () {
		if (pthread_mutex_unlock(&lock) != 0) {
			perror("Error unlocking queue mutex");
			exit(EXIT_FAILURE);
			return false; // Redundant
		}
		return true;
	}

};

#endif

