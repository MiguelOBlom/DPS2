#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifndef LIST_H_
#define LIST_H_

struct element;
struct list {
	struct element* first;
	size_t n_elements;
	size_t element_size;
};


struct list create_list(size_t element_size);
void list_add_element(struct list * list, size_t id, void * data);
void* list_remove_element(struct list * list, size_t id);
void* list_list_elements(const struct list* list, size_t * data_len);

#endif // LIST_H_