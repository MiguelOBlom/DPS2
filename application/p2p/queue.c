#include "queue.h"

void queue_init(struct queue_lock* ql, size_t max_size) {
	// Initialize the mutex lock
	if (pthread_mutex_init(&ql->lock, 0) != 0) {
		perror("Cannot initialize queue mutex lock");
		exit(EXIT_FAILURE);
	}

	// Configure members of the queue_lock structure
	ql->top = 0;
	ql->size = 0;
	ql->max_size = max_size;
	ql->data = (void**) malloc(sizeof(void*) * max_size);
}

void queue_delete(struct queue_lock* ql) {
	pthread_mutex_destroy(&ql->lock);
	free(ql->data);
}

int queue_is_full (struct queue_lock* ql) {
	return ql->size == ql->max_size;
}

int queue_is_empty (struct queue_lock * ql) {
	return ql->size == 0;
}

int queue_enqueue (struct queue_lock* ql, void* new_entry) {
	int ret = 0;
	// Lock the queue
	if(pthread_mutex_lock(&ql->lock) == 0) {
		// Place the item in the queue
		if (!queue_is_full(ql)) {
			ql->data[ql->top] = new_entry;
			ql->top = (ql->top + 1) % ql->max_size;
			ql->size++;
			ret = 1;
		}

		// Unlock the queue
		if (pthread_mutex_unlock(&ql->lock) != 0) {
			perror("Error unlocking queue mutex");
			exit(EXIT_FAILURE);
		}

	} else {
		perror("Error while locking queue mutex");
		exit(EXIT_FAILURE);
	}

	return ret;
}

void* queue_dequeue(struct queue_lock* ql) {
	void* ret = NULL;
	// Lock the queue
	if(pthread_mutex_lock(&ql->lock) == 0) {
		// Remove an item from the queue
		if (!queue_is_empty(ql)) {
			ret = ql->data[(ql->top - ql->size + ql->max_size) % ql->max_size];
			ql->size--;
		}

		// Unlock the queue
		if (pthread_mutex_unlock(&ql->lock) != 0) {
			perror("Error unlocking queue mutex");
			exit(EXIT_FAILURE);
		}

	} else {
		perror("Error while locking queue mutex");
		exit(EXIT_FAILURE);
	}
	// Return the item
	return ret;
}