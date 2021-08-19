#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "../scheduler.h"


/*Scheduling queue constants */

// Sections where the scheduling queue initialization can fail

#define FAILED_SCHEDULING_QUEUES 0
#define FAILED_MEMORY_POOL       1
#define FAILED_QUEUE_ENTRIES     2


/* Scheduling queue prototypes */

static void link_queues(schedule_queues_t * queues, uint8_t queue_size);
static bool init_queue_entries(schedule_queues_t * queues, uint8_t queue_size, uint8_t pkt_size);
static list_node_t ** prepare_unscheduled_task(schedule_queues_t * queues, uint8_t task_id, uint8_t task_type, uint8_t* payload_pkt, uint8_t payload_size);


/** Public scheduling queue methods **/

// Initializes scheduling queues
schedule_queues_t * init_scheduling_queues(uint8_t queue_size, uint8_t pkt_size)
{
    uint8_t init_fail_section;
    schedule_queues_t * queues = malloc(sizeof(schedule_queues_t));

    // Check if scheduler queues where initialized
    if (queues == NULL)
    {
        init_fail_section = FAILED_SCHEDULING_QUEUES;
        goto handle_queue_deinit;
    }

    // Define attributes for scheduling system
    queues->size = queue_size;
    queues->schedule_pool = malloc(sizeof(list_node_t) * queue_size);  // Memory pool for scheduling tasks

    // Check if the schedule pool and the entries inside it where initialized
    if (queues->schedule_pool== NULL || !init_queue_entries(queues, queue_size, pkt_size))
    {
        init_fail_section = (queues->schedule_pool == NULL)? FAILED_MEMORY_POOL: FAILED_QUEUE_ENTRIES;
        goto handle_queue_deinit;
    }

    link_queues(queues, queue_size);
    return queues;

handle_queue_deinit:

    // Intentionally using switch fall-through to free up memory in order
    // when a part of scheduling fifos are not initialized correctly
    switch (init_fail_section)
    {
        case FAILED_QUEUE_ENTRIES:
            free(queues->schedule_pool);
        
        case FAILED_MEMORY_POOL:
            free(queues);

        case FAILED_SCHEDULING_QUEUES:
            return NULL;
    }    
}


// Uninitialize scheduling queues
void deinit_scheduling_queues(schedule_queues_t * queues)
{
    for (uint8_t i = 0; i < queues->size; i++)
    {
        queue_entry_t * entry = queues->schedule_pool[i].item;

        deinit_serial_pkt(&entry->pkt);
        free(entry);
    }

    free(queues->schedule_pool);
    free(queues);
}


bool push_task(schedule_queues_t * queues, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size, bool is_priority)
{
    list_node_t ** unscheduled_task_node = prepare_unscheduled_task(queues, task_id, task_type, payload_pkt, payload_size);

    if (unscheduled_task_node != NULL)  // If there are any unscheduled tasks
    {
        // Most recent scheduled task in corresponding fifo
        list_node_t ** tail_task_node = (is_priority)? &queues->priority_tail: &queues->normal_tail;

        move_to_back(tail_task_node, unscheduled_task_node);
    }

    return unscheduled_task_node != NULL;
}


bool push_task_to_front(schedule_queues_t * queues, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size, bool is_priority)
{
    list_node_t ** unscheduled_task_node = prepare_unscheduled_task(queues, task_id, task_type, payload_pkt, payload_size);

    if (unscheduled_task_node != NULL)
    {
        list_node_t ** head_task_node = (is_priority)? &queues->priority_head: &queues->normal_head;

        move_to_front(head_task_node, unscheduled_task_node);
    }

    return unscheduled_task_node != NULL;
}


void pop_task(schedule_queues_t * queues, bool is_priority)
{
    if (!queues_are_empty(queues))
    {
        // Oldest scheduled task in corresponding fifo
        list_node_t ** head_node = (is_priority)? &queues->priority_head: &queues->normal_head;

        // Reset old task
        queue_entry_t * completed_task = (*head_node)->item;

        completed_task->id = NULL;
        completed_task->pkt.byte_count = 0;
        completed_task->rescheduled = false;

        move_to_front(&queues->unscheduled, head_node);  // Push completed task to unscheduled task
    }
}


void reschedule_queue_task(schedule_queues_t * queues, bool is_priority)
{
    list_node_t ** head_node;
    list_node_t ** tail_node;

    if (is_priority)
    {
        head_node = &queues->priority_head;
        tail_node = &queues->priority_tail;
    }
    else
    {
        head_node = &queues->normal_head;
        tail_node = &queues->normal_tail;
    }

    ((queue_entry_t *) (*head_node)->item)->rescheduled = true;

    move_to_back(tail_node, head_node);
}


// Checks if a task is in the task queue
bool in_queue(schedule_queues_t * queues, uint8_t id)
{
    queue_entry_t * entry;
    list_node_t * queue_array[] = {queues->normal_head, queues->priority_head};

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
        queues->schedule_pool[i].item = entry;

        // If entry was not initialized correctly
        if (entry == NULL)
        {
            break;
        }

        // Initialize entry attributes
        entry->rescheduled = false;
        entry->pkt = init_serial_pkt(pkt_size);

        // If buffers where not initialized correctly
        if (entry->pkt.buf == NULL)
        {
            break;
        }
    }

    // Unitialize entries if no more space is left
    if (i != (int) queue_size)
    {
        for (; i < 0; i--)
        {
            queue_entry_t * entry = queues->schedule_pool[i].item;
        
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
static void link_queues(schedule_queues_t * queues, uint8_t queue_size)
{
    // Queue linking
    queues->normal_head = NULL;
    queues->normal_tail = queues->normal_head;
    queues->priority_head = NULL;
    queues->priority_tail = queues->priority_head;
    queues->unscheduled = queues->schedule_pool;

    // Link up memory pool
    for (uint8_t i = 0; i < queue_size; i++)
    {
        queues->schedule_pool[i].next = (i < queue_size - 1)? &queues->schedule_pool[i + 1]: NULL;
    }
}


// Check and prepare unscheduled task for scheduling
static list_node_t ** prepare_unscheduled_task(schedule_queues_t * queues, uint8_t task_id, uint8_t task_type, uint8_t* payload_pkt, uint8_t payload_size)
{
    if (queues_are_full(queues))
    {
        return NULL;
    }

    queue_entry_t * new_task = queues->unscheduled->item;  // Fetch an unscheduled task from stack

    if (!pass_outgoing_pkt(&new_task->pkt, task_id, task_type, payload_pkt, payload_size))
    {
        return NULL;
    }

    return &queues->unscheduled;
}