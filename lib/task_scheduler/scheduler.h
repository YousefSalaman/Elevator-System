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

typedef void (*rx_schedule_cb)(uint8_t *);

typedef struct 
{
    uint16_t size;
    uint8_t * in_buf;
    uint8_t * out_buf;
    uint16_t byte_count;

} serial_rx_pkt_t;


typedef struct
{
    uint16_t size;
    uint8_t * buf;
    uint16_t byte_count;

} serial_tx_pkt_t;


typedef struct 
{
    uint8_t prev_task;
    task_queue_t queue;
    task_table_t table;
    serial_rx_pkt_t rx_pkt;
    serial_tx_pkt_t tx_pkt;

} task_scheduler_t;


// Scheduler constants

// enum rx_pkt_offsets
// {

// };


/* Scheduler functions */

// Task-related functions
void schedule_task(task_scheduler_t * scheduler, uint8_t id);
void register_task(task_table_t * table, uint8_t id, int payload_size, void * task);


void deinit_task_scheduler(task_scheduler_t * scheduler);
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, uint16_t pkt_size);

void process_rx_scheduler_pkt(task_scheduler_t * scheduler);
bool process_incoming_byte(task_scheduler_t * scheduler, uint8_t byte);


#ifdef __cplusplus
}
#endif

#endif  /* SCHEDULER_H */