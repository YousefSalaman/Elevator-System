#include <stdlib.h>
#include <string.h>

#include "list.h"

/* Public linked list functions */

// Adds a new item to the list
bool add_item(list_node_t ** head, void * item, size_t item_size)
{
    list_node_t * new_node = malloc(sizeof(list_node_t));

    if (new_node != NULL)
    {
        void * new_item = malloc(item_size);

        // Free up the recently created node if no more space if available
        if (new_item == NULL)
        {
            free(new_node);
        }

        // Pass the item to the list
        else
        {
            memcpy(new_item, item, item_size);  
    
            new_node->item = new_item;
            new_node->next = *head;

            *head = new_node;  // Make our node the new head of the list

            return true;
        }
    }
    return false;
}


/**Clears up a list
 * 
 * It leaves a list with only one item, its tail. This assumes the
 * list's tail has a NULL pointer as the item.
*/
void clear_list(list_node_t ** head)
{
    list_node_t * node = *head;

    // Free up everything except the last entry
    for (list_node_t * temp_node; node->item != NULL; node = temp_node)
    {
        temp_node = node->next;
        free(node->item);
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

        return array;
    }
    return NULL;
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


/**Get the length of a list
 * 
 * It assumes the list's tail has a NULL pointer as the
 * item.
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
