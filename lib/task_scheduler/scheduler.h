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
 * have a way to handle how the given task is executed with the given 
 * information.
*/
typedef void (*rx_schedule_cb)(uint8_t task_id, void * task, uint8_t * pkt);


typedef struct
{
    char * msg;   // Buffer to store the msg in
    void * args;  // Arguments to pass to error functions
    uint8_t type; // Type of error

} err_t;


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
    err_t err;
    uint8_t prev_task;
    task_queue_t queue;
    task_table_t table;
    rx_schedule_cb rx_cb;
    serial_rx_pkt_t rx_pkt;
    serial_tx_pkt_t tx_pkt;

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