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
    uint8_t * id;
    serial_pkt_t pkt;

} queue_entry_t;


typedef struct task_queue
{
    uint8_t size;    // Size of queue
    bool is_empty;   // Indicates if queue is empty or not
    uint8_t* next;   // Next available space in queue
    uint8_t** head;

} task_queue_t;


typedef struct task_queues
{
    uint8_t size;
    list_node_t * normal;          // Scheduled task entries
    list_node_t * priority;        // Scheduled task entries with priority
    list_node_t * unscheduled;     // Unused entries in memory pool
    list_node_t ** schedule_pool;  // Pointer to memory pool

} schedule_queues_t;


/* Scheduling queue methods */

#define queues_are_full(queues) ((queues)->unscheduled->next == NULL)

// Shorthands for pushing/popping to correct queue

#define pop_normal_task(queues) pop_task(queues, false)
#define pop_priority_task(queues) pop_task(queues, true)
#define push_normal_task(queues, task_id, pkt, pkt_size) push_task(queues, task_id, pkt, pkt_size, false);
#define push_priority_task(queues, task_id, pkt, pkt_size) push_task(queues, task_id, pkt, pkt_size, true);


task_queue_t init_task_queue(uint8_t size);
void deinit_task_queue(schedule_queues_t ** queues);
bool in_queue(schedule_queues_t * queues, uint8_t id);

void pop_task(schedule_queues_t ** queues, bool is_priority);
bool push_task(schedule_queues_t ** queues, uint8_t task_id, uint8_t * pkt, uint8_t pkt_size, bool is_priority);


#ifdef __cplusplus
}
#endif

#endif