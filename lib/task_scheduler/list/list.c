#include <stdlib.h>
#include <string.h>

#include "list.h"

/* Public linked list functions */

/**Place a new tail node in list
 * 
 * This function applies to the more general case in which
 * the new tail might have been taken from another linked
 * list.
 */ 
void move_to_back(list_node_t ** tail, list_node_t ** new_tail)
{
    if (*tail != NULL)
    {
        (*tail)->next = *new_tail;;
    }
    (*new_tail)->next = NULL;
    *tail = *new_tail;
}


/**Place a new head node in list
 * 
 * This function applies to the more general case in which
 * the new tail might have been taken from another linked
 * list.
 */ 
void move_to_front(list_node_t ** head, list_node_t ** new_head)
{
    (*new_head)->next = *head;
    *head = *new_head;
}