#include "list.h"

// Find a way to make a binary search tree using the IP and PORT
// also interpret returned data as an array of datatypes

struct element {
	size_t id;
	struct element* next;
	void *data;	
};

struct list create_list(size_t element_size) {
	struct list list;
	list.first = NULL;
	list.n_elements = 0;
	list.element_size = element_size;
	return list;
}

void list_add_element(struct list * list, size_t id, void * data) {
	// Check if we do not overstep max data
	struct element* element;

	if (list->n_elements >= (SIZE_MAX / list->element_size) - 1) {
		printf("Could not add element to list, since malloc would need to allocate a larger amount of data than size_t would be able to store.");
		return;
	} else {
		element = (struct element*) malloc(sizeof(struct element));
		printf("%d\n", *((int*)data));
		element->next = list->first;
		element->id = id;
		element->data = data;
		printf("%d\n", *((int*)element->data));
		list->first = element;
		list->n_elements++;
	}


}

void* list_remove_element(struct list * list, size_t id) {
	void* data = NULL;
	struct element* q = NULL; 
	struct element* p = list->first;

	while (p) {
		if (p->id == id) {
			if (q) {
				q->next = p->next;
			} else {
				list->first = p->next;
			}
			data = p->data;
			free(p);
			break;
		}

		q = p;
		p = p->next;
	}
	return data;
}

void* list_list_elements(const struct list* list, size_t * data_len) {
	void* data;
	size_t elements_handled = 0;
	struct element* p;
	
	*data_len = list->n_elements * list->element_size;
	printf("int: %lu, data_len = %lu\n", sizeof(int), *data_len);
	data = malloc(*data_len);

	if (!data) {
		printf("Too much data in list.");
	} else {
		p = list->first;

		while (p) {
			printf("%p, %lu, %p\n", data, elements_handled * list->element_size, data + elements_handled * list->element_size);
			memcpy(data + elements_handled * list->element_size, p->data, list->element_size);
			p = p->next;
			elements_handled++;
		}
	}

	return data;
}

//void * list_foreach(const struct list* list, )

/*

// USAGE:

int main() {
	int* data;
	size_t data_len;
	struct list l = create_list(sizeof(*data));

	for (int i = 0; i < 10; i++){
		data = malloc(sizeof(int));
		*data = i;
		list_add_element(&l, i, data);
	}

	void * elements = list_list_elements(&l, &data_len);

	for (size_t i = 0; i < l.n_elements; ++i) {
		printf("%p: %d\n", (elements + i * l.element_size), *((int*)(elements + i * l.element_size)));
	}

	free(elements);
	
	for (size_t i = 0; i < 10; ++i) {
		data = list_remove_element(&l, i);

		if (data) {
			free(data);
		}
	}


	return 0;
}
*/