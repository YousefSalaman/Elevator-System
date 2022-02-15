#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#include "task_table/table.h"
#include "scheduler_config.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Scheduler constants */

/**Scheduler printer unpacking keys
 * 
 * Whenever you want to change a task printer variable on the main computer,
 * you need to specify what variable type you are sending, so the it can be
 * correctly processed by the main computer, which is why these are provided
 * below.
 * 
 * You will notice that the variable types are ASCII/Unicode characters.
 * The reason for this is because the Python struct module uses these to 
 * know what format an incoming byte stream must be interpreted/unpacked
 * and the 'c' format seems to be using Unicode to interpret the characters.
 * To avoid any mishaps, use the constants below to describe the type of
 * variable you want to send. However, the amount of bytes per data type may
 * vary depending on the MCUs, so you should verify the internal data type
 * sizes before sending it to the main computer.
 */ 

#define PRINT_SIZE_T     '\x4E'  // Prints size_t variable
#define PRINT_SSIZE_T    '\x6E'  // Prints ssize_t variable
#define PRINT_INT8_T     '\x42'  // Prints signed char and expects 1 byte
#define PRINT_UINT8_T    '\x62'  // Prints unsigned char and expects 1 byte
#define PRINT_INT16_T    '\x68'  // Prints signed short and expects 2 bytes
#define PRINT_UINT16_T   '\x48'  // Prints unsigned short and expects 2 bytes
#define PRINT_INT32_T    '\x69'  // Prints signed integer and expects 4 bytes
#define PRINT_UINT32_T   '\x49'  // Prints unsigned integer and expects 4 bytes
#define PRINT_INT64_T    '\x51'  // Prints signed long long and expects 8 bytes
#define PRINT_UINT64_T   '\x71'  // Prints unsigned long long and expects 8 bytes
#define PRINT_FLOAT16_T  '\x65'  // Prints half-precision float and expects 2 bytes
#define PRINT_FLOAT32_T  '\x66'  // Prints single-precision float and expects 4 bytes
#define PRINT_FLOAT64_T  '\x64'  // Prints double-precision float and expects 8 bytes
#define PRINT_BOOL       '\x3F'  // Prints the correspoding boolean value and expects 1 byte
#define PRINT_CHAR       '\x63'  // Prints the corresponding ASCII/Unicode character and expects 1 byte


/* Scheduler and helper types */

typedef unsigned long (*timer_schedule_cb)(void);

/**Tx callback function
 * 
 * When a task needs to be sent to another destination, this callback will
 * be called with the packet and the amount of bytes to sent.
 */
typedef void (*tx_schedule_cb)(uint8_t * pkt, uint8_t pkt_size);

/**Rx callback function
 * 
 * After a packet has been processed, this callback will be called with the
 * task function, task id, and its associated payload. The callback must
 * define a way to run the task with the given information.
*/
typedef uint8_t (*rx_schedule_cb)(uint8_t task_id, task_t task, uint8_t * pkt);


/* Scheduler functions */

void deinit_task_scheduler(void);
bool init_task_scheduler(rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb);

void send_task(void);
void build_rx_task_pkt(uint8_t byte);

void register_task_private(uint8_t id, int payload_size, task_t task);

#define register_task(id, payload_size, task) register_task_private((id), (payload_size), (task_t) (task))

// Task scheduling methods

void schedule_task(uint8_t id, uint8_t type, uint8_t * pkt, uint8_t pkt_size, bool is_priority, bool is_fast);

/**Schedule a normal task
 * 
 * A normal task is a task that when scheduled, it will wait for a reply by
 * an external device running the scheduling system so it can remove the
 * task from the normal scheduling queue. 
 * 
 * It uses the timer callback to check if the allowed reply time has passed to
 * wait for the other system to reply. If it does not reply in time, the task
 * will be rescheduled with a larger timer window for the other system to reply.
 * If this also fails, the system will reschedule the task in the priority queue
 * and tell the other system to execute it and unschedule it afterwards without
 * verifying if it was executed properly in the other system.
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
 * the system will immediately send the task to the other system
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

void send_printer_task_var(uint8_t task_id, uint8_t task_type, uint8_t value_id, uint8_t value_type, void * value, size_t size);

#define modify_printer_var(id, var_id, var_type, var, size) send_printer_task_var(id, EXTERNAL_TASK, var_id, var_type, var, size)
#define modify_internal_printer_var(id, var_id, var_type, var, size) send_printer_task_var(id, INTERNAL_TASK, var_id, var_type, var, size)

#ifdef __cplusplus
}
#endif

#endif  /* SCHEDULER_H */