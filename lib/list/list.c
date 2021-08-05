#include <stdlib.h>
#include <string.h>

#include "list.h"


/* Linked list function prototypes */

static list_node_t * create_item(void * item, size_t item_size);


/* Public linked list functions */

// Appends an item to the tail of the list
bool append(list_node_t ** tail, void * item, size_t item_size)
{
    list_node_t * new_node = create_item(item, item_size);

    if (new_node != NULL)
    {
        move_to_back(tail, &new_node);
    }

    return new_node != NULL;
}


// Appends an item to the left of the list
bool append_left(list_node_t ** head, void * item, size_t item_size)
{
    list_node_t * new_node = create_item(item, item_size);

    if (new_node != NULL)
    {
        new_node->next = *head;
        *head = new_node;
    }

    return new_node != NULL;
}


/**Clears up a list
 * 
 * It leaves a list with only one item, its tail. Assumes the
 * linked list is NULL-terminated and that the items do not
 * have any malloced information.
*/
void clear_list(list_node_t ** head)
{
    list_node_t * node = *head;

    // Free up everything except the last entry
    for (list_node_t * temp_node; node->item != NULL; node = temp_node)
    {
        temp_node = node->next;

        if (node->item != NULL)
        {
            free(node->item);
        }
        free(node);
    }
    *head = node;  // Make the tail the new head 
}


// Create an array of lists
list_node_t ** create_list_array(uint16_t size)
{
    list_node_t ** array = malloc(sizeof(list_node_t *) * size);

    if (array != NULL)
    {
        list_node_t * node = *array;

        for (uint16_t i = 0;  i < size; node = array[++i])
        {
            node->item = NULL;
        }
    }

    return array;
}


// Free up an array of lists
void erase_list_array(list_node_t ** array, uint16_t size)
{
    if (array != NULL)
    {
        list_node_t * head = *array;

        for (uint16_t i = 0; i < size; head = array[++i])
        {
            clear_list(&head);
            free(head);
        }
    }
}


// Place a new tail node in list
void move_to_back(list_node_t ** tail, list_node_t ** new_tail)
{
    if (*tail != NULL)
    {
        (*tail)->next = *new_tail;;
    }
    (*new_tail)->next = NULL;
    *tail = *new_tail;
}


// Place a new head node in list
void move_to_front(list_node_t ** head, list_node_t ** new_head)
{
    (*new_head)->next = *head;
    *head = *new_head;
}


/**Get the length of a list
 * 
 * Assumes linked list is NULL-terminated.
*/
uint16_t get_list_length(list_node_t * head)
{
    uint16_t length = 0;

    for (list_iterator(head, node))
    {
        length++;
    }

    return length;
}

/* Private linked list functions */

// Create a node and place an item in it
static list_node_t * create_item(void * item, size_t item_size)
{
    list_node_t * new_node = malloc(sizeof(list_node_t));

    if (new_node != NULL)
    {
        void * new_item = malloc(item_size);

        if (new_item == NULL)  // Free up the recently created node if no more space if available
        {
            free(new_node);
            new_node = NULL;
        }
        else  // Pass the item to the list node
        {
            memcpy(new_item, item, item_size);
            new_node->item = new_item;
        }
    }
    return new_node;
}