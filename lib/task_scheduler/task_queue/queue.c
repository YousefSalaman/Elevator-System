
/*This script holds a FIFO queue object that manages the incoming task from the Raspberry Pi.*/

#include <stdlib.h>

#include "queue.h"


/** Queue methods **/

// Initializes task queue
task_queue_t init_task_queue(uint8_t size)
{
    task_queue_t queue;

    uint8_t ** head = malloc(sizeof(uint8_t *) * size);

    // Set tasks to NULL
    if (head != NULL)
    {
        for (uint8_t i = 0; i < size; i++)
        {
            head[i] = NULL;
        }
    }

    // Set up the other queue attributes
    queue.head = head;
    queue.next = head;
    queue.size = size;
    queue.is_empty = true;

    return queue;
}


// Deinitialize task queue
void deinit_task_queue(task_queue_t * queue)
{
    if (queue->head != NULL)
    {
        uint8_t * task = *queue->head;
        for (uint8_t i = 0; i < queue->size; i++)
        {
            free(task + i);
        }
    }
}


// Pushes a task to the task queue
void push_task(task_queue_t * queue, uint8_t id)
{
    if (queue->next < *queue->head + queue->size)
    {
        *queue->next++ = id;
        queue->is_empty = false;
    }
}


// Pops a task from the task queue
void pop_task(task_queue_t * queue)
{
    if (!queue->is_empty)
    {
        // It erases the first element in queue by shifting elements to the left
        for (uint8_t i = 0; queue->head + i < queue->next; i++)
        {
            queue->head[i] = (i == queue->size - 1)? NULL: queue->head[i + 1];
        }

        // Adjust pointer for next available space in queue
        queue->next--;
        if (queue->next == *queue->head)
        {
            queue->is_empty = true;
        }
    }
}


// Checks if a task is in the task queue
bool in_queue(task_queue_t * queue, uint8_t id)
{
    uint8_t * ptask = *queue->head; // Temporary pointer that points to tasks in the queue

    while (ptask < queue->next)
    {
        if ((*ptask++) == id)  // If task in queue matches id 
        {
            return true;
        }
    }

    return false;
}