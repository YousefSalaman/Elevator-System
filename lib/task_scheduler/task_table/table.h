#ifndef TASK_TABLE_H
#define TASK_TABLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**Task entry object
 * 
 * An object that represents an entry in the task lookup
 * table. This is meant to store a task with an id, so
 * this can be retrieved later and used when needed.
*/
typedef struct entry
{
    uint8_t id;
    void * task;
    uint16_t * size;
    struct entry * next;

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
    uint16_t size;
    task_entry_t ** entries;

} task_table_t;


/* Task table methods */

void deinit_task_table(task_table_t table);
task_table_t init_task_table(uint16_t size);
void* lookup_task(task_table_t table, uint8_t id);


#ifdef __cplusplus
}
#endif

#endif