#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _QUEUE_
#define _QUEUE_

struct queue_lock {
	pthread_mutex_t lock;
	size_t top;
	size_t size;
	size_t max_size;
	void** data;
};

void queue_init(struct queue_lock* ql, size_t max_size);
void queue_delete(struct queue_lock* ql);
int queue_is_full (struct queue_lock* ql);
int queue_is_empty (struct queue_lock * ql);
int queue_enqueue (struct queue_lock* ql, void* new_entry);
void* queue_dequeue(struct queue_lock* ql);

// Would be unsafe, since critical section is left after
// function has returned, which means that the pointer 
// can exist outside of the queue environment
// So a dequeue from another thread may delete it
// We are left with a dangling pointer in the original thread
/*
void* queue_peek (struct queue_lock* ql) {
	void * ret = NULL;
	if (!queue_is_empty(ql)) {
		ret = data[(top - size + max_size) % max_size];
	}
	return ret;
}
*/


/*
max_size = 2;

enqueue 0:
data[0] changed [1, 0]
top: 0 -> 1
size: 0 -> 1

enqueue 1:
data[1] changed [1, 1]
top: 1 -> 0
size: 1 -> 2

dequeue 0:
(top - size + max_size) % max_size = (0 - 2 + 2) % 2 = 0
data[0] fetched [0, 1]
top: 0
size: 2 -> 1

enqueue 2:
data[0] changed [1, 1]
top: 0 -> 1
size: 1 -> 2

dequeue 1:
(top - size + max_size) % max_size = (1 - 2 + 2) % 2 = 1
data[1] fetched [1, 0]
top: 1
size: 2 -> 1

dequeue 2:
(top - size + max_size) % max_size = (1 - 1 + 2) % 2 = 0
data[0] fetched [0, 0]
top: 1
size: 1 -> 0


enqueue X:
	set data[top];
	top = (top + 1) % max_size;
	size++;

dequeue X:
	fetch data[(top - size + max_size) % max_size]
	//top = top;
	size--;
*/

#endif
