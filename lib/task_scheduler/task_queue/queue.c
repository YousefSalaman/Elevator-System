
/* This script holds a FIFO queue object to store and manage incoming tasks. */

#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "../scheduler.h"


/* Scheduling queue prototypes */

static void link_queues(schedule_queues_t ** queues, uint8_t queue_size);
static bool init_queue_entries(schedule_queues_t * queues, uint8_t queue_size, uint8_t pkt_size);
static list_node_t ** prepare_unscheduled_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t* pkt, uint8_t pkt_size);


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
    queues->unscheduled->item = malloc(sizeof(list_node_t *) * queue_size);  // Memory pool for scheduling tasks

    // Check if the schedule pool and the entries inside it where initialized
    if (queues->unscheduled->item == NULL || !init_queue_entries(queues, queue_size, pkt_size))
    {
        goto handle_uninitialized_queues;
    }

    link_queues(&queues, queue_size);
    return queues;

// Free up memory if some part of the queues were not initialized correctly
handle_uninitialized_queues:

    if (queues != NULL)
    {
        if (queues->unscheduled->item != NULL)
        {
            free(queues->unscheduled->item);
        }
        free(queues);
    }
    return NULL;
}


// Uninitialize task queue
void deinit_task_queue(schedule_queues_t ** queues)
{
    list_node_t ** scheduling_pool = (*queues)->unscheduled->item;

    for (uint8_t i = 0; i < (*queues)->size; i++)
    {
        queue_entry_t * entry = scheduling_pool[i]->item;

        deinit_serial_pkt(&entry->pkt);
        free(entry);
    }

    free(scheduling_pool);
    free(*queues);
}


bool push_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t * pkt, uint8_t pkt_size, bool is_priority)
{
    list_node_t ** unscheduled_task = prepare_unscheduled_task(queues, task_id, pkt, pkt_size);

    if (unscheduled_task != NULL)  // If there are any unscheduled tasks
    {
        list_node_t ** last_scheduled_task;  // Most recent scheduled task in corresponding fifo

        last_scheduled_task = (is_priority)? &(*queues)->priority->item: &(*queues)->normal->item;
        move_to_back(last_scheduled_task, unscheduled_task);
    }

    return unscheduled_task != NULL;
}


void pop_task(schedule_queues_t ** queues, bool is_priority)
{
    if (!are_queues_empty(*queues))
    {
        list_node_t ** first_scheduled_task;  // Oldest scheduled task in corresponding fifo

        first_scheduled_task = (is_priority)? &(*queues)->priority->next: &(*queues)->normal->next;

        // Reset old task
        queue_entry_t * completed_task = (*first_scheduled_task)->item;

        completed_task->id = NULL;
        completed_task->pkt.byte_count = 0;

        move_to_front(&(*queues)->unscheduled->next, first_scheduled_task);
    }
}


// Checks if a task is in the task queue
bool in_queue(schedule_queues_t * queues, uint8_t id)
{
    queue_entry_t * entry;
    list_node_t * queue_array[] = {queues->normal, queues->priority};

    // Search the normal and priority pending task fifos for a matching task id
    for (uint8_t i = 0; i < 2; i++)
    {
        for (list_iterator(queue_array[i], queue_node))
        {
            entry = queue_node->item;
            if (*entry->id  == id)
            {
                return true;
            }
        }
    }

    return false;  // Return false if nothing was found
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
    list_node_t ** scheduling_pool = (*queues)->unscheduled->item;

    // Queue linking
    (*queues)->normal->next = NULL;
    (*queues)->normal->item = (*queues)->normal;       // Save reference of the normal tail for O(1) pushing
    (*queues)->priority->next = NULL;
    (*queues)->priority->item = (*queues)->priority;   // Save reference of the priorirty tail for O(1) pushing
    (*queues)->unscheduled->next = *scheduling_pool;   // Save reference to memory pool for assigning schedules

    // Link up memory pool
    for (uint8_t i = 0; i < queue_size; i++)
    {
        scheduling_pool[i]->next = (i < queue_size - 1)? scheduling_pool[i + 1]: NULL;
    }
}


// Check and prepare unscheduled task for scheduling
static list_node_t ** prepare_unscheduled_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t* pkt, uint8_t pkt_size)
{
    if (queues_are_full(*queues))
    {
        return NULL;
    }

    queue_entry_t * new_task = (*queues)->unscheduled->next->item;  // Fetch an unscheduled task from stack

    // Check if packet has the correct size
    if (pkt_size > new_task->pkt.size)
    {
        return NULL;
    }

    // Set up new task entry
    *new_task->id = task_id;
    new_task->pkt.byte_count = pkt_size;
    memcpy(new_task->pkt.in_buf, pkt, pkt_size);

    return &(*queues)->unscheduled->next;
}