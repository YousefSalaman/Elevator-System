#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#include "../list/list.h"
#include "../serial_pkt/serial_pkt.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    int16_t id;        // Id to keep track of pending task
    bool rescheduled;  // Flag to determine if entry has been rescheduled
    serial_pkt_t pkt;  // Packet given to the pending task

} queue_entry_t;


/**Queues for scheduling tasks
 * 
 * 
*/
typedef struct task_queues
{
    uint8_t size;                  // Number of available task entries
    list_node_t * unscheduled;     // Stack of unused entries in memory pool
    list_node_t * normal_head;     // FIFO head for scheduled task entries
    list_node_t * normal_tail;     // FIFO tail for scheduled task entries
    list_node_t * priority_head;   // FIFO head for scheduled task entries with priority
    list_node_t * priority_tail;   // FIFO tail for scheduled task entries with priority
    list_node_t * schedule_pool;   // Memory pool for tasks

} schedule_queues_t;


/* Scheduling queues methods */

// Queue init/deinit methods

void deinit_scheduling_queues(schedule_queues_t * queues);
schedule_queues_t * init_scheduling_queues(uint8_t queue_size, uint8_t pkt_size);

// Queue peeking methods

/**Peek at the head of one of the scheduling fifos
 * 
 * The function passes a copy of the of entry head for the corresponding
 * scheduling fifo.
*/

#define peek_normal(queues) ((queue_entry_t *) (queues)->normal_head->item)      // Pass entry of the normal scheduling fifo
#define peek_priority(queues) ((queue_entry_t *) (queues)->priority_head->item)  // Pass entry of the priority scheduling fifo

// Queue popping methods

void pop_task(schedule_queues_t * queues, bool is_priority);

#define pop_normal_task(queues) pop_task(queues, false)
#define pop_priority_task(queues) pop_task(queues, true)

// Queue pushing methods

bool push_task(schedule_queues_t * queues, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size, bool is_priority, bool is_fast);

#define push_normal_task(queues, task_id, pkt, pkt_size) push_task(queues, task_id, pkt, pkt_size, false, false)
#define push_priority_task(queues, task_id, pkt, pkt_size) push_task(queues, task_id, pkt, pkt_size, true, false)

void prioritize_normal_task(schedule_queues_t * queues);

// Rescheduling queue methods

void reschedule_queue_task(schedule_queues_t * queues, bool is_priority);

// Queue status verification methods

bool in_queue(schedule_queues_t * queues, uint8_t id);

#define queues_are_full(queues) ((queues)->unscheduled == NULL)

#define is_normal_queue_empty(queues) ((queues)->normal_head == NULL)
#define is_priority_queue_empty(queues) ((queues)->priority_head == NULL) 
#define queues_are_empty(queues) (is_priority_queue_empty(queues) && is_normal_queue_empty(queues))


#ifdef __cplusplus
}
#endif

#endif