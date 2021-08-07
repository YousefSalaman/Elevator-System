#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>
#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#include "task_queue/queue.h"
#include "task_table/table.h"


#ifdef __cplusplus
extern "C" {
#endif


// Scheduler and helper types

/**Rx callback function
 * 
 * After a packet has been processed, this callback will be called with the
 * task function, task id, and its associated payload. The callback must
 * define a way to run the task with the given information.
*/
typedef void (*rx_schedule_cb)(uint8_t task_id, void * task, uint8_t * pkt);


typedef struct 
{
    // 
    uint8_t prev_task;
    task_table_t table;

    // Scheduler rx attributes
    serial_pkt_t rx_pkt;
    rx_schedule_cb rx_cb;

    // Scheduler tx attributes
    // tx_schedule_cb tx_cb;
    schedule_queues_t queues;

} task_scheduler_t;


// Scheduler constants

enum rx_pkt_offsets
{
    // Pre COBS decoding offsets

    SCHEDULER_HDR_OFFSET = 4,

    // Post COBS decoding offsets

    TASK_ID_OFFSET = 1,
    CRC_CHECKSUM_OFFSET = 2,
    PAYLOAD_OFFSET = 4
};


/* Scheduler functions */

// Task-related functions

// void schedule_task(task_scheduler_t * scheduler, uint8_t id);
void register_task(task_table_t * table, uint8_t id, int payload_size, void * task);

void deinit_task_scheduler(task_scheduler_t * scheduler);
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, uint16_t pkt_size);

void process_scheduler_rx_pkt(task_scheduler_t * scheduler);
bool process_incoming_byte(task_scheduler_t * scheduler, uint8_t byte);


#ifdef __cplusplus
}
#endif

#endif  /* SCHEDULER_H */