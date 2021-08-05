#ifndef LIST_OBJ_H
#define LIST_OBJ_H

#include <stddef.h>
#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct list_node 
{
    void * item;
    struct list_node * next;

} list_node_t;


// Linked list methods

void pop(list_node_t ** head);
void clear_list(list_node_t ** head);
uint16_t get_list_length(list_node_t * head);
list_node_t ** create_list_array(uint16_t size);
void erase_list_array(list_node_t ** array_head, uint16_t size);

bool append(list_node_t ** tail, void * item, size_t item_size);
void move_to_back(list_node_t ** tail, list_node_t ** new_tail);
void move_to_front(list_node_t ** head, list_node_t ** new_head);
bool append_left(list_node_t ** head, void * item, size_t item_size);


// Get item from list's head (this expects a list_node_t ** as an argument)
#define peek(head) (*head)->item

// Shorthand method to iterate over a list
#define list_iterator(head, node) list_node_t * node = head; node->item != NULL; node = node->next


#ifdef __cplusplus
}
#endif

#endif