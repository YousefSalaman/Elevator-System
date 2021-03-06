#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "../scheduler_config.h"


/* Public task table functions */

// Hash function for the lookup table
#define hash(table, id) id % (table).size


// Initialize task table for the elevator comms
task_table_t init_task_table(uint8_t size)
{
    task_table_t table;
    task_entry_t ** entries = malloc(sizeof(task_entry_t *) * size);

    // Set values of the table to NULL
    if (entries != NULL)
    {
        for (uint16_t i = 0; i < size; i++)
        {
            entries[i] = NULL;
        }
    }

    // Set up values for the table object
    table.size = size;
    table.entries = entries;

    return table;
}


// Uninitialize the task table
void deinit_task_table(task_table_t table)
{
    task_entry_t * entry;
    task_entry_t * temp_entry;

    // Free up registered entries in table
    if (table.entries != NULL)
    {
        for (int i = 0; i < table.size; i++)
        {
            // Free up elements in the linked list for a given slot in the table
            for (entry = table.entries[i]; entry != NULL; entry = temp_entry)
            {
                temp_entry = entry->next;
                free(entry);
            }
        }
    }

    free(table.entries);  // Free up entry array 
}


// Returns a task with the given task number
task_entry_t * lookup_task(task_table_t table, uint8_t id)
{
    // Search and return task in table with the resulting hash number
    for (task_entry_t * entry = table.entries[hash(table, id)]; entry != NULL; entry = entry->next)
    {
        if (entry->id == id)
        {
            return entry;
        }
    }
    return NULL;  // No task was found with the given id
}


/**Add a task to the task table
 * 
 * NOTE: If payload size is set to a non-positive integer, the code will
 * disable packet size checking this task.
 */
void register_task_in_table(task_table_t * table, uint8_t id, int8_t payload_size, task_t task) 
{
    // Add task to table if one was not added with the same id
    if (lookup_task(*table, id) == NULL)
    {
        uint8_t hash_value = hash(*table, id);
        task_entry_t * entry = malloc(sizeof(task_entry_t));

        // Set attributes for new entry 
        if (entry != NULL)
        {
            entry->id = id;
            entry->task = task;
            entry->size = payload_size;
            entry->next = table->entries[hash_value];

            table->entries[hash_value] = entry;  // Make new entry head of list
        }
    }
}
