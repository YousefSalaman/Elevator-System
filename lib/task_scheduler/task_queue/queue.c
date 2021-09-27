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
    uint8_t init_fail_section = FAILED_SCHEDULING_QUEUES;
    schedule_queues_t * queues = malloc(sizeof(schedule_queues_t));

    // Check if scheduler queues where initialized
    if (queues == NULL)
    {
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
    }    

    return NULL;
}


// Uninitialize scheduling queues
void deinit_scheduling_queues(schedule_queues_t * queues)
{
    if (queues != NULL)
    {
        for (uint8_t i = 0; i < queues->size; i++)
        {
            queue_entry_t * entry = queues->schedule_pool[i].item;

            deinit_serial_pkt(&entry->pkt);
        }

        free(queues->schedule_pool->item);
        free(queues->schedule_pool);
        free(queues);
    }
}


bool push_task(schedule_queues_t * queues, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size, bool is_priority, bool to_front)
{
    list_node_t ** unscheduled_task_node = prepare_unscheduled_task(queues, task_id, task_type, payload_pkt, payload_size);

    // If there are no unscheduled tasks, then tell the system
    // tasks were scheduled
    if (unscheduled_task_node == NULL)
    {
        return false;
    }

    // Place unscheduled task in one of the queues

    list_node_t * next_unscheduled_head = (*unscheduled_task_node)->next;

    if (to_front) // Place task in the front
    {
        list_node_t ** head_task_node = (is_priority)? &queues->priority_head: &queues->normal_head;
        move_to_front(head_task_node, unscheduled_task_node); 
    }
    else  // Place task in the back
    {
        list_node_t ** tail_task_node = (is_priority)? &queues->priority_tail: &queues->normal_tail;

        // If the queue is empty, correctly adjust the FIFO
        // by making the head the tail
        if ((*tail_task_node) == NULL)
        {
            list_node_t** head_task_node = (is_priority) ? &queues->priority_head : &queues->normal_head;
            
            *head_task_node = *unscheduled_task_node;
        }

        move_to_back(tail_task_node, unscheduled_task_node);
    }

    *unscheduled_task_node = next_unscheduled_head;

    return true;
}


void pop_task(schedule_queues_t * queues, bool is_priority)
{
    if (!queues_are_empty(queues))
    {
        // Oldest scheduled task in corresponding fifo
        list_node_t ** head_node;
        list_node_t ** tail_node;

        // Fetch the tail and head of the corresponding fifo
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

        // If the current node is the last one in the FIFO, then
        // also unlink the tail to correctly adjust the FIFO
        if (*head_node == *tail_node)
        {
            *tail_node = NULL;
        }

        // Reset old task
        queue_entry_t * completed_task = (*head_node)->item;

        completed_task->id = -1;
        completed_task->pkt.byte_count = 0;
        completed_task->rescheduled = false;

        list_node_t * next_head = (*head_node)->next;

        move_to_front(&queues->unscheduled, head_node);  // Push completed task to unscheduled task

        *head_node = next_head;
    }
}


// Grants priority to the head of the normal queue
void prioritize_normal_task(schedule_queues_t * queues)
{
    list_node_t * next_normal_head = queues->normal_head->next;

    move_to_front(&queues->priority_head, &queues->normal_head);

    queues->normal_head = next_normal_head;
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
            if (entry->id  == id)
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
    int i = 0;

    // Create entries with packets

    queue_entry_t * schedule_item_pool = malloc(sizeof(queue_entry_t) * queue_size);

    if (schedule_item_pool != NULL)
    {
        for (i = 0; i < queue_size; i++)
        {
            queue_entry_t * entry = &schedule_item_pool[i];

            queues->schedule_pool[i].item = entry;

            // Initialize entry attributes
            entry->id = -1;
            entry->rescheduled = false;
            entry->pkt = init_serial_pkt(pkt_size);

            // If buffers where not initialized correctly
            if (entry->pkt.buf == NULL)
            {
                break;
            }
            
        }
    }

    // Unitialize entries if no more space is left
    if (i != (int) queue_size)
    {
        for (; i < 0; i--)
        {
            queue_entry_t * entry = queues->schedule_pool[i].item;
            deinit_serial_pkt(&entry->pkt);
        }

        free(schedule_item_pool);
    }

    return i == (int) queue_size;
}


/**Link queues and memory pool
 */ 
static void link_queues(schedule_queues_t * queues, uint8_t queue_size)
{
    // Queue linking
    queues->normal_head = NULL;
    queues->normal_tail = NULL;
    queues->priority_head = NULL;
    queues->priority_tail = NULL;
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
    // If no task is available for scheduling, then return nothing
    if (queues_are_full(queues))
    {
        return NULL;
    }

    queue_entry_t * new_task = queues->unscheduled->item;  // Fetch an unscheduled task from stack

    // Verify if outgoing packet object was processed correctly
    if (!process_outgoing_pkt(&new_task->pkt, task_id, task_type, payload_pkt, payload_size))
    {
        return NULL;
    }

    new_task->id = task_id;  // Pass the task id to the unscheduled task node

    return &queues->unscheduled;
}