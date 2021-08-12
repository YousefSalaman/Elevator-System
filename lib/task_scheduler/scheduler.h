#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Public scheduler constants */

// Offsets

#define RET_CODE_OFFSET 6 // 

// Task types

#define INTERNAL_TASK 0
#define EXTERNAL_TASK 1

// Task time range for the other system to reply

#define SHORT_TIMER 350  // Allowed time for the first reply time range
#define LONG_TIMER  500  // Allowed time for the second reply time range

// Internal command ids

#define ALERT_SYSTEM    0
#define PRINT_MESSAGE   1
#define UNSCHEDULE_TASK 2
#define MODIFY_TASK_VAL 3

// Internal pseudo-command ids (these are for sending messages)

#define PKT_DECODE    0
#define PKT_ENCODE    1
#define TASK_LOOKUP   2
#define TASK_REGISTER 3


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


/* Scheduler functions */

void deinit_task_scheduler(void);
void init_task_scheduler(uint8_t queue_size, uint16_t table_size, rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb);

void send_task(void);
void perform_task(void);
bool store_task_rx_byte(uint8_t byte);
void register_task(uint8_t id, int payload_size, void * task);

// Task scheduling methods

void schedule_task(uint8_t id, uint8_t type, uint8_t * pkt, uint8_t pkt_size, bool is_priority, bool is_fast);

/**Schedule a normal task
 * 
 * A normal task is a task that when scheduled, it will wait for a reply by
 * an external device running the scheduling system so it can remove the
 * task from the normal scheduling queue. 
 * 
 * It uses the timer callback to check if the allowed reply time has passed to wait for the other system to reply. If it 
 * doesn't reply in time, the task will be rescheduled with a larger timer
 * window for the other system to reply. If this also fails, the system will
 * unschedule the task.
 */
#define schedule_normal_task(id, payload_pkt, payload_size) schedule_task(id, EXTERNAL_TASK, payload_pkt, payload_size, false, false)

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
#define schedule_priority_task(id, payload_pkt, payload_size) schedule_task(id, EXTERNAL_TASK, payload_pkt, payload_size, true, false)

/**Schedule an immediate priority task
 * 
 * This type of task follows the same rules as a priority task, but
 * this system will immediately send the task to the other system
 * connected to the microcontroller.
*/
#define schedule_fast_task(id, type, payload_pkt, payload_size) schedule_task(id, type, payload_pkt, payload_size, true, true)

// Internal scheduler commands

/**Alert when a task has been completed
 * 
 * The id and return code must be a number from 0 to 255. If the return
 * code is non-zero, the system will assume an error has ocurred while
 * running the task.
*/
#define alert_task_completion(id, ret_code) schedule_fast_task(ALERT_SYSTEM, INTERNAL_TASK, ((uint8_t []) {id, ret_code}), sizeof(uint8_t) * 2)

#define print_message(id, msg_num) schedule_fast_task(PRINT_MESSAGE, INTERNAL_TASK, ((uint8_t []) {id, EXTERNAL_TASK, msg_num}), sizeof(uint8_t) * 3)
#define print_internal_message(id, msg_num) schedule_fast_task(PRINT_MESSAGE, INTERNAL_TASK, ((uint8_t []) {id, INTERNAL_TASK, msg_num}), sizeof(uint8_t) * 3)

void send_task_val(uint8_t task_id, uint8_t task_type, uint8_t value_id, void * value, size_t size);

#define modify_task_val(id, val_id, val, size) send_task_val(id, EXTERNAL_TASK, val_id, val, size)
#define modify_internal_task_val(id, val_id, val, size) send_task_val(id, INTERNAL_TASK, val_id, val, size)

#ifdef __cplusplus
}
#endif

#endif  /* SCHEDULER_H */