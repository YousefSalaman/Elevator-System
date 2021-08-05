
/* This script holds a FIFO queue object to store and manage incoming tasks. */

#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "../scheduler.h"


/** Public scheduling queue methods **/

// Initializes scheduling queues
schedule_queues_t * init_scheduling_queues(uint8_t queue_size, uint8_t pkt_size)
{
    schedule_queues_t * queues = malloc(sizeof(schedule_queues_t));

    // Check if scheduler queues where initialized
    if (queues == NULL)
    {
        goto handle_uninitialized_queues;
    }

    queues->size = queue_size;
    queues->schedule_pool = malloc(sizeof(list_node_t *) * queue_size);

    // Check if the schedule pool and the entries inside it where initialized
    if (queues->schedule_pool == NULL || !init_queue_entries(queues, queue_size, pkt_size))
    {
        goto handle_uninitialized_queues;
    }

    link_queues(&queues, queue_size);
    return queues;

// Free up memory if some part of the queues were not initialized correctly
handle_uninitialized_queues:

    if (queues != NULL)
    {
        if (queues->schedule_pool != NULL)
        {
            free(queues->schedule_pool);
        }
        free(queues);
    }
    return NULL;
}


// Deinitialize task queue
void deinit_task_queue(schedule_queues_t ** queues)
{
    for (uint8_t i = 0; i < (*queues)->size; i++)
    {
        queue_entry_t * entry = (*queues)->schedule_pool[i]->item;

        deinit_serial_pkt(&entry->pkt);
        free(entry);
    }

    free((*queues)->schedule_pool);
    free(*queues);
}


bool push_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t * pkt, uint8_t pkt_size, bool is_priority)
{
    queue_entry_t * new_task = prepare_unscheduled_task(queues, task_id, pkt, pkt_size);

    if (new_task != NULL)
    {
        if (is_priority)
        {
            // The head of the unscheduled queue has as its item a pointer to the tail
            // of the priority fifo. This was set up during the "link_queues" function,
            // so this operation is now O(1)
            move_to_back(&(*queues)->unscheduled->item, &new_task);
        }
        else
        {
            // The head of the priority queue has as its item a pointer to the tail of
            // the normal fifo. This was set up during the "link_queues" function, so
            // this operation is now O(1)
            move_to_back(&(*queues)->priority->item, &new_task);
        }
        
    }
    return new_task != NULL;
}


void pop_task(schedule_queues_t ** queues, bool is_priority)
{
    list_node_t * scheduled_task;

    if (is_priority)
    {
        scheduled_task = (*queues)->priority->next;
    }
    else
    {
        scheduled_task = (*queues)->normal->next;
    }

    // The heads of the queues have a poiter to the real head of the fifos, so
    // moving this task to the front of the unscheduled stack, unschedules them
    // and makes the operation O(1)
    move_to_front(&(*queues)->unscheduled->next, &scheduled_task);
}


// Checks if a task is in the task queue
bool in_queue(schedule_queues_t * queues, uint8_t id)
{
    queue_entry_t * entry;

    // Search in normal fifo for task
    for (list_iterator(queues->normal, queue_node))
    {
        entry = queue_node->item;
        if (*entry->id  == id)
        {
            return true;
        }
    }

    // Search in priority fifo for task
    for (list_iterator(queues->priority, queue_node))
    {
        entry = queue_node->item;
        if (*entry->id  == id)
        {
            return true;
        }
    }

    return false;
}


/** Private scheduling queue methods **/

// Initialize entries in the scheduling memory pool
static bool init_queue_entries(schedule_queues_t * queues, uint8_t queue_size, uint8_t pkt_size)
{
    int i;

    // Create entries with packets
    for (i = 0; i < queue_size; i++)
    {
        queue_entry_t * entry = malloc(sizeof(queue_entry_t));
        queues->schedule_pool[i]->item = entry;

        // If entry was not initialized correctly
        if (entry == NULL)
        {
            break;
        }

        entry->id = NULL;
        entry->pkt = init_serial_pkt(pkt_size);

        // If buffers where not initialized correctly
        if (entry->pkt.in_buf == NULL || entry->pkt.out_buf == NULL)
        {
            break;
        }
    }

    // Unitialize entries if no more space is left
    if (i != (int) queue_size)
    {
        for (; i < 0; i--)
        {
            queue_entry_t * entry = queues->schedule_pool[i]->item;
        
            deinit_serial_pkt(&entry->pkt);
            if (entry != NULL)
            {
                free(entry);
            }
        }
    }

    return i == (int) queue_size;
}


/**Link queues and memory pool
 * 
 * TODO: May replace with inline function.
 */ 
static void link_queues(schedule_queues_t ** queues, uint8_t queue_size)
{
    // Queue linking
    (*queues)->normal->next = NULL;
    (*queues)->normal->item = NULL;
    (*queues)->priority->next = NULL;
    (*queues)->priority->item = (*queues)->normal;              // Save reference to achieve O(1) normal fifo push
    (*queues)->unscheduled->item = (*queues)->priority;         // Save reference to achieve O(1) priority fifo push
    (*queues)->unscheduled->next = *(*queues)->schedule_pool;   // Save reference to memory pool for assigning schedules

    // Link up memory pool
    for (uint8_t i = 0; i < queue_size; i++)
    {
        (*queues)->schedule_pool[i]->next = (i < queue_size - 1)? (*queues)->schedule_pool[i + 1]: NULL;
    }
}


// Check and prepare unscheduled task for scheduling
static queue_entry_t * prepare_unscheduled_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t* pkt, uint8_t pkt_size)
{
    if (!queues_are_full(*queues))
    {
        queue_entry_t * new_task = (*queues)->unscheduled->next->item;  // Fetch an unscheduled task from stack

        // Check if packet has the correct size
        if (pkt_size > new_task->pkt.size)
        {
            return NULL;
        }

        // Set up new task entry
        new_task->id = task_id;
        new_task->pkt.byte_count = pkt_size;
        memcpy(new_task->pkt.in_buf, pkt, pkt_size);

        return new_task;
    }
}