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
void move_to_back(list_node_t ** tail, list_node_t ** new_tail);
void move_to_front(list_node_t ** head, list_node_t ** new_head);


// Get item from list's head (this expects a list_node_t ** as an argument)
#define peek(head) (*head)->item

// Shorthand method to iterate over a list
#define list_iterator(head, node) list_node_t * node = head; node->item != NULL; node = node->next


#ifdef __cplusplus
}
#endif

#endif