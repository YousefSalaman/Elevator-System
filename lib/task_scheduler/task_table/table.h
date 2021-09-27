#ifndef TASK_TABLE_H
#define TASK_TABLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// Generic task type
typedef void (*task_t)(void);

/**Task entry object
 * 
 * An object that represents an entry in the task lookup
 * table. This is meant to store a task with an id, so
 * this can be retrieved later and used when needed.
*/
typedef struct entry
{
    task_t task;          // Task to be executed
    uint8_t id;           // Id (unique identifier) for the task
    int16_t size;         // Size of the payload of the entry (if negative, it skips checking the length)
    struct entry * next;  // Next entry in entry stack for a given hash

} task_entry_t;


/**Task table object
 * 
 * Lookup table to store tasks given by an external 
 * source. It uses a basic hash function to store the
 * entries in the table, each slot in the table is a
 * linked list with the possible values the hash
 * function can output.
*/
typedef struct 
{
    uint8_t size;             // Size of the table
    task_entry_t ** entries;  // Array of stacks to store entries

} task_table_t;


/* Task table methods */

void deinit_task_table(task_table_t table);
task_table_t init_task_table(uint8_t size);
task_entry_t * lookup_task(task_table_t table, uint8_t id);
void register_task_in_table(task_table_t * table, uint8_t id, int8_t payload_size, task_t task);


#ifdef __cplusplus
}
#endif

#endif