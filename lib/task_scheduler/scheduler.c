#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "task_queue/queue.h"
#include "task_table/table.h"


/**Scheduler object
 * 
 */
typedef struct
{
    // Rx attributes
    task_table_t table;
    serial_pkt_t rx_pkt;
    rx_schedule_cb rx_cb;

    // Tx attributes
    int16_t prev_task;
    tx_schedule_cb tx_cb;
    unsigned long start_time;
    schedule_queues_t * queues;
    timer_schedule_cb timer_cb;

} task_scheduler_t;


static task_scheduler_t scheduler;


/* Private scheduler macro functions */

#define reschedule_normal_task() reschedule_queue_task(scheduler.queues, false)
#define reschedule_priority_task() reschedule_queue_task(scheduler.queues, true)

#define prioritize_task() move_to_front(&scheduler.queues->priority_head, &scheduler.queues->normal_head)


/* Scheduler function prototypes */

static void perform_task(void);
static void process_current_task(void);


/* Public scheduler functions */

// Initialize task scheduler
bool init_task_scheduler(rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb)
{
    scheduler.prev_task = -1;

    scheduler.rx_cb = rx_cb;
    scheduler.tx_cb = tx_cb;
    scheduler.timer_cb = timer_cb;

    scheduler.table = init_task_table(TABLE_SIZE);
    scheduler.rx_pkt = init_serial_pkt(MAX_ENCODED_PKT_BUF_SIZE);
    scheduler.queues = init_scheduling_queues(QUEUE_SIZE, MAX_ENCODED_PKT_BUF_SIZE);

    // Verify if the scheduler was initialized correctly

    bool is_initialized = scheduler.table.entries && scheduler.rx_pkt.buf && scheduler.queues;

    if (!is_initialized)
    {
        deinit_task_scheduler();  
    }

    return is_initialized;
}


// Unitialize task scheduler
void deinit_task_scheduler(void)
{
    deinit_task_table(scheduler.table);
    deinit_serial_pkt(&scheduler.rx_pkt);
    deinit_scheduling_queues(scheduler.queues);
}


// Register a task in the scheduler
void register_task_private(uint8_t id, int payload_size, task_t task)
{
    register_task_in_table(&scheduler.table, id, payload_size, task);
}


// Form and detect the task rx packet reading byte-by-byte
void build_rx_task_pkt(uint8_t byte)
{
    if (process_incoming_byte(&scheduler.rx_pkt, byte))
    {
        perform_task();
    }
}


// A null/empty task for the scheduling system
void null_scheduler_task(void * _){}


// Schedule a task for an external device to perform
void schedule_task(uint8_t id, uint8_t type, uint8_t * pkt, uint8_t pkt_size, bool is_priority, bool is_fast)
{
    if (!in_queue(scheduler.queues, id))
    {
        // Send a task to free up space in queues (if queues were full)
        if (queues_are_full(scheduler.queues))
        {
            if (is_priority_queue_empty(scheduler.queues))
            {
                prioritize_normal_task(scheduler.queues);
            }

            send_task();
        }

        push_task(scheduler.queues, id, type, pkt, pkt_size, is_priority, is_fast);  // Put task in appropiate queue

        // Send task immediately if it's a fast task
        if (is_fast)
        {
            send_task();
        }
    }
}


// Send a task to be performed by another scheduling system connected to this one
void send_task(void)
{
    if (!queues_are_empty(scheduler.queues))
    {
        schedule_queues_t * queues = scheduler.queues;
        queue_entry_t * entry = is_priority_queue_empty(queues)? peek_normal(queues): peek_priority(queues);

        if (!is_priority_queue_empty(queues))  // Send priority tasks
        {
            scheduler.tx_cb(entry->pkt.buf, entry->pkt.byte_count);
            pop_priority_task(scheduler.queues);
        }
        else  // Send normal task
        {
            // Check if the task has been sent already
            if (scheduler.prev_task != entry->id)
            {
                scheduler.prev_task = entry->id;
                scheduler.start_time = scheduler.timer_cb();
                scheduler.tx_cb(entry->pkt.buf, entry->pkt.byte_count);  // Run the serial tx routine
            }

            // Check if elapsed time passed the reply window
            bool reply_time_passed;  // Indicates if allowed reply time window for normal task has passed
            if (entry->rescheduled)
            {
                reply_time_passed = scheduler.timer_cb() - scheduler.start_time >= LONG_TIMER;
            }
            else
            {
                reply_time_passed = scheduler.timer_cb() - scheduler.start_time >= SHORT_TIMER;
            }

            // Checks if the allowed reply time has passed
            if (reply_time_passed)
            {
                if (entry->rescheduled) // Second reply window range check
                {
                    pop_normal_task(scheduler.queues);
                }
                else // First reply window range check
                {
                    entry->rescheduled = true;
                    reschedule_normal_task();
                }
            }
        }
    }
}


/* Internal scheduler commands */

// Modify a task printer value in the main computer
void send_printer_task_var(uint8_t task_id, uint8_t task_type, uint8_t value_id, uint8_t value_type, void * value, size_t size)
{
    static uint8_t var_buf[sizeof(MAX_PRINTER_SEND_TYPE) + 4];  // Static storage to place the value of the variable

    if (size <= sizeof(MAX_PRINTER_SEND_TYPE))
    {
        var_buf[0] = task_id;
        var_buf[1] = task_type;
        var_buf[2] = value_id;
        var_buf[3] = value_type;
        memcpy(var_buf + 2, value, size);
        schedule_fast_task(MODIFY_PRINTER_VAR, INTERNAL_TASK, var_buf, size + 4);
    }
}


/**Decides what action for the task to take based on the other system's reply
 * 
 * This function is recognized as one of the internal commands of the
 * scheduling system. If the return code in the packet is non-zero, then
 * the system will reschedule the task. Otherwise, it will be taken out
 * of the normal queue.
 */ 
static void process_current_task(void)
{
    if (scheduler.queues->normal_head != NULL)
    {
        queue_entry_t* entry = peek_normal(scheduler.queues);

        if (scheduler.rx_pkt.buf[PAYLOAD_OFFSET] == entry->id)
        {
            if (scheduler.rx_pkt.buf[PAYLOAD_OFFSET + 1] && !entry->rescheduled)
            {
                entry->rescheduled = true;
                reschedule_normal_task();
            }
            else
            {
                pop_normal_task(scheduler.queues);
            }
        }
    }
}


/* Private scheduler functions */

/**Process the stored scheduler rx packet
 * 
 * In here, the packet that was built with process_incoming_byte
 * will be processed in its entirety. 
 */ 
static void perform_task(void)
{
    uint8_t ret_code;
    serial_pkt_t * rx_pkt = &scheduler.rx_pkt;
    task_entry_t * entry = process_incoming_pkt(scheduler.table, rx_pkt);

    if (entry != NULL) // If all checks passed, run rx callback (this is the handler for external tasks)
    {
        ret_code = scheduler.rx_cb(entry->id, entry->task, rx_pkt->buf + PAYLOAD_OFFSET);
        alert_task_completion(entry->id, ret_code);
    }

    else if (get_task_type(rx_pkt) == INTERNAL_TASK && get_task_id(rx_pkt) == ALERT_SYSTEM)
    {
        process_current_task();
    }

    rx_pkt->byte_count = 0;  // Reset rx packet buffer
}