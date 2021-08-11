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


/* Public scheduler constants */

// Task types

#define INTERNAL_TASK 0
#define EXTERNAL_TASK 1

// Task time range for the other system to reply

#define SHORT_TIMER 350  // Allowed time for the first reply time range
#define LONG_TIMER  500  // Allowed time for the second reply time range

/* Scheduler and helper types */

typedef unsigned long (*timer_schedule_cb)(void);

typedef void (*tx_schedule_cb)(uint8_t * pkt, uint8_t pkt_size);

/**Rx callback function
 * 
 * After a packet has been processed, this callback will be called with the
 * task function, task id, and its associated payload. The callback must
 * define a way to run the task with the given information.
*/
typedef uint8_t (*rx_schedule_cb)(uint8_t task_id, void * task, uint8_t * pkt);

typedef struct 
{
    // Task scheduling attributes
    uint8_t prev_task;
    task_table_t table;
    unsigned long start_time;
    schedule_queues_t * queues;
    timer_schedule_cb timer_cb;

    // Scheduler rx attributes
    serial_pkt_t rx_pkt;
    rx_schedule_cb rx_cb;

    // Scheduler tx attributes
    tx_schedule_cb tx_cb;

} task_scheduler_t;


/* Scheduler functions */

void deinit_task_scheduler(task_scheduler_t * scheduler);
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb)

void send_task(task_scheduler_t * scheduler);
void perform_task(task_scheduler_t * scheduler);
void register_task(task_table_t * table, uint8_t id, int payload_size, void * task);

// Task scheduling methods

void schedule_task(task_scheduler_t * scheduler, uint8_t id, uint8_t type, uint8_t * pkt, uint8_t pkt_size, bool is_priority, bool is_fast);

/**Schedule a normal task
 * 
 * A normal task is a task that when scheduled, it will wait for a reply by
 * an external device running the scheduling system to be removed from the
 * normal scheduling queue. 
 * 
 * It uses an internal timer to wait for the other system to reply. If it 
 * doesn't reply in time, the task will be rescheduled with a larger timer
 * window for the other system to reply. If this also fails, the system will
 * unschedule the task.
 */
#define schedule_normal_task(scheduler, id, payload_pkt, payload_size) schedule_task(scheduler, id, EXTERNAL_TASK, payload_pkt, payload_size, false, false)

/**Schedule a priority task
 * 
 * A priority task is a task that will take precedence over normal tasks.
 * That is, if a normal task and priority task have been scheduled, the 
 * system will schedule the priority task first.
 * 
 * These tasks will not wait for the other system connected to the
 * microcontroller for a reply, so these tasks are unscheduled than normal
 * tasks, but they also run the risk of not knowing what happened in the 
 * other system.
 * 
 * Furthermore, these tasks don't use a timer and also they run even if 
 * a normal task is waiting for a reply.
*/
#define schedule_priority_task(scheduler, id, payload_pkt, payload_size) schedule_task(scheduler, id, EXTERNAL_TASK, payload_pkt, payload_size, true, false)

/**Schedule an immediate priority task
 * 
 * This type of task follows the same rules as a priority task, but
 * this system will immediately send the task to the other system
 * connected to the microcontroller.
*/
#define schedule_fast_task(scheduler, id, type, payload_pkt, payload_size) schedule_task(scheduler, id, type, payload_pkt, payload_size, true, true)

#define store_rx_byte(scheduler, byte) process_incoming_byte(&(scheduler).rx_pkt, byte)


#ifdef __cplusplus
}
#endif

#endif  /* SCHEDULER_H */