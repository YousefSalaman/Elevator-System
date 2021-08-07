#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#include <list.h>

#include "../serial_pkt/serial_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    uint8_t * id;      // Id to keep track of pending task
    serial_pkt_t pkt;  // Packet given to the pending task

} queue_entry_t;


/**Queues for scheduling tasks
 * 
 * 
*/
typedef struct task_queues
{
    uint8_t size;                  // Number of available task entries
    list_node_t * normal;          // FIFO for scheduled task entries
    list_node_t * priority;        // FIFO for Scheduled task entries with priority
    list_node_t * unscheduled;     // Stack of unused entries in memory pool
    list_node_t ** schedule_pool;  // Pointer to task entry memory pool

} schedule_queues_t;


/* Scheduling queue methods */

void deinit_task_queue(schedule_queues_t ** queues);
bool in_queue(schedule_queues_t * queues, uint8_t id);
schedule_queues_t * init_scheduling_queues(uint8_t queue_size, uint8_t pkt_size);

void pop_task(schedule_queues_t ** queues, bool is_priority);
bool push_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t * pkt, uint8_t pkt_size, bool is_priority);


/* Scheduling queues function-like macros */

#define queues_are_full(queues) ((queues)->unscheduled->next == NULL)

// Check if the queues are empty

#define is_normal_queue_empty(queues) ((queues)->priority->next == NULL)
#define is_priority_queue_empty(queues) ((queues)->normal->next == NULL)
#define are_queues_empty(queues) (is_priority_queue_empty(queues) && is_normal_queue_empty(queues))


// Shorthands for pushing/popping to correct queue

#define pop_normal_task(queues) pop_task(queues, false)
#define pop_priority_task(queues) pop_task(queues, true)
#define push_normal_task(queues, task_id, pkt, pkt_size) push_task(queues, task_id, pkt, pkt_size, false);
#define push_priority_task(queues, task_id, pkt, pkt_size) push_task(queues, task_id, pkt, pkt_size, true);


#ifdef __cplusplus
}
#endif

#endif